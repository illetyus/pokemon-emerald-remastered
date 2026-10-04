#ifndef REMASTER_EMERALD_OBJECT_STATE_H
#define REMASTER_EMERALD_OBJECT_STATE_H

#include "remaster/emerald_events.h"
#include "remaster/emerald_movement.h"
#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_SAVED_OBJECT_EVENT_COUNT = 16,
    REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES = 0x28,
    REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT = 64,
    REMASTER_EMERALD_OBJECT_TEMPLATE_BYTES = 0x18,
    REMASTER_EMERALD_RUNTIME_OBJECT_COUNT = 16
};

typedef struct RemasterEmeraldRuntimeObject {
    uint8_t active;
    uint16_t local_id;
    int32_t current_x;
    int32_t current_y;
    int32_t previous_x;
    int32_t previous_y;
    int32_t initial_x;
    int32_t initial_y;
    uint8_t elevation;
    uint8_t player_collision_exempt;
} RemasterEmeraldRuntimeObject;

typedef struct RemasterEmeraldObjectRuntime {
    RemasterEmeraldRuntimeObject objects[
        REMASTER_EMERALD_RUNTIME_OBJECT_COUNT];
    size_t count;
} RemasterEmeraldObjectRuntime;

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
 * Saved live ObjectEvent records are preserved as opaque 0x28-byte records.
 * The fork widened graphicsId to u16. Production save-sector evidence and
 * the resulting SaveBlock1 offsets confirm the AGBCC array stride is 0x28.
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

void remaster_emerald_object_runtime_reset(
    RemasterEmeraldObjectRuntime *runtime);

int remaster_emerald_object_runtime_load(
    RemasterEmeraldObjectRuntime *runtime,
    const RemasterEmeraldSave *save,
    const RemasterEmeraldObjectEventDef *events,
    size_t event_count,
    int32_t player_x,
    int32_t player_y);

int remaster_emerald_object_runtime_sync_view(
    RemasterEmeraldObjectRuntime *runtime,
    const RemasterEmeraldSave *save,
    const RemasterEmeraldObjectEventDef *events,
    size_t event_count,
    int32_t player_x,
    int32_t player_y);

int remaster_emerald_object_runtime_find(
    const RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    size_t *out_index);

int remaster_emerald_object_runtime_get(
    const RemasterEmeraldObjectRuntime *runtime,
    size_t index,
    RemasterEmeraldRuntimeObject *out_object);

int remaster_emerald_object_runtime_set_active(
    RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    int active);

int remaster_emerald_object_runtime_set_position(
    RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    int32_t x,
    int32_t y,
    uint8_t elevation);

int remaster_emerald_object_runtime_set_player_collision_exempt(
    RemasterEmeraldObjectRuntime *runtime,
    uint16_t local_id,
    int exempt);

int remaster_emerald_object_runtime_build_colliders(
    const RemasterEmeraldObjectRuntime *runtime,
    RemasterEmeraldObjectCollider *out_colliders,
    size_t collider_capacity,
    size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif
