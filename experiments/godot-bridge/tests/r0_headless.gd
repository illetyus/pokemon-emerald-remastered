extends SceneTree


func _init() -> void:
    var core := RemasterCoreBridge.new()

    core.reset()
    core.step(&"MOVE_UP")

    var state: Dictionary = core.snapshot()
    assert(int(state.tile_x) == 1)
    assert(int(state.tile_y) == 1)
    assert(int(state.step_count) == 0)

    core.step(&"MOVE_RIGHT")
    core.step(&"MOVE_RIGHT")
    core.step(&"INTERACT")
    core.step(&"MOVE_DOWN")
    core.step(&"MOVE_DOWN")
    core.step(&"MOVE_DOWN")

    state = core.snapshot()
    assert(int(state.tile_x) == 3)
    assert(int(state.tile_y) == 4)
    assert(int(state.step_count) == 5)
    assert(int(state.interaction_count) == 1)
    assert(int(state.event_flags) == 1)
    assert(bool(state.encounter_pending) == true)

    var hash_before: int = core.state_hash()
    var save_data: PackedByteArray = core.save_state()
    assert(save_data.size() > 0)

    core.reset()
    assert(core.load_state(save_data))

    state = core.snapshot()
    assert(int(state.tile_x) == 3)
    assert(int(state.tile_y) == 4)
    assert(core.state_hash() == hash_before)

    core.step(&"MOVE_RIGHT")
    state = core.snapshot()
    assert(int(state.tile_x) == 4)
    assert(int(state.tile_y) == 4)
    assert(int(state.step_count) == 6)

    print("R0 Godot+C bridge test passed")
    quit(0)
