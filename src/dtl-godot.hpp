#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <DTL/Shape/CellularAutomatonMixIsland.hpp>

namespace godot
{
  class DTL : public RefCounted
  {
    GDCLASS(DTL, RefCounted);

  protected:
    static void _bind_methods();

  public:
    DTL();
    ~DTL();

    Array CellularAutomatonMixIsland(int width, int height, int iterations = 5, int land_values = 5);
    Array CellularAutomatonIsland(int width, int height, int iterations = 5, float probability = 0.4);
    Array FractalLoopIsland(int width, int height, int min_value = 10, int altitude = 150, int add_altitude = 70);
    Array FractalIsland(int width, int height, int min_value = 10, int altitude = 150, int add_altitude = 75);
    Array DiamondSquareAverageIsland(int width, int height, int min_value = 0, int altitude = 80, int add_altitude = 60);
    Array DiamondSquareAverageCornerIsland(int width, int height, int min_value = 20, int altitude = 80, int add_altitude = 60);
    Array SimpleVoronoiIsland(int width, int height, float voronoi_num = 40.0, float probability = 0.5);
    Array PerlinIsland(int width, int height, float frequency = 10.0, int octaves = 6, int max_height = 200, int min_height = 200);
    Array PerlinLoopIsland(int width, int height, float frequency = 10.0, int octaves = 6, int max_height = 200, int min_height = 200);
    Array PerlinSolitaryIsland(int width, int height, float truncated_proportion_ = 0.5, float mountain_proportion_ = 0.45, float frequency = 6.0, int octaves = 6, int max_height = 200, int min_height = 200);
    Array RogueLike(int width, int height, int max_ways = 20, int min_room_width = 3, int max_room_width = 3, int min_room_height = 4, int max_room_height = 4, int min_way_horizontal = 3, int max_way_horizontal = 4, int min_way_vertical = 3, int max_way_vertical = 4);
    Array SimpleRogueLike(int width, int height, int division_min = 3, int division_max = 4, int room_min_x = 5, int room_max_x = 6, int room_min_y = 7, int room_max_y = 8);
    Array MazeDig(int width, int height);
    Array MazeBar(int width, int height);
    Array ClusteringMaze(int width, int height);
  };
}