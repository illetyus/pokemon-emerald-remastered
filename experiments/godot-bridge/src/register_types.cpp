#include "remaster_core_bridge.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static void initialize_remaster_bridge(ModuleInitializationLevel level)
{
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE)
        return;

    ClassDB::register_class<RemasterCoreBridge>();
}

static void uninitialize_remaster_bridge(ModuleInitializationLevel level)
{
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE)
        return;
}

extern "C" {

GDExtensionBool GDE_EXPORT remaster_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    const GDExtensionClassLibraryPtr library,
    GDExtensionInitialization *initialization)
{
    GDExtensionBinding::InitObject init_obj(
        get_proc_address,
        library,
        initialization
    );

    init_obj.register_initializer(initialize_remaster_bridge);
    init_obj.register_terminator(uninitialize_remaster_bridge);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}

}
