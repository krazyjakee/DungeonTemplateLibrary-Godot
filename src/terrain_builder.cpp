#include "terrain_builder.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <queue>
#include <thread>
#include <utility>

using namespace godot;

TerrainBuilder::TerrainBuilder() {}
TerrainBuilder::~TerrainBuilder() {}

void TerrainBuilder::_bind_methods()
{
  ClassDB::bind_method(D_METHOD("build", "matrix", "cfg"), &TerrainBuilder::build);
}

static inline float cfg_f(const Dictionary &c, const char *k, float d)
{
  return c.has(k) ? (float)(double)c[k] : d;
}
static inline int cfg_i(const Dictionary &c, const char *k, int d)
{
  return c.has(k) ? (int)c[k] : d;
}
static inline bool cfg_b(const Dictionary &c, const char *k, bool d)
{
  return c.has(k) ? (bool)c[k] : d;
}
static inline float clampf_(float v, float lo, float hi)
{
  return v < lo ? lo : (v > hi ? hi : v);
}
static inline float lerpf_(float a, float b, float t) { return a + (b - a) * t; }

// Split [0, total) across hardware_concurrency() worker threads (capped at 8;
// runs serially below a small total so tiny passes don't pay thread-spawn
// overhead). Used for the embarrassingly-parallel per-row passes (blur,
// steepness, image pack, relax).
template <typename F>
static void parallel_rows(int total, F &&body)
{
  if (total <= 0)
    return;
  int hw = (int)std::thread::hardware_concurrency();
  if (hw <= 0)
    hw = 2;
  int n = std::min(hw, 8);
  if (n <= 1 || total < 64)
  {
    for (int i = 0; i < total; i++)
      body(i);
    return;
  }
  std::vector<std::thread> threads;
  threads.reserve(n);
  int chunk = (total + n - 1) / n;
  for (int t = 0; t < n; t++)
  {
    int start = t * chunk;
    int end = std::min(start + chunk, total);
    if (start >= end)
      break;
    threads.emplace_back([start, end, &body]() {
      for (int i = start; i < end; i++)
        body(i);
    });
  }
  for (auto &th : threads)
    th.join();
}

// Chaikin corner-cutting. The A* route is an 8-connected staircase; this
// rounds it into a smooth curve while pinning the endpoints so networks stay
// connected at the waypoints.
static std::vector<std::pair<float, float>> chaikin(
    const std::vector<std::pair<float, float>> &p, int iters)
{
  std::vector<std::pair<float, float>> cur = p;
  for (int it = 0; it < iters && cur.size() >= 3; it++)
  {
    std::vector<std::pair<float, float>> nx;
    nx.reserve(cur.size() * 2);
    nx.push_back(cur.front());
    for (size_t i = 0; i + 1 < cur.size(); i++)
    {
      const auto &a = cur[i];
      const auto &b = cur[i + 1];
      nx.push_back({0.75f * a.first + 0.25f * b.first, 0.75f * a.second + 0.25f * b.second});
      nx.push_back({0.25f * a.first + 0.75f * b.first, 0.25f * a.second + 0.75f * b.second});
    }
    nx.push_back(cur.back());
    cur.swap(nx);
  }
  return cur;
}

// Refreshed after each blur pass: max neighbour height delta in world units.
// A* and waypoint placement read this O(1) instead of recomputing 4 neighbour
// lookups per query.
void TerrainBuilder::compute_steepness()
{
  steep.assign((size_t)w * h, 0.0f);
  float scale = world_per_buf / world_per_texel;
  parallel_rows(h, [&](int y) {
    int row = y * w;
    for (int x = 0; x < w; x++)
    {
      float b = buf[row + x];
      float d = 0.0f;
      if (x + 1 < w) d = std::max(d, std::fabs(buf[row + x + 1] - b));
      if (x > 0)     d = std::max(d, std::fabs(buf[row + x - 1] - b));
      if (y + 1 < h) d = std::max(d, std::fabs(buf[row + w + x] - b));
      if (y > 0)     d = std::max(d, std::fabs(buf[row - w + x] - b));
      steep[row + x] = d * scale;
    }
  });
}

// Separable two-pass 3x3 box blur (two passes -> ~5x5 Gaussian). Horizontal
// then vertical = 3 reads + 1 write per pixel per axis (vs 9 reads for the
// naive 2D kernel). Cache-friendly and parallel over rows.
void TerrainBuilder::box_blur_2x_separable()
{
  std::vector<float> tmp(buf.size());
  for (int pass = 0; pass < 2; pass++)
  {
    // Horizontal: buf -> tmp
    parallel_rows(h, [&](int y) {
      int row = y * w;
      for (int x = 0; x < w; x++)
      {
        float sum = buf[row + x];
        int cnt = 1;
        if (x > 0)     { sum += buf[row + x - 1]; cnt++; }
        if (x + 1 < w) { sum += buf[row + x + 1]; cnt++; }
        tmp[row + x] = sum / (float)cnt;
      }
    });
    // Vertical: tmp -> buf
    parallel_rows(h, [&](int y) {
      int row = y * w;
      for (int x = 0; x < w; x++)
      {
        float sum = tmp[row + x];
        int cnt = 1;
        if (y > 0)     { sum += tmp[row - w + x]; cnt++; }
        if (y + 1 < h) { sum += tmp[row + w + x]; cnt++; }
        buf[row + x] = sum / (float)cnt;
      }
    });
  }
}

// Full-resolution A*. Edge cost = world distance scaled up quadratically by
// the per-texel grade so the search detours around steep ground; grades over
// the cap are impassable (the goal cell is always reachable so a waypoint on
// rough ground still terminates). No coarse grid — at full res a cliff is
// never hidden between samples.
//
// Uses a generation counter: each call bumps cur_gen, and visited_gen[i] ==
// cur_gen tells us whether g_cost/came hold valid data for this run. This
// avoids three full-buffer fills (≈12 MB for a 1000x1000 map) per A* call.
std::vector<int> TerrainBuilder::astar(int sx, int sy, int gx, int gy)
{
  float slope_weight = lerpf_(2.0f, 24.0f, hill_avoidance);
  float hard_cap = lerpf_(1.2f, 0.30f, hill_avoidance);
  const int start = sy * w + sx;
  const int goal = gy * w + gx;
  const float diag = 1.41421356f;

  // Elevation-following term: penalize straying from the gentle straight
  // grade that connects the two endpoint heights. This makes the search
  // route *around* a hill at the right contour instead of climbing over it.
  float h0 = h_world(buf[start]);
  float h1 = h_world(buf[goal]);
  float route_span = std::sqrt((float)((sx - gx) * (sx - gx) + (sy - gy) * (sy - gy)));
  if (route_span < 1.0f)
    route_span = 1.0f;
  float elev_weight = lerpf_(0.5f, 12.0f, hill_avoidance);
  float elev_scale = std::max(1.0f, amplitude * 0.15f);

  static const int nx8[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  static const int ny8[8] = {0, 0, 1, -1, 1, -1, 1, -1};

  for (int relax = 0; relax < 2; relax++)
  {
    float cap = (relax == 0) ? hard_cap : hard_cap * 2.5f;

    cur_gen++;

    using QN = std::pair<float, int>;
    std::priority_queue<QN, std::vector<QN>, std::greater<QN>> open;

    g_cost[start] = 0.0f;
    came[start] = -1;
    visited_gen[start] = cur_gen;
    float hx = (float)(sx - gx), hy = (float)(sy - gy);
    open.push({std::sqrt(hx * hx + hy * hy) * world_per_texel, start});

    bool found = false;
    while (!open.empty())
    {
      int cur = open.top().second;
      open.pop();
      if (cur == goal)
      {
        found = true;
        break;
      }
      if (closed_gen[cur] == cur_gen)
        continue;
      closed_gen[cur] = cur_gen;
      int cx = cur % w;
      int cy = cur / w;
      float ha = h_world(buf[cur]);
      float steep_cur = steep[cur];
      for (int k = 0; k < 8; k++)
      {
        int nx = cx + nx8[k];
        int ny = cy + ny8[k];
        if (nx < 0 || nx >= w || ny < 0 || ny >= h)
          continue;
        int ni = ny * w + nx;
        if (closed_gen[ni] == cur_gen)
          continue;
        float dist = world_per_texel;
        if (nx8[k] != 0 && ny8[k] != 0)
          dist *= diag;
        // Effective steepness = worse of along-track grade and the terrain's
        // own steepest local face at either cell. The cross-slope term is what
        // stops the route from sidehilling across a steep face — i.e. it
        // routes *around* the hill, not along its side.
        float along = std::fabs(h_world(buf[ni]) - ha) / dist;
        float slope = std::max(along, std::max(steep_cur, steep[ni]));
        if (slope > cap && ni != goal)
          continue;
        float r = slope / cap;
        float fdx = (float)(nx - sx), fdy = (float)(ny - sy);
        float frac = clampf_(std::sqrt(fdx * fdx + fdy * fdy) / route_span, 0.0f, 1.0f);
        float target_h = lerpf_(h0, h1, frac);
        float dev = std::fabs(h_world(buf[ni]) - target_h) / elev_scale;
        float step = dist * (1.0f + slope_weight * r * r + elev_weight * dev * dev);
        float tentative = g_cost[cur] + step;
        float prior = (visited_gen[ni] == cur_gen) ? g_cost[ni]
                                                    : std::numeric_limits<float>::infinity();
        if (tentative < prior)
        {
          g_cost[ni] = tentative;
          came[ni] = cur;
          visited_gen[ni] = cur_gen;
          float ex = (float)(nx - gx), ey = (float)(ny - gy);
          open.push({tentative + std::sqrt(ex * ex + ey * ey) * world_per_texel, ni});
        }
      }
    }

    if (!found && goal != start)
      continue;

    std::vector<int> path;
    int node = goal;
    while (node != -1)
    {
      path.push_back(node);
      if (node == start)
        break;
      node = came[node];
    }
    std::reverse(path.begin(), path.end());
    if (path.size() >= 2)
      return path;
  }
  return std::vector<int>();
}

void TerrainBuilder::carve_route(const std::vector<std::pair<float, float>> &poly,
                                 float width_world, float max_grade, int mask_channel)
{
  // Sample spacing tied to road width: the carve uses true distance to the
  // nearest sample, so overlapping samples a fraction of a width apart give a
  // smooth distance field cheaply.
  float wr = std::max(1.0f, (width_world * 0.5f) / world_per_texel);
  float spacing = std::max(1.0f, wr * 0.75f);

  std::vector<std::pair<float, float>> pts;
  for (size_t i = 0; i + 1 < poly.size(); i++)
  {
    float dx = poly[i + 1].first - poly[i].first;
    float dy = poly[i + 1].second - poly[i].second;
    float len = std::sqrt(dx * dx + dy * dy);
    int steps = std::max(1, (int)std::ceil(len / spacing));
    for (int s = 0; s < steps; s++)
    {
      float t = (float)s / (float)steps;
      pts.push_back({poly[i].first + dx * t, poly[i].second + dy * t});
    }
  }
  pts.push_back(poly.back());
  if (pts.size() < 2)
    return;

  // Centerline surface height (world) from the natural terrain, grade-limited
  // forward+backward then smoothed so it climbs gently — the longitudinal
  // "not too steep" constraint. `ground` keeps the un-graded natural height
  // under the centerline so we know how deep the cut/fill is at each sample.
  float sea_world = sea_level;
  std::vector<float> surf(pts.size());
  std::vector<float> ground(pts.size());
  for (size_t i = 0; i < pts.size(); i++)
  {
    int px = std::clamp((int)std::lround(pts[i].first), 0, w - 1);
    int py = std::clamp((int)std::lround(pts[i].second), 0, h - 1);
    float g = std::max(h_world(orig[py * w + px]), sea_world + 0.01f * amplitude);
    ground[i] = g;
    surf[i] = g;
  }
  // Broad (low-pass) ground reference: the cut/fill cap follows the large-
  // scale terrain, not local noise, so the bed can never dive into a crater
  // far below the land around it. Scratch buffer is reused across iterations
  // instead of reallocating a fresh vector inside the loop.
  std::vector<float> gsm = ground;
  std::vector<float> scratch(gsm.size());
  for (int p = 0; p < 12; p++)
  {
    std::memcpy(scratch.data(), gsm.data(), gsm.size() * sizeof(float));
    for (size_t i = 1; i + 1 < gsm.size(); i++)
      gsm[i] = (scratch[i - 1] + 2.0f * scratch[i] + scratch[i + 1]) * 0.25f;
  }
  // Hard cap on how far the bed may sit below / above the broad terrain. A
  // route that would need a deeper cut than this is A*'s problem to route
  // around (the elevation-following cost now does that); the carve must never
  // bore a pit. Endpoints stay pinned to natural ground so networks connect.
  float max_dev = clampf_(amplitude * 0.12f, 3.0f, 18.0f);
  float ah = surf.front();
  float bh = surf.back();
  scratch.resize(surf.size());
  for (int it = 0; it < 24; it++)
  {
    for (size_t i = 1; i < pts.size(); i++)
    {
      float dx = pts[i].first - pts[i - 1].first, dy = pts[i].second - pts[i - 1].second;
      float seg = std::max(std::sqrt(dx * dx + dy * dy) * world_per_texel, 0.001f);
      float md = max_grade * seg;
      surf[i] = clampf_(surf[i], surf[i - 1] - md, surf[i - 1] + md);
    }
    for (int i = (int)pts.size() - 2; i >= 0; i--)
    {
      float dx = pts[i + 1].first - pts[i].first, dy = pts[i + 1].second - pts[i].second;
      float seg = std::max(std::sqrt(dx * dx + dy * dy) * world_per_texel, 0.001f);
      float md = max_grade * seg;
      surf[i] = clampf_(surf[i], surf[i + 1] - md, surf[i + 1] + md);
    }
    std::memcpy(scratch.data(), surf.data(), surf.size() * sizeof(float));
    for (size_t i = 1; i + 1 < pts.size(); i++)
      surf[i] = (scratch[i - 1] + 2.0f * scratch[i] + scratch[i + 1]) * 0.25f;
    for (size_t i = 1; i + 1 < pts.size(); i++)
      surf[i] = clampf_(surf[i], gsm[i] - max_dev, gsm[i] + max_dev);
    surf.front() = ah;
    surf.back() = bh;
  }

  // Lateral embankment grade. Bounded well below vertical, so the terrain
  // *always* ramps gently down to the road bed no matter how deep the cut.
  // A deep crossing just gets a correspondingly broad smooth cutting /
  // embankment instead of a cliff.
  float lat_grade = clampf_(max_grade * 4.0f, 0.06f, 0.30f);
  float min_sh = std::max(width_world * 0.5f, 2.0f * world_per_texel);
  float max_scan = (float)std::min(w, h); // safety bound only

  // Accumulate a smooth influence field. Every sample contributes a weighted
  // vote for its bed height to nearby texels; build() later composites the
  // weighted average and lerps it against the natural terrain by the strongest
  // influence. Because it is a continuous blend (not a nearest-sample winner),
  // parallel passes and crossings merge instead of stair-stepping.
  for (size_t i = 0; i < pts.size(); i++)
  {
    float cxf = pts[i].first, cyf = pts[i].second;
    float sw = surf[i];
    float depth = std::fabs(ground[i] - sw);
    float sh_len = std::max(min_sh, depth / lat_grade); // world units
    float scan_r = wr + sh_len / world_per_texel + 1.0f;
    if (scan_r > max_scan)
      scan_r = max_scan;
    // The falloff must reach zero by the edge of whatever we actually scan,
    // so a clamped scan can never leave a hard rim.
    float fall = std::min(sh_len, (scan_r - wr) * world_per_texel);
    if (fall < min_sh)
      fall = min_sh;

    int x0 = std::max(0, (int)std::floor(cxf - scan_r));
    int x1 = std::min(w - 1, (int)std::ceil(cxf + scan_r));
    int y0 = std::max(0, (int)std::floor(cyf - scan_r));
    int y1 = std::min(h - 1, (int)std::ceil(cyf + scan_r));
    for (int ty = y0; ty <= y1; ty++)
    {
      for (int tx = x0; tx <= x1; tx++)
      {
        int idx = ty * w + tx;
        float ddx = tx - cxf, ddy = ty - cyf;
        float d = std::sqrt(ddx * ddx + ddy * ddy);
        float infl;
        if (d <= wr)
        {
          infl = 1.0f; // flat road bed
        }
        else
        {
          float ld = (d - wr) * world_per_texel;
          if (ld >= fall)
            continue; // outside this sample's smooth reach
          float u = ld / fall;
          infl = 1.0f - u * u * (3.0f - 2.0f * u); // smoothstep falloff
        }
        if (infl <= 0.0f)
          continue;
        // Square the weight so the flat bed strongly dominates over a faraway
        // embankment skirt that happens to overlap it.
        float wgt = infl * infl;
        acc_h[idx] += wgt * sw;
        acc_w[idx] += wgt;
        if (infl > max_infl[idx])
          max_infl[idx] = infl;
        float m = clampf_(d <= wr ? 1.0f
                                  : 1.0f - (d - wr) / std::max(wr * 0.6f, 1.0f),
                          0.0f, 1.0f);
        if (m > 0.0f)
        {
          int mi = idx * 2 + mask_channel;
          mask[mi] = std::max(mask[mi], m);
        }
      }
    }
  }
}

void TerrainBuilder::build_network(std::mt19937 &rng, int node_count,
                                   float width_world, float max_grade, int mask_channel)
{
  float min_wp_world = sea_level + 0.04f * amplitude;
  float min_wp_buf = world_to_buf(min_wp_world);
  int margin = std::max(2, (int)(std::min(w, h) * 0.08f));
  float min_sep = std::min(w, h) * 0.18f;
  float flat_thresh = lerpf_(1.2f, 0.35f, hill_avoidance);

  std::uniform_int_distribution<int> dx(margin, w - 1 - margin);
  std::uniform_int_distribution<int> dy(margin, h - 1 - margin);

  std::vector<std::pair<int, int>> wp;
  int attempts = 0;
  int max_attempts = node_count * 200;
  while ((int)wp.size() < node_count && attempts < max_attempts)
  {
    attempts++;
    int px = dx(rng);
    int py = dy(rng);
    if (buf[py * w + px] <= min_wp_buf)
      continue;
    float bar = (attempts < node_count * 120) ? flat_thresh : flat_thresh * 3.0f;
    if (steep[py * w + px] > bar)
      continue;
    bool ok = true;
    for (auto &p : wp)
    {
      float sx = (float)(px - p.first), sy = (float)(py - p.second);
      if (std::sqrt(sx * sx + sy * sy) < min_sep)
      {
        ok = false;
        break;
      }
    }
    if (ok)
      wp.push_back({px, py});
  }
  if (wp.size() < 2)
    return;

  // Minimum spanning tree (Prim) by planar distance — one connected network,
  // no redundant loops.
  int n = (int)wp.size();
  std::vector<bool> in_tree(n, false);
  in_tree[0] = true;
  std::vector<std::pair<int, int>> edges;
  for (int e = 0; e < n - 1; e++)
  {
    float best = std::numeric_limits<float>::infinity();
    int ba = -1, bb = -1;
    for (int a = 0; a < n; a++)
    {
      if (!in_tree[a])
        continue;
      for (int b = 0; b < n; b++)
      {
        if (in_tree[b])
          continue;
        float sx = (float)(wp[a].first - wp[b].first), sy = (float)(wp[a].second - wp[b].second);
        float d = std::sqrt(sx * sx + sy * sy);
        if (d < best)
        {
          best = d;
          ba = a;
          bb = b;
        }
      }
    }
    if (bb < 0)
      break;
    in_tree[bb] = true;
    edges.push_back({ba, bb});
  }

  for (auto &e : edges)
  {
    std::vector<int> route = astar(wp[e.first].first, wp[e.first].second,
                                   wp[e.second].first, wp[e.second].second);
    if (route.size() < 2)
      continue;
    // Strip the staircase to a coarse control polygon, then Chaikin-smooth
    // it into a flowing curve (endpoints pinned to the waypoints).
    int stride = std::max(3, (int)std::lround((width_world * 0.75f) / world_per_texel));
    std::vector<std::pair<float, float>> ctrl;
    for (size_t i = 0; i < route.size(); i += stride)
      ctrl.push_back({(float)(route[i] % w), (float)(route[i] / w)});
    int last = route.back();
    if (ctrl.empty() ||
        ctrl.back().first != (float)(last % w) || ctrl.back().second != (float)(last / w))
      ctrl.push_back({(float)(last % w), (float)(last / w)});
    std::vector<std::pair<float, float>> poly = chaikin(ctrl, 3);
    for (auto &q : poly)
    {
      q.first = clampf_(q.first, 0.0f, (float)(w - 1));
      q.second = clampf_(q.second, 0.0f, (float)(h - 1));
    }
    carve_route(poly, width_world, max_grade, mask_channel);
  }
}

Dictionary TerrainBuilder::build(Dictionary matrix, Dictionary cfg)
{
  Dictionary out;
  if (!matrix.has("width") || !matrix.has("height") || !matrix.has("data"))
    return out;
  w = (int)matrix["width"];
  h = (int)matrix["height"];
  if (w <= 0 || h <= 0)
    return out;
  PackedByteArray data = matrix["data"];
  if (data.size() < (int64_t)w * h)
    return out;
  const uint8_t *src = data.ptr();

  amplitude = cfg_f(cfg, "amplitude", 100.0f);
  sea_level = cfg_f(cfg, "sea_level", 0.0f);
  terrain_size = cfg_f(cfg, "terrain_size", 1000.0f);
  hill_avoidance = clampf_(cfg_f(cfg, "hill_avoidance", 0.7f), 0.0f, 1.0f);
  bool paths_enabled = cfg_b(cfg, "paths_enabled", false);
  bool roads_enabled = cfg_b(cfg, "roads_enabled", false);

  int lowest_i = 999999, highest_i = -999999;
  buf.assign((size_t)w * h, 0.0f);
  // Single pass over the flat byte buffer — no Variant marshalling.
  for (int i = 0, n = w * h; i < n; i++)
  {
    int cell = (int)src[i];
    buf[i] = (float)cell;
    if (cell < lowest_i) lowest_i = cell;
    if (cell > highest_i) highest_i = cell;
  }
  lowest = (float)lowest_i;
  range = (float)(highest_i - lowest_i);
  if (range == 0.0f)
    range = 1.0f;
  world_per_texel = terrain_size / (float)w;
  world_per_buf = amplitude / range;

  box_blur_2x_separable();
  compute_steepness();

  mask.assign((size_t)w * h * 2, 0.0f);

  if (paths_enabled || roads_enabled)
  {
    orig = buf; // natural reference, captured before any carve
    acc_h.assign((size_t)w * h, 0.0f);
    acc_w.assign((size_t)w * h, 0.0f);
    max_infl.assign((size_t)w * h, 0.0f);
    // A* scratch — sized once. visited_gen / closed_gen are bumped per call
    // (cur_gen), no fill needed; g_cost / came are read only when the gen
    // matches, so their initial garbage is fine.
    g_cost.assign((size_t)w * h, 0.0f);
    came.assign((size_t)w * h, -1);
    visited_gen.assign((size_t)w * h, 0);
    closed_gen.assign((size_t)w * h, 0);
    cur_gen = 0;

    unsigned int seed = (unsigned int)cfg_i(cfg, "seed", 0);
    std::mt19937 rng(seed);
    if (paths_enabled)
      build_network(rng, std::max(2, cfg_i(cfg, "path_node_count", 7)),
                    std::max(0.5f, cfg_f(cfg, "path_width", 4.0f)),
                    clampf_(cfg_f(cfg, "path_max_grade", 0.14f), 0.01f, 1.0f), 0);
    if (roads_enabled)
      build_network(rng, std::max(2, cfg_i(cfg, "road_node_count", 5)),
                    std::max(0.5f, cfg_f(cfg, "road_width", 11.0f)),
                    clampf_(cfg_f(cfg, "road_max_grade", 0.08f), 0.01f, 1.0f), 1);

    // Composite all routes at once: weighted-average bed height, lerped
    // against the natural terrain by the strongest influence. Parallelized
    // over rows since each texel writes only to itself.
    float sea_buf = world_to_buf(sea_level);
    parallel_rows(h, [&](int y) {
      int row = y * w;
      for (int x = 0; x < w; x++)
      {
        int idx = row + x;
        if (acc_w[idx] <= 0.0f)
          continue;
        if (orig[idx] <= sea_buf)
          continue;
        float blended = acc_h[idx] / acc_w[idx];
        float infl = clampf_(max_infl[idx], 0.0f, 1.0f);
        float ow = h_world(orig[idx]);
        buf[idx] = world_to_buf(lerpf_(ow, blended, infl));
      }
    });

    // Relax the carved band so any residual seam — notably where two routes
    // at different bed heights cross — is smoothed out. Fully-flat beds
    // (infl≈1) and untouched terrain (infl≈0) are pinned so roads stay flat
    // and the natural landscape is left alone; only the transition skirt and
    // crossings move. Reuses one scratch buffer instead of allocating per
    // pass; parallelized over rows.
    std::vector<float> relax_scratch(buf.size());
    for (int pass = 0; pass < 3; pass++)
    {
      std::memcpy(relax_scratch.data(), buf.data(), buf.size() * sizeof(float));
      parallel_rows(h, [&](int y) {
        int row = y * w;
        for (int x = 0; x < w; x++)
        {
          int idx = row + x;
          float mi = max_infl[idx];
          if (mi <= 0.001f || mi >= 0.999f)
            continue;
          float sum = 0.0f;
          int cnt = 0;
          if (x > 0)     { sum += relax_scratch[idx - 1]; cnt++; }
          if (x + 1 < w) { sum += relax_scratch[idx + 1]; cnt++; }
          if (y > 0)     { sum += relax_scratch[idx - w]; cnt++; }
          if (y + 1 < h) { sum += relax_scratch[idx + w]; cnt++; }
          if (cnt == 0)
            continue;
          buf[idx] = lerpf_(relax_scratch[idx], sum / (float)cnt, 0.5f);
        }
      });
    }
  }

  // Cache the center-height before we hand `buf` away. Used by DrawMatrix3D
  // to seed an initial camera focus / debug readout.
  float center_height = 0.0f;
  {
    int cx = w / 2;
    int cy = h / 2;
    center_height = (buf[cy * w + cx] - lowest) / range * amplitude;
  }
  out["center_height"] = center_height;

  // Pack normalized heights into a flat float buffer and create the Image from
  // raw bytes — Image::set_pixel goes through the Variant call layer per pixel,
  // which dominates at 1M+ pixels. Parallelized over rows.
  PackedByteArray height_bytes;
  height_bytes.resize((int64_t)w * h * (int64_t)sizeof(float));
  {
    float *dst = reinterpret_cast<float *>(height_bytes.ptrw());
    float inv_range = 1.0f / range;
    parallel_rows(h, [&](int y) {
      int row = y * w;
      for (int x = 0; x < w; x++)
        dst[row + x] = (buf[row + x] - lowest) * inv_range;
    });
  }
  Ref<Image> himg = Image::create_from_data(w, h, false, Image::FORMAT_RF, height_bytes);
  himg->generate_mipmaps();

  // Mask is two floats per pixel (R=path, G=road). It's a paint mask consumed
  // at one mip level, so skip mipmap generation — saves time at large sizes
  // and the shader doesn't sample mips of it.
  PackedByteArray mask_bytes;
  mask_bytes.resize((int64_t)w * h * 2 * (int64_t)sizeof(float));
  {
    float *dst = reinterpret_cast<float *>(mask_bytes.ptrw());
    parallel_rows(h, [&](int y) {
      int row = y * w;
      for (int x = 0; x < w; x++)
      {
        int i = (row + x) * 2;
        dst[i] = mask[i];
        dst[i + 1] = mask[i + 1];
      }
    });
  }
  Ref<Image> mimg = Image::create_from_data(w, h, false, Image::FORMAT_RGF, mask_bytes);

  out["height"] = ImageTexture::create_from_image(himg);
  out["mask"] = ImageTexture::create_from_image(mimg);

  // Hand the carved height buffer back so TerrainLOD's collider sampling and
  // anything else that needs raw heights can read from a plain
  // PackedFloat32Array instead of paying the Image::get_pixel cost.
  PackedFloat32Array heights_packed;
  heights_packed.resize(w * h);
  std::memcpy(heights_packed.ptrw(), buf.data(), buf.size() * sizeof(float));
  out["heights"] = heights_packed;
  out["lowest"] = lowest;
  out["range"] = range;

  // Free the big scratch buffers; the textures are what callers keep.
  buf.clear(); buf.shrink_to_fit();
  orig.clear(); orig.shrink_to_fit();
  mask.clear(); mask.shrink_to_fit();
  steep.clear(); steep.shrink_to_fit();
  acc_h.clear(); acc_h.shrink_to_fit();
  acc_w.clear(); acc_w.shrink_to_fit();
  max_infl.clear(); max_infl.shrink_to_fit();
  g_cost.clear(); g_cost.shrink_to_fit();
  came.clear(); came.shrink_to_fit();
  visited_gen.clear(); visited_gen.shrink_to_fit();
  closed_gen.clear(); closed_gen.shrink_to_fit();
  return out;
}
