#ifndef REMASTER_EMERALD_OBJECT_STATE_H
#define REMASTER_EMERALD_OBJECT_STATE_H

#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_SAVED_OBJECT_EVENT_COUNT = 16,
    REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES = 0x24,
    REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT = 64,
    REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES = 0x18
};

typedef struct RemasterEmeraldObjectTemplate {
    uint8_t local_id;
    uint8_t kind;
    uint16_t graphics_id;
    int16_t x;
    int16_t y;
    uint8_t elevation;
    uint8_t movement_type;
    uint8_t movement_range_x;
    uint8_t movement_range_y;
    uint16_t trainer_type;
    uint16_t trainer_range_or_berry_tree_id;

    /*
     * Raw 32-bit GBA ROM address. Never cast this to a host pointer.
     * Remaster gameplay binds scripts by generated IR identity instead.
     * Vanilla repairs this field from the current map header on Continue.
     */
    uint32_t legacy_script_address;

    uint16_t flag_id;
} RemasterEmeraldObjectTemplate;

/*
 * Saved live ObjectEvent records are preserved as opaque 0x24-byte records.
 * AGBCC layout of the fork's widened graphicsId is intentionally not guessed
 * until an actual production-toolchain layout probe can run.
 */
int remaster_emerald_saved_object_event_read(
    const RemasterEmeraldSave *save,
    size_t index,
    uint8_t out_record[REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES]);

int remaster_emerald_saved_object_event_write(
    RemasterEmeraldSave *save,
    size_t index,
    const uint8_t record[REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES]);

int remaster_emerald_object_template_get(
    const RemasterEmeraldSave *save,
    size_t index,
    RemasterEmeraldObjectTemplate *out_template);

int remaster_emerald_object_template_set(
    RemasterEmeraldSave *save,
    size_t index,
    const RemasterEmeraldObjectTemplate *object_template);

void remaster_emerald_object_templates_clear(
    RemasterEmeraldSave *save);

int remaster_emerald_object_templates_replace(
    RemasterEmeraldSave *save,
    const RemasterEmeraldObjectTemplate *templates,
    size_t template_count);

int remaster_emerald_object_template_find_local_id(
    const RemasterEmeraldSave *save,
    uint8_t local_id,
    size_t *out_index);

int remaster_emerald_object_template_set_coords(
    RemasterEmeraldSave *save,
    uint8_t local_id,
    int16_t x,
    int16_t y);

int remaster_emerald_object_template_set_movement_type(
    RemasterEmeraldSave *save,
    uint8_t local_id,
    uint8_t movement_type);

#ifdef __cplusplus
}
#endif

#endif
