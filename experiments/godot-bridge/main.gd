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

var core: RemasterCoreBridge
var saved_state := PackedByteArray()


func _ready() -> void:
    core = RemasterCoreBridge.new()
    queue_redraw()


func apply_action(action: StringName) -> void:
    core.step(action)
    queue_redraw()


func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventKey and event.pressed and not event.echo:
        match event.keycode:
            KEY_ESCAPE:
                get_tree().quit()
            KEY_R:
                core.reset()
                queue_redraw()
            KEY_F5:
                saved_state = core.save_state()
            KEY_F9:
                if not saved_state.is_empty():
                    core.load_state(saved_state)
                    queue_redraw()
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


func _draw() -> void:
    if core == null:
        return

    var state: Dictionary = core.snapshot()

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
        Color(0.929, 0.725, 0.294) if (int(state.event_flags) & 1) != 0 else Color(0.710, 0.455, 0.208),
        true
    )

    var inset := 12.0
    var player_rect := Rect2(
        MAP_ORIGIN + Vector2(int(state.tile_x), int(state.tile_y)) * TILE_SIZE + Vector2(inset, inset),
        Vector2(TILE_SIZE - inset * 2.0 - 2.0, TILE_SIZE - inset * 2.0 - 2.0)
    )
    draw_rect(
        player_rect,
        Color(0.824, 0.290, 0.290) if bool(state.encounter_pending) else Color(0.290, 0.545, 0.824),
        true
    )

    var status := "C core | tile=(%d,%d) steps=%d flags=0x%X hash=%d" % [
        int(state.tile_x),
        int(state.tile_y),
        int(state.step_count),
        int(state.event_flags),
        core.state_hash(),
    ]
    draw_string(ThemeDB.fallback_font, Vector2(64, 610), status, HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color.WHITE)
