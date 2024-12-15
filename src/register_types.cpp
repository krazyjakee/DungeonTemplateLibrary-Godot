#include "register_types.hpp"

// Include your DTL header here
#include "DTL.hpp"
#include "dtl-godot.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
  {
    return;
  }

  // Register your DTL class
  GDREGISTER_CLASS(DTL);
}

void uninitialize_module(ModuleInitializationLevel p_level)
{
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
  {
    return;
  }
}

extern "C"
{
  GDExtensionBool GDE_EXPORT gdextension_entry(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                               const GDExtensionClassLibraryPtr p_library,
                                               GDExtensionInitialization *r_initialization)
  {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_module);
    init_obj.register_terminator(uninitialize_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
  }
}
