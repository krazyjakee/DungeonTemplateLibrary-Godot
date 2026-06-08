#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <DTL/Shape/CellularAutomatonMixIsland.hpp>

namespace godot
{
  // All shape generators return Dictionary { "width": int, "height": int,
  // "data": PackedByteArray } — a flat byte buffer indexed as data[y * w + x].
  // PackedByteArray gives O(1) cell access from GDScript and zero per-cell
  // Variant marshalling when crossing into C++ consumers like TerrainBuilder.
  class DTL : public RefCounted
  {
    GDCLASS(DTL, RefCounted);

  protected:
    static void _bind_methods();

  public:
    DTL();
    ~DTL();

    Dictionary CellularAutomatonMixIsland(int width, int height, int iterations = 5, int land_values = 5);
    Dictionary CellularAutomatonIsland(int width, int height, int iterations = 5, float probability = 0.4);
    Dictionary FractalLoopIsland(int width, int height, int min_value = 10, int altitude = 150, int add_altitude = 70);
    Dictionary FractalIsland(int width, int height, int min_value = 10, int altitude = 150, int add_altitude = 75);
    Dictionary DiamondSquareAverageIsland(int width, int height, int min_value = 0, int altitude = 80, int add_altitude = 60);
    Dictionary DiamondSquareAverageCornerIsland(int width, int height, int min_value = 20, int altitude = 80, int add_altitude = 60);
    Dictionary SimpleVoronoiIsland(int width, int height, float voronoi_num = 40.0, float probability = 0.5);
    Dictionary PerlinIsland(int width, int height, float frequency = 10.0, int octaves = 6, int max_height = 200, int min_height = 200);
    Dictionary PerlinLoopIsland(int width, int height, float frequency = 10.0, int octaves = 6, int max_height = 200, int min_height = 200);
    Dictionary PerlinSolitaryIsland(int width, int height, float truncated_proportion_ = 0.5, float mountain_proportion_ = 0.45, float frequency = 6.0, int octaves = 6, int max_height = 200, int min_height = 200);
    Dictionary RogueLike(int width, int height, int max_ways = 20, int min_room_width = 3, int max_room_width = 3, int min_room_height = 4, int max_room_height = 4, int min_way_horizontal = 3, int max_way_horizontal = 4, int min_way_vertical = 3, int max_way_vertical = 4);
    Dictionary SimpleRogueLike(int width, int height, int division_min = 3, int division_max = 4, int room_min_x = 5, int room_max_x = 6, int room_min_y = 7, int room_max_y = 8);
    Dictionary MazeDig(int width, int height);
    Dictionary MazeBar(int width, int height);
    Dictionary ClusteringMaze(int width, int height);
    int _seed = 0;
    void SetSeed(int seed = 0);
  };
}
