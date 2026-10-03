#ifndef REMASTER_EMERALD_EVENTS_H
#define REMASTER_EMERALD_EVENTS_H

#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RemasterEmeraldObjectEventDef {
    uint16_t local_id;
    int16_t x;
    int16_t y;
    uint8_t elevation;
    uint16_t flag_id;
    const char *script_id;
} RemasterEmeraldObjectEventDef;

typedef struct RemasterEmeraldWarpEventDef {
    int16_t x;
    int16_t y;
    uint8_t elevation;
    uint8_t dest_warp_id;
    uint8_t dest_map_num;
    uint8_t dest_map_group;
} RemasterEmeraldWarpEventDef;

typedef enum RemasterEmeraldCoordEventKind {
    REMASTER_EMERALD_COORD_TRIGGER = 0,
    REMASTER_EMERALD_COORD_WEATHER = 1
} RemasterEmeraldCoordEventKind;

typedef struct RemasterEmeraldCoordEventDef {
    RemasterEmeraldCoordEventKind kind;
    int16_t x;
    int16_t y;
    uint8_t elevation;

    /* trigger == 0 mirrors Vanilla TRIGGER_RUN_IMMEDIATELY. */
    uint16_t trigger;
    uint16_t index;

    /* Used only for REMASTER_EMERALD_COORD_WEATHER. */
    uint16_t weather;

    /* Stable R2 script identity for trigger events; null for weather-only. */
    const char *script_id;
} RemasterEmeraldCoordEventDef;

typedef enum RemasterEmeraldCoordMatchKind {
    REMASTER_EMERALD_COORD_MATCH_NONE = 0,
    REMASTER_EMERALD_COORD_MATCH_SCRIPT = 1,
    REMASTER_EMERALD_COORD_MATCH_WEATHER = 2
} RemasterEmeraldCoordMatchKind;

typedef struct RemasterEmeraldCoordMatch {
    RemasterEmeraldCoordMatchKind kind;
    size_t event_index;
    uint16_t weather;
} RemasterEmeraldCoordMatch;

typedef enum RemasterEmeraldBackgroundEventKind {
    REMASTER_EMERALD_BG_FACING_ANY = 0,
    REMASTER_EMERALD_BG_FACING_NORTH = 1,
    REMASTER_EMERALD_BG_FACING_SOUTH = 2,
    REMASTER_EMERALD_BG_FACING_EAST = 3,
    REMASTER_EMERALD_BG_FACING_WEST = 4,
    REMASTER_EMERALD_BG_HIDDEN_ITEM = 7,
    REMASTER_EMERALD_BG_SECRET_BASE = 8
} RemasterEmeraldBackgroundEventKind;

typedef struct RemasterEmeraldBackgroundEventDef {
    int16_t x;
    int16_t y;
    uint8_t elevation;
    uint8_t kind;
    uint16_t item_id;
    uint16_t hidden_flag_id;
    uint16_t secret_base_id;
    const char *script_id;
} RemasterEmeraldBackgroundEventDef;

typedef enum RemasterEmeraldBackgroundMatchKind {
    REMASTER_EMERALD_BG_MATCH_NONE = 0,
    REMASTER_EMERALD_BG_MATCH_SCRIPT = 1,
    REMASTER_EMERALD_BG_MATCH_HIDDEN_ITEM = 2,
    REMASTER_EMERALD_BG_MATCH_SECRET_BASE = 3
} RemasterEmeraldBackgroundMatchKind;

typedef struct RemasterEmeraldBackgroundMatch {
    RemasterEmeraldBackgroundMatchKind kind;
    size_t event_index;
    uint16_t item_id;
    uint16_t hidden_flag_id;
    uint16_t secret_base_id;
} RemasterEmeraldBackgroundMatch;

int remaster_emerald_object_event_visible(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldObjectEventDef *event);

int remaster_emerald_find_warp(
    const RemasterEmeraldWarpEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    size_t *out_index);

RemasterEmeraldCoordMatch remaster_emerald_find_coord_event(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldCoordEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation);

RemasterEmeraldBackgroundMatch remaster_emerald_find_background_event(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldBackgroundEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    uint8_t facing_direction);

#ifdef __cplusplus
}
#endif

#endif
