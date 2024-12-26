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
  };
}