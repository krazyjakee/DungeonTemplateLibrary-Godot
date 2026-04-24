#include "terrain_lod.hpp"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include <algorithm>
#include <cmath>

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

  ClassDB::bind_method(D_METHOD("set_terrain_size", "size"), &TerrainLOD::set_terrain_size);
  ClassDB::bind_method(D_METHOD("get_terrain_size"), &TerrainLOD::get_terrain_size);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "terrain_size"), "set_terrain_size", "get_terrain_size");

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
void TerrainLOD::set_terrain_size(float p_size) { terrain_size = p_size; }
float TerrainLOD::get_terrain_size() const { return terrain_size; }

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

Ref<ImageTexture> TerrainLOD::_create_height_texture(const Array &matrix)
{
  int map_height = matrix.size();
  if (map_height == 0)
    return Ref<ImageTexture>();

  Array first_row = matrix[0];
  int map_width = first_row.size();
  if (map_width == 0)
    return Ref<ImageTexture>();

  // Find min/max values
  int highest_value = -999999;
  int lowest_value = 999999;

  for (int y = 0; y < map_height; y++)
  {
    Array row = matrix[y];
    for (int x = 0; x < map_width; x++)
    {
      int cell = row[x];
      if (cell < lowest_value)
        lowest_value = cell;
      if (cell > highest_value)
        highest_value = cell;
    }
  }

  int range = highest_value - lowest_value;
  if (range == 0)
    range = 1;

  // Create image
  Ref<Image> image = Image::create_empty(map_width, map_height, false, Image::FORMAT_RGB8);

  for (int y = 0; y < map_height; y++)
  {
    Array row = matrix[y];
    for (int x = 0; x < map_width; x++)
    {
      int cell = row[x];
      float normalized = static_cast<float>(cell - lowest_value) / static_cast<float>(range);
      image->set_pixel(x, y, Color(normalized, normalized, normalized));
    }
  }

  return ImageTexture::create_from_image(image);
}

void TerrainLOD::generate(Array matrix)
{
  if (matrix.size() == 0)
    return;

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

  height_texture = _create_height_texture(matrix);
  if (height_texture.is_null())
    return;

  height_image = height_texture->get_image();

  _rebuild_chunks();
}

void TerrainLOD::_rebuild_chunks()
{
  clear_terrain();

  if (height_texture.is_null())
    return;

  // Get the DrawMatrix3D node and pull its material and amplitude
  Node *dm_node = get_node_or_null(draw_matrix_3d_path);
  if (dm_node == nullptr)
    return;

  Ref<ShaderMaterial> material = Object::cast_to<Object>(dm_node)->get("terrain_material");
  if (material.is_null())
    return;

  float amplitude = dm_node->get("amplitude");
  current_amplitude = amplitude;

  // Set the height texture on the shared material
  material->set_shader_parameter("height_texture", height_texture);

  float chunk_world_size = terrain_size / static_cast<float>(chunk_count);
  float base_dist = chunk_world_size * 2.0f * lod_distance_multiplier;
  float half_terrain = terrain_size * 0.5f;
  // Margin: diagonal half-extent of chunk footprint for range overlap
  float margin = chunk_world_size * 0.7071f; // sqrt(2)/2

  chunks.reserve(static_cast<size_t>(chunk_count) * chunk_count);

  for (int cz = 0; cz < chunk_count; cz++)
  {
    for (int cx = 0; cx < chunk_count; cx++)
    {
      // Chunk parent node positioned at center of this chunk
      Node3D *chunk_parent = memnew(Node3D);
      float pos_x = (static_cast<float>(cx) + 0.5f) * chunk_world_size - half_terrain;
      float pos_z = (static_cast<float>(cz) + 0.5f) * chunk_world_size - half_terrain;
      chunk_parent->set_position(Vector3(pos_x, 0.0f, pos_z));
      add_child(chunk_parent);

      ChunkData chunk;
      chunk.parent = chunk_parent;
      chunk.cx = cx;
      chunk.cz = cz;
      chunks.push_back(chunk);

      for (int lod = 0; lod < lod_levels; lod++)
      {
        // Calculate subdivisions for this LOD level (halving each time, min 2)
        int subs = lod0_subdivisions >> lod;
        if (subs < 2)
          subs = 2;

        Ref<ArrayMesh> mesh = _build_chunk_mesh(cx, cz, subs, amplitude);
        if (mesh.is_null())
          continue;

        MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
        mesh_instance->set_mesh(mesh);
        mesh_instance->set_material_override(material);

        // Visibility range distances with margin so chunks don't disappear
        // while their edges are still on screen
        float range_begin = 0.0f;
        float range_end = 0.0f;

        if (lod == 0)
        {
          range_begin = 0.0f;
          range_end = base_dist + margin;
        }
        else if (lod == lod_levels - 1)
        {
          float raw_begin = base_dist * static_cast<float>(2 * lod - 1);
          range_begin = std::max(0.0f, raw_begin - margin);
          range_end = 0.0f; // 0 means infinite
        }
        else
        {
          float raw_begin = base_dist * static_cast<float>(2 * lod - 1);
          float raw_end = base_dist * static_cast<float>(2 * lod + 1);
          range_begin = std::max(0.0f, raw_begin - margin);
          range_end = raw_end + margin;
        }

        mesh_instance->set_visibility_range_begin(range_begin);
        mesh_instance->set_visibility_range_end(range_end);
        mesh_instance->set_visibility_range_fade_mode(GeometryInstance3D::VISIBILITY_RANGE_FADE_SELF);

        chunk_parent->add_child(mesh_instance);
      }
    }
  }
}

Ref<ArrayMesh> TerrainLOD::_build_chunk_mesh(int chunk_x, int chunk_z, int subdivisions, float amplitude)
{
  Ref<SurfaceTool> st;
  st.instantiate();

  st->begin(Mesh::PRIMITIVE_TRIANGLES);

  float chunk_world_size = terrain_size / static_cast<float>(chunk_count);

  // UV sub-region for this chunk within the full heightmap
  float u_start = static_cast<float>(chunk_x) / static_cast<float>(chunk_count);
  float v_start = static_cast<float>(chunk_z) / static_cast<float>(chunk_count);
  float uv_range = 1.0f / static_cast<float>(chunk_count);

  float half_chunk = chunk_world_size * 0.5f;
  int verts_per_side = subdivisions + 1;
  float step = chunk_world_size / static_cast<float>(subdivisions);

  // Generate vertices
  for (int z = 0; z < verts_per_side; z++)
  {
    for (int x = 0; x < verts_per_side; x++)
    {
      float local_x = -half_chunk + static_cast<float>(x) * step;
      float local_z = -half_chunk + static_cast<float>(z) * step;

      float u = u_start + (static_cast<float>(x) / static_cast<float>(subdivisions)) * uv_range;
      float v = v_start + (static_cast<float>(z) / static_cast<float>(subdivisions)) * uv_range;

      st->set_uv(Vector2(u, v));
      st->set_normal(Vector3(0.0f, 1.0f, 0.0f));
      st->add_vertex(Vector3(local_x, 0.0f, local_z));
    }
  }

  // Generate triangle indices
  for (int z = 0; z < subdivisions; z++)
  {
    for (int x = 0; x < subdivisions; x++)
    {
      int top_left = z * verts_per_side + x;
      int top_right = top_left + 1;
      int bottom_left = (z + 1) * verts_per_side + x;
      int bottom_right = bottom_left + 1;

      // First triangle (clockwise from above)
      st->add_index(top_left);
      st->add_index(top_right);
      st->add_index(bottom_left);

      // Second triangle
      st->add_index(top_right);
      st->add_index(bottom_right);
      st->add_index(bottom_left);
    }
  }

  st->generate_normals();
  Ref<ArrayMesh> mesh = st->commit();

  // Set custom AABB to account for vertex shader height displacement.
  float half_chunk_size = chunk_world_size * 0.5f;
  AABB custom_aabb(
      Vector3(-half_chunk_size, 0.0f, -half_chunk_size),
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

  collision_timer += static_cast<float>(delta);
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

  // Compare in TerrainLOD-local space (chunk parents are positioned in local space).
  Vector3 player_world = player3d->get_global_position();
  Vector3 player_local = get_global_transform().affine_inverse().xform(player_world);

  float activate_r = collision_radius;
  float deactivate_r = collision_radius * 1.2f; // hysteresis margin
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
  float chunk_world_size = terrain_size / static_cast<float>(chunk_count);
  float h = chunk_world_size * 0.5f;
  Vector3 cp = chunk.parent->get_position();
  float closest_x = std::clamp(p.x, cp.x - h, cp.x + h);
  float closest_z = std::clamp(p.z, cp.z - h, cp.z + h);
  float dx = p.x - closest_x;
  float dz = p.z - closest_z;
  return dx * dx + dz * dz;
}

float TerrainLOD::_sample_height_bilinear(float u, float v) const
{
  if (height_image.is_null())
    return 0.0f;

  int w = height_image->get_width();
  int h = height_image->get_height();
  if (w <= 0 || h <= 0)
    return 0.0f;

  float fx = std::clamp(u, 0.0f, 1.0f) * static_cast<float>(w - 1);
  float fz = std::clamp(v, 0.0f, 1.0f) * static_cast<float>(h - 1);
  int x0 = static_cast<int>(std::floor(fx));
  int z0 = static_cast<int>(std::floor(fz));
  int x1 = std::min(x0 + 1, w - 1);
  int z1 = std::min(z0 + 1, h - 1);
  float tx = fx - static_cast<float>(x0);
  float tz = fz - static_cast<float>(z0);

  float h00 = height_image->get_pixel(x0, z0).r;
  float h10 = height_image->get_pixel(x1, z0).r;
  float h01 = height_image->get_pixel(x0, z1).r;
  float h11 = height_image->get_pixel(x1, z1).r;

  float h0 = h00 * (1.0f - tx) + h10 * tx;
  float h1 = h01 * (1.0f - tx) + h11 * tx;
  return h0 * (1.0f - tz) + h1 * tz;
}

void TerrainLOD::_build_chunk_collision(ChunkData &chunk)
{
  if (chunk.collider_active)
    return;
  if (height_image.is_null())
    return;

  int subs = std::max(2, collision_subdivisions);
  int verts_per_side = subs + 1;
  float chunk_world_size = terrain_size / static_cast<float>(chunk_count);
  float half_chunk = chunk_world_size * 0.5f;
  float step = chunk_world_size / static_cast<float>(subs);

  float u_start = static_cast<float>(chunk.cx) / static_cast<float>(chunk_count);
  float v_start = static_cast<float>(chunk.cz) / static_cast<float>(chunk_count);
  float uv_range = 1.0f / static_cast<float>(chunk_count);

  // Sample displaced vertex positions (matches shader's texture(height_texture, UV).r * amplitude)
  std::vector<Vector3> verts;
  verts.reserve(static_cast<size_t>(verts_per_side) * verts_per_side);
  for (int z = 0; z < verts_per_side; z++)
  {
    for (int x = 0; x < verts_per_side; x++)
    {
      float local_x = -half_chunk + static_cast<float>(x) * step;
      float local_z = -half_chunk + static_cast<float>(z) * step;
      float u = u_start + (static_cast<float>(x) / static_cast<float>(subs)) * uv_range;
      float v = v_start + (static_cast<float>(z) / static_cast<float>(subs)) * uv_range;
      float height = _sample_height_bilinear(u, v) * current_amplitude;
      verts.push_back(Vector3(local_x, height, local_z));
    }
  }

  // Build flat face array for ConcavePolygonShape3D (3 verts per triangle)
  PackedVector3Array faces;
  faces.resize(subs * subs * 6);
  int idx = 0;
  for (int z = 0; z < subs; z++)
  {
    for (int x = 0; x < subs; x++)
    {
      int tl = z * verts_per_side + x;
      int tr = tl + 1;
      int bl = (z + 1) * verts_per_side + x;
      int br = bl + 1;

      faces.set(idx++, verts[tl]);
      faces.set(idx++, verts[tr]);
      faces.set(idx++, verts[bl]);
      faces.set(idx++, verts[tr]);
      faces.set(idx++, verts[br]);
      faces.set(idx++, verts[bl]);
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
