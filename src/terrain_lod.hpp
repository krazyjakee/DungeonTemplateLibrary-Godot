#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/node_path.hpp>

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

  private:
    Ref<ArrayMesh> _build_chunk_mesh(int chunk_x, int chunk_z, int subdivisions, float amplitude);
    Ref<ImageTexture> _create_height_texture(const Array &matrix);
    void _rebuild_chunks();

    float terrain_size = 500.0f;
    int chunk_count = 8;
    int lod_levels = 3;
    int lod0_subdivisions = 64;
    float lod_distance_multiplier = 1.0f;
    NodePath draw_matrix_3d_path;
    Ref<ImageTexture> height_texture;
  };
}
