extends Node2D

const MAP_WIDTH := 8
const MAP_HEIGHT := 8
const TILE_SIZE := 64.0
const MAP_ORIGIN := Vector2(64.0, 64.0)
const PERSISTENT_SAVE_PATH := "user://r0_state.bin"
const SAVE_MAGIC := PackedByteArray([82, 48, 83, 49]) # R0S1

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

var core
var saved_state := PackedByteArray()


func _ready() -> void:
    core = ClassDB.instantiate("RemasterCoreBridge")
    if core == null:
        push_error("RemasterCoreBridge GDExtension is not loaded")
        return

    load_persistent_state()
    queue_redraw()


func persist_state_to_disk() -> bool:
    if core == null:
        return false

    var payload: PackedByteArray = core.save_state()
    if payload.is_empty():
        return false

    var file := FileAccess.open(PERSISTENT_SAVE_PATH, FileAccess.WRITE)
    if file == null:
        push_error("R0 persistent save open failed: %s" % FileAccess.get_open_error())
        return false

    file.store_buffer(SAVE_MAGIC)
    file.store_32(payload.size())
    file.store_buffer(payload)
    file.store_64(core.state_hash())
    file.flush()

    print("R0 persistent save complete hash=%d path=%s" % [
        core.state_hash(),
        PERSISTENT_SAVE_PATH,
    ])
    return true


func load_persistent_state() -> bool:
    if core == null or not FileAccess.file_exists(PERSISTENT_SAVE_PATH):
        return false

    var file := FileAccess.open(PERSISTENT_SAVE_PATH, FileAccess.READ)
    if file == null:
        return false

    var magic := file.get_buffer(SAVE_MAGIC.size())
    if magic != SAVE_MAGIC:
        push_warning("R0 persistent save rejected: bad magic")
        return false

    var payload_size := file.get_32()
    var expected_size: int = core.save_state().size()
    if payload_size != expected_size:
        push_warning("R0 persistent save rejected: payload size mismatch")
        return false

    var payload := file.get_buffer(payload_size)
    var expected_hash := file.get_64()

    if payload.size() != payload_size or not core.load_state(payload):
        push_warning("R0 persistent save rejected: payload load failed")
        core.reset()
        return false

    var actual_hash: int = core.state_hash()
    if actual_hash != expected_hash:
        push_warning("R0 persistent save rejected: hash mismatch")
        core.reset()
        return false

    print("R0 persistent load complete hash=%d path=%s" % [
        actual_hash,
        PERSISTENT_SAVE_PATH,
    ])
    return true


func _notification(what: int) -> void:
    if core == null:
        return

    match what:
        NOTIFICATION_APPLICATION_PAUSED:
            print("R0 lifecycle: APPLICATION_PAUSED")
            persist_state_to_disk()
        NOTIFICATION_APPLICATION_RESUMED:
            print("R0 lifecycle: APPLICATION_RESUMED hash=%d" % core.state_hash())


func _exit_tree() -> void:
    if core != null:
        persist_state_to_disk()


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
                persist_state_to_disk()
            KEY_F9:
                if not saved_state.is_empty():
                    core.load_state(saved_state)
                    queue_redraw()
                elif load_persistent_state():
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
