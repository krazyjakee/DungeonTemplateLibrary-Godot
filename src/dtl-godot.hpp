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
  };
}