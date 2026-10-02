extends Node2D

const MAP_WIDTH := 8
const MAP_HEIGHT := 8
const TILE_SIZE := 64.0
const MAP_ORIGIN := Vector2(64.0, 64.0)

const COLLISION := [
    [1,1,1,1,1,1,1,1],
    [1,0,0,0,0,0,0,1],
    [1,0,1,0,0,1,0,1],
    [1,0,1,0,0,1,0,1],
    [1,0,0,0,0,0,0,1],
    [1,0,1,1,0,0,0,1],
    [1,0,0,0,0,0,0,1],
    [1,1,1,1,1,1,1,1],
]

var tile_x := 1
var tile_y := 1
var step_count := 0
var interaction_count := 0
var event_flags := 0
var encounter_pending := false
var saved_state: Dictionary = {}


func _ready() -> void:
    reset_state()
    queue_redraw()


func reset_state() -> void:
    tile_x = 1
    tile_y = 1
    step_count = 0
    interaction_count = 0
    event_flags = 0
    encounter_pending = false
    queue_redraw()


func is_walkable(x: int, y: int) -> bool:
    if x < 0 or y < 0 or x >= MAP_WIDTH or y >= MAP_HEIGHT:
        return false
    return COLLISION[y][x] == 0


func apply_action(action: StringName) -> int:
    var next_x := tile_x
    var next_y := tile_y
    var events := 0
    encounter_pending = false

    match action:
        &"MOVE_UP":
            next_y -= 1
        &"MOVE_DOWN":
            next_y += 1
        &"MOVE_LEFT":
            next_x -= 1
        &"MOVE_RIGHT":
            next_x += 1
        &"INTERACT":
            interaction_count += 1
            events |= 2
            if tile_x == 3 and tile_y == 1 and (event_flags & 1) == 0:
                event_flags |= 1
                events |= 4
            queue_redraw()
            return events
        _:
            return events

    if not is_walkable(next_x, next_y):
        queue_redraw()
        return events | 16

    tile_x = next_x
    tile_y = next_y
    step_count += 1
    events |= 1
    encounter_pending = (step_count % 5) == 0
    if encounter_pending:
        events |= 8
    queue_redraw()
    return events


func snapshot() -> Dictionary:
    return {
        "tile_x": tile_x,
        "tile_y": tile_y,
        "step_count": step_count,
        "interaction_count": interaction_count,
        "event_flags": event_flags,
        "encounter_pending": encounter_pending,
    }


func load_snapshot(data: Dictionary) -> void:
    tile_x = int(data.get("tile_x", 1))
    tile_y = int(data.get("tile_y", 1))
    step_count = int(data.get("step_count", 0))
    interaction_count = int(data.get("interaction_count", 0))
    event_flags = int(data.get("event_flags", 0))
    encounter_pending = bool(data.get("encounter_pending", false))
    queue_redraw()


func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventKey and event.pressed and not event.echo:
        match event.keycode:
            KEY_ESCAPE:
                get_tree().quit()
            KEY_R:
                reset_state()
            KEY_F5:
                saved_state = snapshot().duplicate(true)
            KEY_F9:
                if not saved_state.is_empty():
                    load_snapshot(saved_state)
            KEY_UP, KEY_W:
                apply_action(&"MOVE_UP")
            KEY_DOWN, KEY_S:
                apply_action(&"MOVE_DOWN")
            KEY_LEFT, KEY_A:
                apply_action(&"MOVE_LEFT")
            KEY_RIGHT, KEY_D:
                apply_action(&"MOVE_RIGHT")
            KEY_SPACE, KEY_ENTER:
                apply_action(&"INTERACT")


    if event is InputEventJoypadButton and event.pressed:
        match event.button_index:
            JOY_BUTTON_DPAD_UP:
                apply_action(&"MOVE_UP")
            JOY_BUTTON_DPAD_DOWN:
                apply_action(&"MOVE_DOWN")
            JOY_BUTTON_DPAD_LEFT:
                apply_action(&"MOVE_LEFT")
            JOY_BUTTON_DPAD_RIGHT:
                apply_action(&"MOVE_RIGHT")
            JOY_BUTTON_A:
                apply_action(&"INTERACT")

    if event is InputEventScreenTouch and event.pressed:
        var normalized := event.position / get_viewport_rect().size
        if normalized.x > 0.70:
            apply_action(&"INTERACT")
        elif normalized.x < 0.35:
            if normalized.y < 0.35:
                apply_action(&"MOVE_UP")
            elif normalized.y > 0.65:
                apply_action(&"MOVE_DOWN")
            else:
                apply_action(&"MOVE_LEFT")
        elif normalized.x < 0.70:
            apply_action(&"MOVE_RIGHT")


func _draw() -> void:
    draw_rect(Rect2(Vector2.ZERO, get_viewport_rect().size), Color(0.086, 0.102, 0.125), true)

    for y in range(MAP_HEIGHT):
        for x in range(MAP_WIDTH):
            var rect := Rect2(
                MAP_ORIGIN + Vector2(x, y) * TILE_SIZE,
                Vector2(TILE_SIZE - 2.0, TILE_SIZE - 2.0)
            )
            var color := Color(0.227, 0.263, 0.298) if COLLISION[y][x] == 1 else Color(0.365, 0.604, 0.357)
            draw_rect(rect, color, true)

    var event_rect := Rect2(
        MAP_ORIGIN + Vector2(3, 1) * TILE_SIZE,
        Vector2(TILE_SIZE - 2.0, TILE_SIZE - 2.0)
    )
    draw_rect(
        event_rect,
        Color(0.929, 0.725, 0.294) if (event_flags & 1) != 0 else Color(0.710, 0.455, 0.208),
        true
    )

    var inset := 12.0
    var player_rect := Rect2(
        MAP_ORIGIN + Vector2(tile_x, tile_y) * TILE_SIZE + Vector2(inset, inset),
        Vector2(TILE_SIZE - inset * 2.0 - 2.0, TILE_SIZE - inset * 2.0 - 2.0)
    )
    draw_rect(
        player_rect,
        Color(0.824, 0.290, 0.290) if encounter_pending else Color(0.290, 0.545, 0.824),
        true
    )

    var status := "tile=(%d,%d) steps=%d interactions=%d flags=0x%X encounter=%s" % [
        tile_x,
        tile_y,
        step_count,
        interaction_count,
        event_flags,
        str(encounter_pending),
    ]
    draw_string(ThemeDB.fallback_font, Vector2(64, 610), status, HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color.WHITE)
