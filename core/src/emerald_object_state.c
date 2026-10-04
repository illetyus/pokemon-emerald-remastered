#include "remaster/emerald_object_state.h"

#include <string.h>

enum {
    SB1_OBJECT_EVENTS = 0x0A30,
    SB1_OBJECT_TEMPLATES = 0x0CB0,

    TEMPLATE_LOCAL_ID = 0x00,
    TEMPLATE_KIND = 0x01,
    TEMPLATE_GRAPHICS_ID = 0x02,
    TEMPLATE_X = 0x04,
    TEMPLATE_Y = 0x06,
    TEMPLATE_ELEVATION = 0x08,
    TEMPLATE_MOVEMENT_TYPE = 0x09,
    TEMPLATE_MOVEMENT_RANGE = 0x0A,
    TEMPLATE_TRAINER_TYPE = 0x0C,
    TEMPLATE_TRAINER_RANGE = 0x0E,
    TEMPLATE_SCRIPT_ADDRESS = 0x10,
    TEMPLATE_FLAG_ID = 0x14
};

static uint16_t object_read_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static int16_t object_read_s16_le(const uint8_t *p)
{
    return (int16_t)object_read_u16_le(p);
}

static uint32_t object_read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8u)
        | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
}

static void object_write_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8u) & 0xffu);
}

static void object_write_s16_le(uint8_t *p, int16_t value)
{
    object_write_u16_le(p, (uint16_t)value);
}

static void object_write_u32_le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8u) & 0xffu);
    p[2] = (uint8_t)((value >> 16u) & 0xffu);
    p[3] = (uint8_t)((value >> 24u) & 0xffu);
}

int remaster_emerald_saved_object_event_read(
    const RemasterEmeraldSave *save,
    size_t index,
    uint8_t out_record[REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES])
{
    size_t offset;

    if (save == 0 || out_record == 0
        || index >= REMASTER_EMERALD_SAVED_OBJECT_EVENT_COUNT)
        return 0;

    offset = SB1_OBJECT_EVENTS
        + index * REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES;

    memcpy(
        out_record,
        save->save_block1 + offset,
        REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES);
    return 1;
}

int remaster_emerald_saved_object_event_write(
    RemasterEmeraldSave *save,
    size_t index,
    const uint8_t record[REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES])
{
    size_t offset;

    if (save == 0 || record == 0
        || index >= REMASTER_EMERALD_SAVED_OBJECT_EVENT_COUNT)
        return 0;

    offset = SB1_OBJECT_EVENTS
        + index * REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES;

    memcpy(
        save->save_block1 + offset,
        record,
        REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES);
    return 1;
}

int remaster_emerald_object_template_get(
    const RemasterEmeraldSave *save,
    size_t index,
    RemasterEmeraldObjectTemplate *out_template)
{
    const uint8_t *record;
    uint8_t movement_range;

    if (save == 0 || out_template == 0
        || index >= REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT)
        return 0;

    record = save->save_block1
        + SB1_OBJECT_TEMPLATES
        + index * REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES;

    memset(out_template, 0, sizeof(*out_template));

    out_template->local_id = record[TEMPLATE_LOCAL_ID];
    out_template->kind = record[TEMPLATE_KIND];
    out_template->graphics_id =
        object_read_u16_le(record + TEMPLATE_GRAPHICS_ID);
    out_template->x = object_read_s16_le(record + TEMPLATE_X);
    out_template->y = object_read_s16_le(record + TEMPLATE_Y);
    out_template->elevation = record[TEMPLATE_ELEVATION];
    out_template->movement_type = record[TEMPLATE_MOVEMENT_TYPE];

    movement_range = record[TEMPLATE_MOVEMENT_RANGE];
    out_template->movement_range_x = movement_range & 0x0fu;
    out_template->movement_range_y = (movement_range >> 4u) & 0x0fu;

    out_template->trainer_type =
        object_read_u16_le(record + TEMPLATE_TRAINER_TYPE);
    out_template->trainer_range_or_berry_tree_id =
        object_read_u16_le(record + TEMPLATE_TRAINER_RANGE);
    out_template->legacy_script_address =
        object_read_u32_le(record + TEMPLATE_SCRIPT_ADDRESS);
    out_template->flag_id =
        object_read_u16_le(record + TEMPLATE_FLAG_ID);

    return 1;
}

int remaster_emerald_object_template_set(
    RemasterEmeraldSave *save,
    size_t index,
    const RemasterEmeraldObjectTemplate *object_template)
{
    uint8_t *record;

    if (save == 0 || object_template == 0
        || index >= REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT
        || object_template->movement_range_x > 0x0fu
        || object_template->movement_range_y > 0x0fu)
        return 0;

    record = save->save_block1
        + SB1_OBJECT_TEMPLATES
        + index * REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES;

    memset(record, 0, REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES);

    record[TEMPLATE_LOCAL_ID] = object_template->local_id;
    record[TEMPLATE_KIND] = object_template->kind;
    object_write_u16_le(
        record + TEMPLATE_GRAPHICS_ID,
        object_template->graphics_id);
    object_write_s16_le(record + TEMPLATE_X, object_template->x);
    object_write_s16_le(record + TEMPLATE_Y, object_template->y);
    record[TEMPLATE_ELEVATION] = object_template->elevation;
    record[TEMPLATE_MOVEMENT_TYPE] = object_template->movement_type;
    record[TEMPLATE_MOVEMENT_RANGE] =
        (uint8_t)(
            (object_template->movement_range_x & 0x0fu)
            | ((object_template->movement_range_y & 0x0fu) << 4u));
    object_write_u16_le(
        record + TEMPLATE_TRAINER_TYPE,
        object_template->trainer_type);
    object_write_u16_le(
        record + TEMPLATE_TRAINER_RANGE,
        object_template->trainer_range_or_berry_tree_id);
    object_write_u32_le(
        record + TEMPLATE_SCRIPT_ADDRESS,
        object_template->legacy_script_address);
    object_write_u16_le(
        record + TEMPLATE_FLAG_ID,
        object_template->flag_id);

    return 1;
}

void remaster_emerald_object_templates_clear(
    RemasterEmeraldSave *save)
{
    if (save == 0)
        return;

    memset(
        save->save_block1 + SB1_OBJECT_TEMPLATES,
        0,
        REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT
            * REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES);
}

int remaster_emerald_object_templates_replace(
    RemasterEmeraldSave *save,
    const RemasterEmeraldObjectTemplate *templates,
    size_t template_count)
{
    size_t i;

    if (save == 0
        || template_count > REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT
        || (template_count != 0u && templates == 0))
        return 0;

    remaster_emerald_object_templates_clear(save);

    for (i = 0; i < template_count; ++i) {
        if (!remaster_emerald_object_template_set(
                save,
                i,
                &templates[i]))
            return 0;
    }

    return 1;
}

int remaster_emerald_object_template_find_local_id(
    const RemasterEmeraldSave *save,
    uint8_t local_id,
    size_t *out_index)
{
    size_t i;

    if (save == 0 || out_index == 0 || local_id == 0)
        return 0;

    for (i = 0; i < REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT; ++i) {
        const uint8_t *record =
            save->save_block1
            + SB1_OBJECT_TEMPLATES
            + i * REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES;

        if (record[TEMPLATE_LOCAL_ID] == local_id) {
            *out_index = i;
            return 1;
        }
    }

    return 0;
}

int remaster_emerald_object_template_set_coords(
    RemasterEmeraldSave *save,
    uint8_t local_id,
    int16_t x,
    int16_t y)
{
    size_t index;
    uint8_t *record;

    if (!remaster_emerald_object_template_find_local_id(
            save,
            local_id,
            &index))
        return 0;

    record = save->save_block1
        + SB1_OBJECT_TEMPLATES
        + index * REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES;

    object_write_s16_le(record + TEMPLATE_X, x);
    object_write_s16_le(record + TEMPLATE_Y, y);
    return 1;
}

int remaster_emerald_object_template_set_movement_type(
    RemasterEmeraldSave *save,
    uint8_t local_id,
    uint8_t movement_type)
{
    size_t index;
    uint8_t *record;

    if (!remaster_emerald_object_template_find_local_id(
            save,
            local_id,
            &index))
        return 0;

    record = save->save_block1
        + SB1_OBJECT_TEMPLATES
        + index * REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES;
    record[TEMPLATE_MOVEMENT_TYPE] = movement_type;
    return 1;
}


void remaster_emerald_object_runtime_reset(
    RemasterEmeraldObjectRuntime *runtime)
{
    if (runtime == 0)
        return;

    memset(runtime, 0, sizeof(*runtime));
}

int remaster_emerald_object_runtime_find(
    const RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    size_t *out_index)
{
    size_t i;

    if (runtime == 0 || out_index == 0 || local_id == 0u)
        return 0;

    for (i = 0; i < runtime->count; ++i) {
        if (runtime->objects[i].local_id == local_id) {
            *out_index = i;
            return 1;
        }
    }

    return 0;
}

int remaster_emerald_object_runtime_get(
    const RemasterEmeraldObjectRuntime *runtime,
    size_t index,
    RemasterEmeraldRuntimeObject *out_object)
{
    if (runtime == 0 || out_object == 0 || index >= runtime->count)
        return 0;

    *out_object = runtime->objects[index];
    return 1;
}

static int runtime_coords_in_spawn_view(
    int32_t player_x,
    int32_t player_y,
    int32_t object_x,
    int32_t object_y)
{
    /*
     * Mirrors TrySpawnObjectEvents with MAP_OFFSET=7,
     * MAP_OFFSET_W=15 and MAP_OFFSET_H=14. Runtime coordinates stay
     * map-local; the comparisons below are the source inequalities after
     * cancelling the +MAP_OFFSET applied to template coordinates.
     */
    return object_x >= player_x - 9
        && object_x <= player_x + 10
        && object_y >= player_y - 7
        && object_y <= player_y + 9;
}

static int runtime_object_in_spawn_view(
    const RemasterEmeraldRuntimeObject *object,
    int32_t player_x,
    int32_t player_y)
{
    if (object == 0)
        return 0;

    return runtime_coords_in_spawn_view(
            player_x,
            player_y,
            object->current_x,
            object->current_y)
        || runtime_coords_in_spawn_view(
            player_x,
            player_y,
            object->initial_x,
            object->initial_y);
}

static int runtime_validate_templates(
    const RemasterEmeraldObjectEventDef *events,
    size_t event_count)
{
    size_t i;
    size_t j;

    if (event_count > REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT
        || (event_count != 0u && events == 0))
        return 0;

    for (i = 0; i < event_count; ++i) {
        if (events[i].local_id == 0u)
            return 0;

        for (j = i + 1u; j < event_count; ++j) {
            if (events[i].local_id == events[j].local_id)
                return 0;
        }
    }

    return 1;
}

int remaster_emerald_object_runtime_sync_view(
    RemasterEmeraldObjectRuntime *runtime,
    const RemasterEmeraldSave *save,
    const RemasterEmeraldObjectEventDef *events,
    size_t event_count,
    int32_t player_x,
    int32_t player_y)
{
    size_t i;

    if (runtime == 0
        || save == 0
        || !runtime_validate_templates(events, event_count))
        return 0;

    /*
     * Vanilla first keeps/removes currently spawned objects as the camera
     * moves, then TrySpawnObjectEvents scans the full template list and fills
     * any free slots with visible templates inside the spawn window.
     */
    for (i = 0; i < runtime->count; ++i) {
        RemasterEmeraldRuntimeObject *object = &runtime->objects[i];

        if (object->active
            && !runtime_object_in_spawn_view(
                object,
                player_x,
                player_y))
        {
            object->active = 0u;
        }
    }

    for (i = 0; i < event_count; ++i) {
        size_t slot = SIZE_MAX;
        size_t existing = SIZE_MAX;
        size_t candidate;
        RemasterEmeraldRuntimeObject *object;

        if (!runtime_coords_in_spawn_view(
                player_x,
                player_y,
                events[i].x,
                events[i].y))
            continue;

        if (!remaster_emerald_object_event_visible(save, &events[i]))
            continue;

        if (remaster_emerald_object_runtime_find(
                runtime,
                events[i].local_id,
                &existing))
        {
            if (runtime->objects[existing].active)
                continue;
            slot = existing;
        } else {
            for (candidate = 0; candidate < runtime->count; ++candidate) {
                if (!runtime->objects[candidate].active) {
                    slot = candidate;
                    break;
                }
            }

            if (slot == SIZE_MAX
                && runtime->count < REMASTER_EMERALD_RUNTIME_OBJECT_COUNT)
            {
                slot = runtime->count++;
            }
        }

        if (slot == SIZE_MAX)
            continue;

        object = &runtime->objects[slot];
        memset(object, 0, sizeof(*object));
        object->active = 1u;
        object->local_id = events[i].local_id;
        object->current_x = events[i].x;
        object->current_y = events[i].y;
        object->previous_x = events[i].x;
        object->previous_y = events[i].y;
        object->initial_x = events[i].x;
        object->initial_y = events[i].y;
        object->elevation = events[i].elevation;
    }

    return 1;
}

int remaster_emerald_object_runtime_load(
    RemasterEmeraldObjectRuntime *runtime,
    const RemasterEmeraldSave *save,
    const RemasterEmeraldObjectEventDef *events,
    size_t event_count,
    int32_t player_x,
    int32_t player_y)
{
    if (runtime == 0)
        return 0;

    remaster_emerald_object_runtime_reset(runtime);
    return remaster_emerald_object_runtime_sync_view(
        runtime,
        save,
        events,
        event_count,
        player_x,
        player_y);
}

int remaster_emerald_object_runtime_set_active(
    RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    int active)
{
    size_t index;

    if (!remaster_emerald_object_runtime_find(
            runtime,
            local_id,
            &index))
        return 0;

    runtime->objects[index].active = active ? 1u : 0u;
    return 1;
}

int remaster_emerald_object_runtime_set_position(
    RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    int32_t x,
    int32_t y,
    uint8_t elevation)
{
    size_t index;
    RemasterEmeraldRuntimeObject *object;

    if (!remaster_emerald_object_runtime_find(
            runtime,
            local_id,
            &index))
        return 0;

    object = &runtime->objects[index];
    object->previous_x = object->current_x;
    object->previous_y = object->current_y;
    object->current_x = x;
    object->current_y = y;
    object->elevation = elevation;
    return 1;
}

int remaster_emerald_object_runtime_set_player_collision_exempt(
    RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    int exempt)
{
    size_t index;

    if (!remaster_emerald_object_runtime_find(
            runtime,
            local_id,
            &index))
        return 0;

    runtime->objects[index].player_collision_exempt =
        exempt ? 1u : 0u;
    return 1;
}

int remaster_emerald_object_runtime_build_colliders(
    const RemasterEmeraldObjectRuntime *runtime,
    RemasterEmeraldObjectCollider *out_colliders,
    size_t collider_capacity,
    size_t *out_count)
{
    size_t i;

    if (runtime == 0
        || out_count == 0
        || collider_capacity < runtime->count
        || (runtime->count != 0u && out_colliders == 0))
        return 0;

    for (i = 0; i < runtime->count; ++i) {
        const RemasterEmeraldRuntimeObject *source =
            &runtime->objects[i];
        RemasterEmeraldObjectCollider *target =
            &out_colliders[i];

        memset(target, 0, sizeof(*target));
        target->active = source->active;
        target->local_id = source->local_id;
        target->current_x = source->current_x;
        target->current_y = source->current_y;
        target->previous_x = source->previous_x;
        target->previous_y = source->previous_y;
        target->elevation = source->elevation;
        target->player_collision_exempt =
            source->player_collision_exempt;
    }

    *out_count = runtime->count;
    return 1;
}
