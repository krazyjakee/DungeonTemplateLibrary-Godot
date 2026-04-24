#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <vector>

namespace godot
{
  class TerrainLOD : public Node3D
  {
    GDCLASS(TerrainLOD, Node3D);

  protected:
    static void _bind_methods();

  public:
    TerrainLOD();
    ~TerrainLOD();

    void generate(Array matrix);
    void clear_terrain();

    void _process(double delta) override;

    // Property getters/setters
    void set_terrain_size(float p_size);
    float get_terrain_size() const;

    void set_chunk_count(int p_count);
    int get_chunk_count() const;

    void set_lod_levels(int p_levels);
    int get_lod_levels() const;

    void set_lod0_subdivisions(int p_subs);
    int get_lod0_subdivisions() const;

    void set_lod_distance_multiplier(float p_mult);
    float get_lod_distance_multiplier() const;

    void set_draw_matrix_3d(NodePath p_path);
    NodePath get_draw_matrix_3d() const;

    void set_collision_enabled(bool p_enabled);
    bool is_collision_enabled() const;

    void set_collision_radius(float p_radius);
    float get_collision_radius() const;

    void set_collision_subdivisions(int p_subs);
    int get_collision_subdivisions() const;

    void set_collision_update_interval(float p_interval);
    float get_collision_update_interval() const;

    void set_player_path(NodePath p_path);
    NodePath get_player_path() const;

  private:
    struct ChunkData
    {
      Node3D *parent = nullptr;
      int cx = 0;
      int cz = 0;
      StaticBody3D *body = nullptr;
      CollisionShape3D *shape_node = nullptr;
      Ref<ConcavePolygonShape3D> shape;
      bool collider_active = false;
    };

    Ref<ArrayMesh> _build_chunk_mesh(int chunk_x, int chunk_z, int subdivisions, float amplitude);
    Ref<ImageTexture> _create_height_texture(const Array &matrix);
    void _rebuild_chunks();
    void _update_colliders();
    void _build_chunk_collision(ChunkData &chunk);
    void _free_chunk_collision(ChunkData &chunk);
    float _chunk_distance_sq_xz(const ChunkData &chunk, const Vector3 &p) const;
    float _sample_height_bilinear(float u, float v) const;

    float terrain_size = 500.0f;
    int chunk_count = 8;
    int lod_levels = 3;
    int lod0_subdivisions = 64;
    float lod_distance_multiplier = 1.0f;
    NodePath draw_matrix_3d_path;
    Ref<ImageTexture> height_texture;
    Ref<Image> height_image;
    float current_amplitude = 0.0f;

    bool collision_enabled = true;
    float collision_radius = 50.0f;
    int collision_subdivisions = 32;
    float collision_update_interval = 0.2f;
    float collision_timer = 0.0f;
    NodePath player_path;

    std::vector<ChunkData> chunks;
  };
}
