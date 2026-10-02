#include "remaster_core_bridge.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

RemasterCoreBridge::RemasterCoreBridge()
{
    remaster_core_init(&state);
}

void RemasterCoreBridge::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("reset"), &RemasterCoreBridge::reset);
    ClassDB::bind_method(D_METHOD("step", "action"), &RemasterCoreBridge::step);
    ClassDB::bind_method(D_METHOD("snapshot"), &RemasterCoreBridge::snapshot);
    ClassDB::bind_method(D_METHOD("save_state"), &RemasterCoreBridge::save_state);
    ClassDB::bind_method(D_METHOD("load_state", "data"), &RemasterCoreBridge::load_state);
    ClassDB::bind_method(D_METHOD("state_hash"), &RemasterCoreBridge::state_hash);
}

void RemasterCoreBridge::reset()
{
    remaster_core_init(&state);
}

void RemasterCoreBridge::step(const StringName &action)
{
    RemasterInput input = REMASTER_INPUT_NONE;

    if (action == StringName("MOVE_UP"))
        input = REMASTER_INPUT_MOVE_UP;
    else if (action == StringName("MOVE_DOWN"))
        input = REMASTER_INPUT_MOVE_DOWN;
    else if (action == StringName("MOVE_LEFT"))
        input = REMASTER_INPUT_MOVE_LEFT;
    else if (action == StringName("MOVE_RIGHT"))
        input = REMASTER_INPUT_MOVE_RIGHT;
    else if (action == StringName("INTERACT"))
        input = REMASTER_INPUT_INTERACT;

    if (input != REMASTER_INPUT_NONE)
        remaster_core_step(&state, input);
}

Dictionary RemasterCoreBridge::snapshot() const
{
    Dictionary result;

    result["tile_x"] = state.tile_x;
    result["tile_y"] = state.tile_y;
    result["step_count"] = state.step_count;
    result["interaction_count"] = state.interaction_count;
    result["event_flags"] = state.event_flags;
    result["encounter_pending"] = state.encounter_pending != 0;

    return result;
}

PackedByteArray RemasterCoreBridge::save_state() const
{
    PackedByteArray result;
    const size_t size = remaster_core_state_size();

    result.resize(static_cast<int64_t>(size));

    if (!remaster_core_save(&state, result.ptrw(), size))
        result.clear();

    return result;
}

bool RemasterCoreBridge::load_state(const PackedByteArray &data)
{
    const size_t expected = remaster_core_state_size();

    if (static_cast<size_t>(data.size()) != expected)
        return false;

    return remaster_core_load(&state, data.ptr(), expected) != 0;
}

int64_t RemasterCoreBridge::state_hash() const
{
    return static_cast<int64_t>(remaster_core_state_hash(&state));
}

} // namespace godot
