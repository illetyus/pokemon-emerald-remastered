extends SceneTree


func _init() -> void:
    var core = ClassDB.instantiate("RemasterCoreBridge")
    assert(core != null)

    core.reset()
    var events: int = core.step(&"MOVE_UP")
    assert(events == 16)

    var state: Dictionary = core.snapshot()
    assert(int(state.tile_x) == 1)
    assert(int(state.tile_y) == 1)
    assert(int(state.step_count) == 0)

    core.step(&"MOVE_RIGHT")
    core.step(&"MOVE_RIGHT")
    events = core.step(&"INTERACT")
    assert(events == 6)
    core.step(&"MOVE_DOWN")
    core.step(&"MOVE_DOWN")
    events = core.step(&"MOVE_DOWN")
    assert(events == 9)

    state = core.snapshot()
    assert(int(state.tile_x) == 3)
    assert(int(state.tile_y) == 4)
    assert(int(state.step_count) == 5)
    assert(int(state.interaction_count) == 1)
    assert(int(state.event_flags) == 1)
    assert(bool(state.encounter_pending) == true)

    var hash_before: int = core.state_hash()
    assert(hash_before == 7218695048241891488)
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

    # Validate the actual presentation script's persistent file wrapper.
    var main_script = load("res://main.gd")
    assert(main_script != null)

    var main_node = main_script.new()
    root.add_child(main_node)
    assert(main_node.core != null)

    main_node.core.reset()
    main_node.core.step(&"MOVE_RIGHT")
    main_node.core.step(&"MOVE_RIGHT")
    var persistent_hash: int = main_node.core.state_hash()

    assert(main_node.persist_state_to_disk())
    main_node.core.reset()
    assert(main_node.core.state_hash() != persistent_hash)

    assert(main_node.load_persistent_state())
    assert(main_node.core.state_hash() == persistent_hash)

    print("R0 Godot+C bridge and persistent-state tests passed")
    quit(0)
