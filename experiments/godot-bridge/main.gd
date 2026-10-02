extends Control

const MAP_WIDTH := 8
const MAP_HEIGHT := 8
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
var perf_elapsed_seconds := 0.0
var perf_total_frame_ms := 0.0
var perf_worst_frame_ms := 0.0
var perf_frame_count := 0

var background: ColorRect
var title_label: Label
var status_label: Label
var diagnostic_label: Label
var board: GridContainer
var cells: Array[ColorRect] = []
var touch_hint: Label


func _ready() -> void:
    if OS.has_feature("mobile"):
        DisplayServer.screen_set_orientation(DisplayServer.SCREEN_LANDSCAPE)
        DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)

    set_process(true)
    set_process_unhandled_input(true)
    _build_ui()

    core = ClassDB.instantiate("RemasterCoreBridge")

    if core == null:
        _show_bridge_error()
        return

    diagnostic_label.text = "C CORE: LOADED"
    diagnostic_label.modulate = Color(0.45, 1.0, 0.55)
    load_persistent_state()
    _refresh_ui()


func _build_ui() -> void:
    background = ColorRect.new()
    background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    background.color = Color(0.035, 0.045, 0.065)
    background.mouse_filter = Control.MOUSE_FILTER_IGNORE
    add_child(background)

    var margin := MarginContainer.new()
    margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    margin.add_theme_constant_override("margin_left", 32)
    margin.add_theme_constant_override("margin_right", 32)
    margin.add_theme_constant_override("margin_top", 26)
    margin.add_theme_constant_override("margin_bottom", 24)
    margin.mouse_filter = Control.MOUSE_FILTER_IGNORE
    add_child(margin)

    var root_box := VBoxContainer.new()
    root_box.add_theme_constant_override("separation", 12)
    margin.add_child(root_box)

    title_label = Label.new()
    title_label.text = "POKEMON EMERALD REMASTERED — R0"
    title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    title_label.add_theme_font_size_override("font_size", 26)
    root_box.add_child(title_label)

    diagnostic_label = Label.new()
    diagnostic_label.text = "C CORE: CHECKING..."
    diagnostic_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    diagnostic_label.add_theme_font_size_override("font_size", 20)
    root_box.add_child(diagnostic_label)

    var board_center := CenterContainer.new()
    board_center.size_flags_vertical = Control.SIZE_EXPAND_FILL
    root_box.add_child(board_center)

    board = GridContainer.new()
    board.columns = MAP_WIDTH
    board.add_theme_constant_override("h_separation", 2)
    board.add_theme_constant_override("v_separation", 2)
    board_center.add_child(board)

    cells.clear()
    for y in range(MAP_HEIGHT):
        for x in range(MAP_WIDTH):
            var cell := ColorRect.new()
            cell.custom_minimum_size = Vector2(44, 44)
            cell.mouse_filter = Control.MOUSE_FILTER_IGNORE
            board.add_child(cell)
            cells.append(cell)

    status_label = Label.new()
    status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    status_label.add_theme_font_size_override("font_size", 18)
    root_box.add_child(status_label)

    touch_hint = Label.new()
    touch_hint.text = "TOUCH: left side = movement   •   right side = interact"
    touch_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    touch_hint.add_theme_font_size_override("font_size", 16)
    touch_hint.modulate = Color(0.75, 0.8, 0.9)
    root_box.add_child(touch_hint)


func _show_bridge_error() -> void:
    background.color = Color(0.20, 0.025, 0.035)
    diagnostic_label.text = "ERROR: ANDROID GDEXTENSION DID NOT LOAD"
    diagnostic_label.modulate = Color(1.0, 0.35, 0.35)
    status_label.text = "RemasterCoreBridge is unavailable. This build is invalid."
    touch_hint.text = "Report this red screen; do not accept the R0 build."
    _paint_static_board()


func _paint_static_board() -> void:
    for y in range(MAP_HEIGHT):
        for x in range(MAP_WIDTH):
            var index := y * MAP_WIDTH + x
            cells[index].color = Color(0.20, 0.22, 0.27) if COLLISION[y][x] == 1 else Color(0.18, 0.38, 0.20)


func _refresh_ui() -> void:
    if core == null:
        return

    var state: Dictionary = core.snapshot()

    for y in range(MAP_HEIGHT):
        for x in range(MAP_WIDTH):
            var index := y * MAP_WIDTH + x
            var color := Color(0.22, 0.25, 0.29) if COLLISION[y][x] == 1 else Color(0.22, 0.58, 0.27)

            if x == 3 and y == 1:
                color = Color(0.95, 0.65, 0.12) if (int(state.event_flags) & 1) != 0 else Color(0.58, 0.32, 0.10)

            if x == int(state.tile_x) and y == int(state.tile_y):
                color = Color(0.92, 0.25, 0.25) if bool(state.encounter_pending) else Color(0.16, 0.48, 0.95)

            cells[index].color = color

    status_label.text = "tile=(%d,%d)  steps=%d  interactions=%d  flags=0x%X  encounter=%s\nhash=%d" % [
        int(state.tile_x),
        int(state.tile_y),
        int(state.step_count),
        int(state.interaction_count),
        int(state.event_flags),
        str(bool(state.encounter_pending)),
        core.state_hash(),
    ]


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


func _process(delta: float) -> void:
    var frame_ms := delta * 1000.0

    perf_elapsed_seconds += delta
    perf_total_frame_ms += frame_ms
    perf_worst_frame_ms = max(perf_worst_frame_ms, frame_ms)
    perf_frame_count += 1

    if perf_elapsed_seconds >= 5.0 and perf_frame_count > 0:
        var fps := float(perf_frame_count) / perf_elapsed_seconds
        var average_ms := perf_total_frame_ms / float(perf_frame_count)

        print("R0 PERF renderer=Godot fps=%.2f avg_frame_ms=%.3f worst_frame_ms=%.3f frames=%d" % [
            fps,
            average_ms,
            perf_worst_frame_ms,
            perf_frame_count,
        ])

        perf_elapsed_seconds = 0.0
        perf_total_frame_ms = 0.0
        perf_worst_frame_ms = 0.0
        perf_frame_count = 0


func apply_action(action: StringName) -> void:
    if core == null:
        return
    core.step(action)
    _refresh_ui()


func _unhandled_input(event: InputEvent) -> void:
    if core == null:
        return

    if event is InputEventKey and event.pressed and not event.echo:
        match event.keycode:
            KEY_ESCAPE:
                get_tree().quit()
            KEY_R:
                core.reset()
                _refresh_ui()
            KEY_F5:
                saved_state = core.save_state()
                persist_state_to_disk()
            KEY_F9:
                if not saved_state.is_empty():
                    core.load_state(saved_state)
                    _refresh_ui()
                elif load_persistent_state():
                    _refresh_ui()
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
