extends SceneTree


func _init() -> void:
    var script := load("res://main.gd")
    var node = script.new()
    root.add_child(node)

    node.reset_state()

    var events: int = node.apply_action(&"MOVE_UP")
    assert(events == 16)
    assert(node.tile_x == 1)
    assert(node.tile_y == 1)
    assert(node.step_count == 0)

    node.apply_action(&"MOVE_RIGHT")
    node.apply_action(&"MOVE_RIGHT")
    events = node.apply_action(&"INTERACT")
    assert(events == 6)
    node.apply_action(&"MOVE_DOWN")
    node.apply_action(&"MOVE_DOWN")
    events = node.apply_action(&"MOVE_DOWN")
    assert(events == 9)

    assert(node.tile_x == 3)
    assert(node.tile_y == 4)
    assert(node.step_count == 5)
    assert(node.interaction_count == 1)
    assert(node.event_flags == 1)
    assert(node.encounter_pending == true)

    var saved: Dictionary = node.snapshot().duplicate(true)
    node.reset_state()
    node.load_snapshot(saved)

    assert(node.tile_x == 3)
    assert(node.tile_y == 4)
    assert(node.step_count == 5)
    assert(node.interaction_count == 1)
    assert(node.event_flags == 1)
    assert(node.encounter_pending == true)

    node.apply_action(&"MOVE_RIGHT")
    assert(node.tile_x == 4)
    assert(node.tile_y == 4)
    assert(node.step_count == 6)
    assert(node.encounter_pending == false)

    print("R0 pure Godot headless test passed")
    quit(0)
