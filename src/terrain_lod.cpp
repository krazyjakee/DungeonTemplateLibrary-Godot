#include "terrain_lod.hpp"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace godot;

TerrainLOD::TerrainLOD()
{
  set_process(true);
}
TerrainLOD::~TerrainLOD() {}

void TerrainLOD::_bind_methods()
{
  ClassDB::bind_method(D_METHOD("generate", "matrix"), &TerrainLOD::generate);
  ClassDB::bind_method(D_METHOD("clear_terrain"), &TerrainLOD::clear_terrain);

  ClassDB::bind_method(D_METHOD("set_chunk_count", "count"), &TerrainLOD::set_chunk_count);
  ClassDB::bind_method(D_METHOD("get_chunk_count"), &TerrainLOD::get_chunk_count);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "chunk_count"), "set_chunk_count", "get_chunk_count");

  ClassDB::bind_method(D_METHOD("set_lod_levels", "levels"), &TerrainLOD::set_lod_levels);
  ClassDB::bind_method(D_METHOD("get_lod_levels"), &TerrainLOD::get_lod_levels);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "lod_levels"), "set_lod_levels", "get_lod_levels");

  ClassDB::bind_method(D_METHOD("set_lod0_subdivisions", "subdivisions"), &TerrainLOD::set_lod0_subdivisions);
  ClassDB::bind_method(D_METHOD("get_lod0_subdivisions"), &TerrainLOD::get_lod0_subdivisions);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "lod0_subdivisions"), "set_lod0_subdivisions", "get_lod0_subdivisions");

  ClassDB::bind_method(D_METHOD("set_lod_distance_multiplier", "multiplier"), &TerrainLOD::set_lod_distance_multiplier);
  ClassDB::bind_method(D_METHOD("get_lod_distance_multiplier"), &TerrainLOD::get_lod_distance_multiplier);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod_distance_multiplier"), "set_lod_distance_multiplier", "get_lod_distance_multiplier");

  ClassDB::bind_method(D_METHOD("set_draw_matrix_3d", "path"), &TerrainLOD::set_draw_matrix_3d);
  ClassDB::bind_method(D_METHOD("get_draw_matrix_3d"), &TerrainLOD::get_draw_matrix_3d);
  ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "draw_matrix_3d"), "set_draw_matrix_3d", "get_draw_matrix_3d");

  ClassDB::bind_method(D_METHOD("set_collision_enabled", "enabled"), &TerrainLOD::set_collision_enabled);
  ClassDB::bind_method(D_METHOD("is_collision_enabled"), &TerrainLOD::is_collision_enabled);
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collision_enabled"), "set_collision_enabled", "is_collision_enabled");

  ClassDB::bind_method(D_METHOD("set_collision_radius", "radius"), &TerrainLOD::set_collision_radius);
  ClassDB::bind_method(D_METHOD("get_collision_radius"), &TerrainLOD::get_collision_radius);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_radius"), "set_collision_radius", "get_collision_radius");

  ClassDB::bind_method(D_METHOD("set_collision_subdivisions", "subdivisions"), &TerrainLOD::set_collision_subdivisions);
  ClassDB::bind_method(D_METHOD("get_collision_subdivisions"), &TerrainLOD::get_collision_subdivisions);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_subdivisions"), "set_collision_subdivisions", "get_collision_subdivisions");

  ClassDB::bind_method(D_METHOD("set_collision_update_interval", "interval"), &TerrainLOD::set_collision_update_interval);
  ClassDB::bind_method(D_METHOD("get_collision_update_interval"), &TerrainLOD::get_collision_update_interval);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_update_interval"), "set_collision_update_interval", "get_collision_update_interval");

  ClassDB::bind_method(D_METHOD("set_player_path", "path"), &TerrainLOD::set_player_path);
  ClassDB::bind_method(D_METHOD("get_player_path"), &TerrainLOD::get_player_path);
  ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player_path"), "set_player_path", "get_player_path");
}

// Property getters/setters
void TerrainLOD::set_chunk_count(int p_count) { chunk_count = std::max(1, p_count); }
int TerrainLOD::get_chunk_count() const { return chunk_count; }

void TerrainLOD::set_lod_levels(int p_levels) { lod_levels = std::max(1, p_levels); }
int TerrainLOD::get_lod_levels() const { return lod_levels; }

void TerrainLOD::set_lod0_subdivisions(int p_subs) { lod0_subdivisions = std::max(2, p_subs); }
int TerrainLOD::get_lod0_subdivisions() const { return lod0_subdivisions; }

void TerrainLOD::set_lod_distance_multiplier(float p_mult) { lod_distance_multiplier = std::max(0.1f, p_mult); }
float TerrainLOD::get_lod_distance_multiplier() const { return lod_distance_multiplier; }

void TerrainLOD::set_draw_matrix_3d(NodePath p_path) { draw_matrix_3d_path = p_path; }
NodePath TerrainLOD::get_draw_matrix_3d() const { return draw_matrix_3d_path; }

void TerrainLOD::set_collision_enabled(bool p_enabled)
{
  collision_enabled = p_enabled;
  if (!p_enabled)
  {
    for (auto &chunk : chunks)
      _free_chunk_collision(chunk);
  }
}
bool TerrainLOD::is_collision_enabled() const { return collision_enabled; }

void TerrainLOD::set_collision_radius(float p_radius) { collision_radius = std::max(0.0f, p_radius); }
float TerrainLOD::get_collision_radius() const { return collision_radius; }

void TerrainLOD::set_collision_subdivisions(int p_subs) { collision_subdivisions = std::max(2, p_subs); }
int TerrainLOD::get_collision_subdivisions() const { return collision_subdivisions; }

void TerrainLOD::set_collision_update_interval(float p_interval) { collision_update_interval = std::max(0.0f, p_interval); }
float TerrainLOD::get_collision_update_interval() const { return collision_update_interval; }

void TerrainLOD::set_player_path(NodePath p_path) { player_path = p_path; }
NodePath TerrainLOD::get_player_path() const { return player_path; }

void TerrainLOD::clear_terrain()
{
  chunks.clear();
  while (get_child_count() > 0)
  {
    Node *child = get_child(0);
    remove_child(child);
    child->queue_free();
  }
}

// Fallback path: rebuild the same normalized-height image that DrawMatrix3D /
// TerrainBuilder would produce. Only used if generate() runs without an
// already-populated DrawMatrix3D sibling. Skips the path/road carve entirely
// since the carve lives in TerrainBuilder. Goes through raw PackedByteArray
// (no Image::set_pixel) so it remains fast at 1000x1000.
Ref<ImageTexture> TerrainLOD::_create_height_texture(Dictionary matrix)
{
  if (!matrix.has("width") || !matrix.has("height") || !matrix.has("data"))
    return Ref<ImageTexture>();
  int map_w = (int)matrix["width"];
  int map_h = (int)matrix["height"];
  if (map_w <= 0 || map_h <= 0)
    return Ref<ImageTexture>();
  PackedByteArray data = matrix["data"];
  if (data.size() < (int64_t)map_w * map_h)
    return Ref<ImageTexture>();
  const uint8_t *src = data.ptr();

  std::vector<float> raw((size_t)map_w * map_h);
  int highest_value = -999999;
  int lowest_value = 999999;
  for (int i = 0, n = map_w * map_h; i < n; i++)
  {
    int cell = (int)src[i];
    raw[i] = (float)cell;
    if (cell < lowest_value) lowest_value = cell;
    if (cell > highest_value) highest_value = cell;
  }

  float range = (float)(highest_value - lowest_value);
  if (range == 0.0f)
    range = 1.0f;
  float lowest_f = (float)lowest_value;

  // Separable 3x3 box blur, two passes. Cache-friendly and only ~6 reads per
  // pixel per pass vs 9 for the naive 2D kernel.
  std::vector<float> tmp(raw.size());
  for (int pass = 0; pass < 2; pass++)
  {
    for (int y = 0; y < map_h; y++)
    {
      int row = y * map_w;
      for (int x = 0; x < map_w; x++)
      {
        float sum = raw[row + x];
        int cnt = 1;
        if (x > 0)         { sum += raw[row + x - 1]; cnt++; }
        if (x + 1 < map_w) { sum += raw[row + x + 1]; cnt++; }
        tmp[row + x] = sum / (float)cnt;
      }
    }
    for (int y = 0; y < map_h; y++)
    {
      int row = y * map_w;
      for (int x = 0; x < map_w; x++)
      {
        float sum = tmp[row + x];
        int cnt = 1;
        if (y > 0)         { sum += tmp[row - map_w + x]; cnt++; }
        if (y + 1 < map_h) { sum += tmp[row + map_w + x]; cnt++; }
        raw[row + x] = sum / (float)cnt;
      }
    }
  }

  // Normalize to 0..1 and pack as raw RF bytes — Image::create_from_data
  // copies the buffer directly with no per-pixel marshalling.
  PackedByteArray height_bytes;
  height_bytes.resize((int64_t)map_w * map_h * (int64_t)sizeof(float));
  float *dst = reinterpret_cast<float *>(height_bytes.ptrw());
  float inv_range = 1.0f / range;
  heights_norm.resize(map_w * map_h);
  float *hp = heights_norm.ptrw();
  for (int i = 0, n = map_w * map_h; i < n; i++)
  {
    float v = (raw[i] - lowest_f) * inv_range;
    dst[i] = v;
    hp[i] = v;
  }
  heights_w = map_w;
  heights_h = map_h;

  Ref<Image> image = Image::create_from_data(map_w, map_h, false, Image::FORMAT_RF, height_bytes);
  image->generate_mipmaps();
  return ImageTexture::create_from_image(image);
}

void TerrainLOD::generate(Dictionary matrix)
{
  if (draw_matrix_3d_path.is_empty())
  {
    UtilityFunctions::printerr("TerrainLOD: draw_matrix_3d path is not set.");
    return;
  }

  Node *dm_node = get_node_or_null(draw_matrix_3d_path);
  if (dm_node == nullptr)
  {
    UtilityFunctions::printerr("TerrainLOD: could not find DrawMatrix3D node at path: ", draw_matrix_3d_path);
    return;
  }

  // Prefer the heightmap DrawMatrix3D already built: it owns the path/road
  // carve, so sharing it keeps the visible LOD mesh, its colliders, and the
  // water plane all deforming off one source. (DrawMatrix3D.draw_terrain runs
  // before TerrainLOD.generate in every terrain script.) Fall back to
  // rebuilding from the raw matrix only if it hasn't generated yet.
  Ref<ImageTexture> dm_tex = dm_node->get("height_texture");
  if (dm_tex.is_valid())
  {
    height_texture = dm_tex;
    // Heights come over as a flat PackedFloat32Array (no Image::get_pixel).
    Variant dm_heights = dm_node->get("heights_packed");
    if (dm_heights.get_type() == Variant::PACKED_FLOAT32_ARRAY)
      heights_norm = dm_heights;
    else
      heights_norm = PackedFloat32Array();
    heights_w = (int)dm_node->get("heights_width");
    heights_h = (int)dm_node->get("heights_height");
    if (heights_w <= 0 || heights_h <= 0 || heights_norm.size() < heights_w * heights_h)
    {
      // Fallback: derive from the image once (slow per-pixel read), but cache.
      Ref<Image> img = dm_tex->get_image();
      if (img.is_valid())
      {
        heights_w = img->get_width();
        heights_h = img->get_height();
        PackedByteArray bytes = img->get_data();
        if (bytes.size() >= (int64_t)heights_w * heights_h * (int64_t)sizeof(float))
        {
          heights_norm.resize(heights_w * heights_h);
          std::memcpy(heights_norm.ptrw(), bytes.ptr(), heights_w * heights_h * sizeof(float));
        }
      }
    }
  }
  else
  {
    height_texture = _create_height_texture(matrix);
    if (height_texture.is_null())
      return;
  }

  _rebuild_chunks();
}

Ref<ShaderMaterial> TerrainLOD::_get_lod_material(const Ref<ShaderMaterial> &base, int lod)
{
  if (lod == 0)
    return base;
  while ((int)lod_materials.size() < lod)
  {
    int next_lod = (int)lod_materials.size() + 1;
    Ref<ShaderMaterial> clone = Ref<ShaderMaterial>(Object::cast_to<ShaderMaterial>(base->duplicate().ptr()));
    if (clone.is_null())
      return base;
    clone->set_shader_parameter("height_lod", (float)next_lod);
    lod_materials.push_back(clone);
  }
  return lod_materials[lod - 1];
}

void TerrainLOD::_rebuild_chunks()
{
  clear_terrain();
  lod_materials.clear();
  base_material.unref();

  if (height_texture.is_null())
    return;

  // Get the DrawMatrix3D node and pull its material and amplitude
  Node *dm_node = get_node_or_null(draw_matrix_3d_path);
  if (dm_node == nullptr)
    return;

  Ref<ShaderMaterial> material = Object::cast_to<Object>(dm_node)->get("terrain_material");
  if (material.is_null())
    return;
  base_material = material;

  float amplitude = dm_node->get("amplitude");
  current_amplitude = amplitude;
  terrain_size = dm_node->get("terrain_size");
  if (terrain_size <= 0.0f)
    terrain_size = 500.0f;

  material->set_shader_parameter("height_texture", height_texture);
  int tex_w = heights_w > 0 ? heights_w : (int)height_texture->get_width();
  int tex_h = heights_h > 0 ? heights_h : (int)height_texture->get_height();
  if (tex_w > 0 && tex_h > 0)
  {
    material->set_shader_parameter("texel_size", Vector2(1.0f / (float)tex_w, 1.0f / (float)tex_h));
  }

  float chunk_world_size = terrain_size / (float)chunk_count;
  float base_dist = chunk_world_size * 2.0f * lod_distance_multiplier;
  float half_terrain = terrain_size * 0.5f;

  chunks.reserve((size_t)chunk_count * chunk_count);

  for (int cz = 0; cz < chunk_count; cz++)
  {
    for (int cx = 0; cx < chunk_count; cx++)
    {
      Node3D *chunk_parent = memnew(Node3D);
      float pos_x = ((float)cx + 0.5f) * chunk_world_size - half_terrain;
      float pos_z = ((float)cz + 0.5f) * chunk_world_size - half_terrain;
      chunk_parent->set_position(Vector3(pos_x, 0.0f, pos_z));
      add_child(chunk_parent);

      ChunkData chunk;
      chunk.parent = chunk_parent;
      chunk.cx = cx;
      chunk.cz = cz;
      chunks.push_back(chunk);

      for (int lod = 0; lod < lod_levels; lod++)
      {
        int subs = lod0_subdivisions >> lod;
        if (subs < 2)
          subs = 2;

        Ref<ArrayMesh> mesh = _build_chunk_mesh(cx, cz, subs, amplitude);
        if (mesh.is_null())
          continue;

        MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
        mesh_instance->set_mesh(mesh);
        mesh_instance->set_material_override(_get_lod_material(material, lod));

        float range_begin = 0.0f;
        float range_end = 0.0f;
        bool is_first = (lod == 0);
        bool is_last = (lod == lod_levels - 1);

        if (is_first)
        {
          range_begin = 0.0f;
          range_end = base_dist;
        }
        else if (is_last)
        {
          range_begin = base_dist * (float)(2 * lod - 1);
          range_end = 0.0f;
        }
        else
        {
          range_begin = base_dist * (float)(2 * lod - 1);
          range_end = base_dist * (float)(2 * lod + 1);
        }

        mesh_instance->set_visibility_range_begin(range_begin);
        mesh_instance->set_visibility_range_end(range_end);
        mesh_instance->set_visibility_range_fade_mode(GeometryInstance3D::VISIBILITY_RANGE_FADE_DISABLED);

        chunk_parent->add_child(mesh_instance);
      }
    }
  }
}

// Direct ArrayMesh build — SurfaceTool's add_vertex / set_uv / set_normal go
// through Variant-bound calls per vertex, so a 128x128 chunk pays ~50k method
// dispatches. Filling PackedVector3Array / PackedVector2Array / PackedInt32Array
// directly is the same data with one bound call total. Skip tangents — the
// shader sets NORMAL itself and Godot can derive TBN screen-space for the
// detail NORMAL_MAP sample.
Ref<ArrayMesh> TerrainLOD::_build_chunk_mesh(int chunk_x, int chunk_z, int subdivisions, float amplitude)
{
  float chunk_world_size = terrain_size / (float)chunk_count;
  float u_start = (float)chunk_x / (float)chunk_count;
  float v_start = (float)chunk_z / (float)chunk_count;
  float uv_range = 1.0f / (float)chunk_count;

  float half_chunk = chunk_world_size * 0.5f;
  int verts_per_side = subdivisions + 1;
  float step = chunk_world_size / (float)subdivisions;

  int vert_count = verts_per_side * verts_per_side;
  int tri_index_count = subdivisions * subdivisions * 6;

  PackedVector3Array verts;
  PackedVector3Array normals;
  PackedVector2Array uvs;
  PackedInt32Array indices;
  verts.resize(vert_count);
  normals.resize(vert_count);
  uvs.resize(vert_count);
  indices.resize(tri_index_count);

  Vector3 *vp = verts.ptrw();
  Vector3 *np = normals.ptrw();
  Vector2 *up = uvs.ptrw();
  int32_t *ip = indices.ptrw();

  // Vertices: flat plane (height comes from vertex shader sampling the
  // heightmap). Normal is a placeholder; the vertex shader overwrites it
  // with the heightmap-derived normal.
  Vector3 default_normal(0.0f, 1.0f, 0.0f);
  for (int z = 0; z < verts_per_side; z++)
  {
    float local_z = -half_chunk + (float)z * step;
    float v = v_start + ((float)z / (float)subdivisions) * uv_range;
    int row = z * verts_per_side;
    for (int x = 0; x < verts_per_side; x++)
    {
      float local_x = -half_chunk + (float)x * step;
      float u = u_start + ((float)x / (float)subdivisions) * uv_range;
      vp[row + x] = Vector3(local_x, 0.0f, local_z);
      np[row + x] = default_normal;
      up[row + x] = Vector2(u, v);
    }
  }

  for (int z = 0; z < subdivisions; z++)
  {
    for (int x = 0; x < subdivisions; x++)
    {
      int tl = z * verts_per_side + x;
      int tr = tl + 1;
      int bl = (z + 1) * verts_per_side + x;
      int br = bl + 1;
      int o = (z * subdivisions + x) * 6;
      ip[o + 0] = tl;
      ip[o + 1] = tr;
      ip[o + 2] = bl;
      ip[o + 3] = tr;
      ip[o + 4] = br;
      ip[o + 5] = bl;
    }
  }

  Array arrays;
  arrays.resize(Mesh::ARRAY_MAX);
  arrays[Mesh::ARRAY_VERTEX] = verts;
  arrays[Mesh::ARRAY_NORMAL] = normals;
  arrays[Mesh::ARRAY_TEX_UV] = uvs;
  arrays[Mesh::ARRAY_INDEX] = indices;

  Ref<ArrayMesh> mesh;
  mesh.instantiate();
  mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

  // Custom AABB so frustum culling accounts for vertex-shader displacement.
  float hh = chunk_world_size * 0.5f;
  AABB custom_aabb(
      Vector3(-hh, 0.0f, -hh),
      Vector3(chunk_world_size, amplitude, chunk_world_size));
  mesh->set_custom_aabb(custom_aabb);

  return mesh;
}

void TerrainLOD::_process(double delta)
{
  if (Engine::get_singleton()->is_editor_hint())
    return;
  if (!collision_enabled)
    return;
  if (chunks.empty())
    return;
  if (player_path.is_empty())
    return;

  collision_timer += (float)delta;
  if (collision_timer < collision_update_interval)
    return;
  collision_timer = 0.0f;

  _update_colliders();
}

void TerrainLOD::_update_colliders()
{
  Node *player_node = get_node_or_null(player_path);
  if (player_node == nullptr)
    return;
  Node3D *player3d = Object::cast_to<Node3D>(player_node);
  if (player3d == nullptr)
    return;

  Vector3 player_world = player3d->get_global_position();
  Vector3 player_local = get_global_transform().affine_inverse().xform(player_world);

  float activate_r = collision_radius;
  float deactivate_r = collision_radius * 1.2f;
  float act_sq = activate_r * activate_r;
  float deact_sq = deactivate_r * deactivate_r;

  for (auto &chunk : chunks)
  {
    float d2 = _chunk_distance_sq_xz(chunk, player_local);
    if (chunk.collider_active)
    {
      if (d2 > deact_sq)
        _free_chunk_collision(chunk);
    }
    else
    {
      if (d2 < act_sq)
        _build_chunk_collision(chunk);
    }
  }
}

float TerrainLOD::_chunk_distance_sq_xz(const ChunkData &chunk, const Vector3 &p) const
{
  float chunk_world_size = terrain_size / (float)chunk_count;
  float h = chunk_world_size * 0.5f;
  Vector3 cp = chunk.parent->get_position();
  float closest_x = std::clamp(p.x, cp.x - h, cp.x + h);
  float closest_z = std::clamp(p.z, cp.z - h, cp.z + h);
  float dx = p.x - closest_x;
  float dz = p.z - closest_z;
  return dx * dx + dz * dz;
}

float TerrainLOD::_sample_height_norm(float u, float v) const
{
  if (heights_w <= 0 || heights_h <= 0 || heights_norm.is_empty())
    return 0.0f;
  const float *hp = heights_norm.ptr();
  float fx = std::clamp(u, 0.0f, 1.0f) * (float)(heights_w - 1);
  float fz = std::clamp(v, 0.0f, 1.0f) * (float)(heights_h - 1);
  int x0 = (int)std::floor(fx);
  int z0 = (int)std::floor(fz);
  int x1 = std::min(x0 + 1, heights_w - 1);
  int z1 = std::min(z0 + 1, heights_h - 1);
  float tx = fx - (float)x0;
  float tz = fz - (float)z0;

  float h00 = hp[z0 * heights_w + x0];
  float h10 = hp[z0 * heights_w + x1];
  float h01 = hp[z1 * heights_w + x0];
  float h11 = hp[z1 * heights_w + x1];

  float h0 = h00 * (1.0f - tx) + h10 * tx;
  float h1 = h01 * (1.0f - tx) + h11 * tx;
  return h0 * (1.0f - tz) + h1 * tz;
}

void TerrainLOD::_build_chunk_collision(ChunkData &chunk)
{
  if (chunk.collider_active)
    return;
  if (heights_norm.is_empty() || heights_w <= 0 || heights_h <= 0)
    return;

  int subs = std::max(2, collision_subdivisions);
  int verts_per_side = subs + 1;
  float chunk_world_size = terrain_size / (float)chunk_count;
  float half_chunk = chunk_world_size * 0.5f;
  float step = chunk_world_size / (float)subs;

  float u_start = (float)chunk.cx / (float)chunk_count;
  float v_start = (float)chunk.cz / (float)chunk_count;
  float uv_range = 1.0f / (float)chunk_count;

  // Sample displaced vertex positions from the flat float buffer; matches
  // shader's texture(height_texture, UV).r * amplitude with bilinear filter.
  std::vector<Vector3> verts;
  verts.reserve((size_t)verts_per_side * verts_per_side);
  for (int z = 0; z < verts_per_side; z++)
  {
    for (int x = 0; x < verts_per_side; x++)
    {
      float local_x = -half_chunk + (float)x * step;
      float local_z = -half_chunk + (float)z * step;
      float u = u_start + ((float)x / (float)subs) * uv_range;
      float v = v_start + ((float)z / (float)subs) * uv_range;
      float height = _sample_height_norm(u, v) * current_amplitude;
      verts.push_back(Vector3(local_x, height, local_z));
    }
  }

  PackedVector3Array faces;
  faces.resize(subs * subs * 6);
  Vector3 *fp = faces.ptrw();
  int idx = 0;
  for (int z = 0; z < subs; z++)
  {
    for (int x = 0; x < subs; x++)
    {
      int tl = z * verts_per_side + x;
      int tr = tl + 1;
      int bl = (z + 1) * verts_per_side + x;
      int br = bl + 1;
      fp[idx++] = verts[tl];
      fp[idx++] = verts[tr];
      fp[idx++] = verts[bl];
      fp[idx++] = verts[tr];
      fp[idx++] = verts[br];
      fp[idx++] = verts[bl];
    }
  }

  Ref<ConcavePolygonShape3D> shape;
  shape.instantiate();
  shape->set_faces(faces);

  StaticBody3D *body = memnew(StaticBody3D);
  CollisionShape3D *cshape = memnew(CollisionShape3D);
  cshape->set_shape(shape);
  body->add_child(cshape);
  chunk.parent->add_child(body);

  chunk.body = body;
  chunk.shape_node = cshape;
  chunk.shape = shape;
  chunk.collider_active = true;
}

void TerrainLOD::_free_chunk_collision(ChunkData &chunk)
{
  if (!chunk.collider_active)
    return;
  if (chunk.body != nullptr)
    chunk.body->queue_free();
  chunk.body = nullptr;
  chunk.shape_node = nullptr;
  chunk.shape.unref();
  chunk.collider_active = false;
}
