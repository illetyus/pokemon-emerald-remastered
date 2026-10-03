#include "remaster/emerald_object_state.h"

#include <string.h>

enum {
    SB1_OBJECT_EVENTS = 0x0A30,
    SB1_OBJECT_TEMPLATES = 0x0C70,

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
