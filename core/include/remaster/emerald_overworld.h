#ifndef REMASTER_EMERALD_OVERWORLD_H
#define REMASTER_EMERALD_OVERWORLD_H

#include "remaster/emerald_events.h"
#include "remaster/emerald_movement.h"
#include "remaster/emerald_state.h"
#include "remaster/emerald_transition.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterEmeraldPlayerStepKind {
    REMASTER_EMERALD_PLAYER_STEP_INVALID = 0,
    REMASTER_EMERALD_PLAYER_STEP_BLOCKED = 1,
    REMASTER_EMERALD_PLAYER_STEP_MOVED = 2,
    REMASTER_EMERALD_PLAYER_STEP_WARP = 3,
    REMASTER_EMERALD_PLAYER_STEP_CONNECTION = 4
} RemasterEmeraldPlayerStepKind;

typedef struct RemasterEmeraldPlayerStepResult {
    RemasterEmeraldPlayerStepKind kind;
    RemasterEmeraldCollision collision;
    int16_t x;
    int16_t y;
    uint8_t elevation;
    size_t warp_index;
    size_t connection_index;
} RemasterEmeraldPlayerStepResult;

typedef enum RemasterEmeraldStepEventKind {
    REMASTER_EMERALD_STEP_EVENT_NONE = 0,
    REMASTER_EMERALD_STEP_EVENT_IMMEDIATE_SCRIPT = 1,
    REMASTER_EMERALD_STEP_EVENT_COORD_SCRIPT = 2,
    REMASTER_EMERALD_STEP_EVENT_WARP = 3
} RemasterEmeraldStepEventKind;

typedef int (*RemasterEmeraldImmediateScriptFn)(
    void *userdata,
    const char *script_id);

typedef struct RemasterEmeraldStepEventResult {
    RemasterEmeraldStepEventKind kind;
    size_t coord_event_index;
    size_t next_coord_event_index;
    size_t warp_index;
    const char *script_id;
    uint8_t weather_changed;
    uint8_t weather;
} RemasterEmeraldStepEventResult;

int remaster_emerald_coord_weather_to_weather(
    uint16_t coord_weather,
    uint8_t *out_weather);

int remaster_emerald_find_directional_warp(
    const RemasterEmeraldWarpEventDef *warps,
    size_t warp_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    uint8_t metatile_behavior,
    uint8_t direction,
    size_t *out_warp_index);

/*
 * Mirrors the step-based portion of Vanilla ProcessPlayerFieldInput:
 * coordinate events are scanned before warp events. Weather coordinate
 * events apply immediately and scanning continues. TRIGGER_RUN_IMMEDIATELY
 * scripts may be executed synchronously through run_immediate; when no host
 * callback is supplied they are returned as an explicit resumable handoff.
 */
int remaster_emerald_process_step_events(
    RemasterEmeraldSave *save,
    const RemasterEmeraldCoordEventDef *coord_events,
    size_t coord_event_count,
    size_t coord_start_index,
    const RemasterEmeraldWarpEventDef *warps,
    size_t warp_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    uint8_t metatile_behavior,
    void *userdata,
    RemasterEmeraldImmediateScriptFn run_immediate,
    RemasterEmeraldStepEventResult *out_result);

/*
 * Executes one authoritative free-roaming player step against the current
 * map. Local movement mutates SaveBlock1 position immediately. Warps and map
 * connections are reported by index but are not applied here because the host
 * must first resolve/load the destination map metadata, then call the existing
 * transition API.
 *
 * Return value:
 *   1 -> the input was valid and OutResult describes the step outcome.
 *   0 -> invalid input or save state; no step was performed.
 */
int remaster_emerald_player_step(
    RemasterEmeraldSave *save,
    const RemasterEmeraldMovementContext *movement,
    const RemasterEmeraldConnectionDef *connections,
    size_t connection_count,
    const RemasterEmeraldWarpEventDef *warps,
    size_t warp_count,
    uint8_t direction,
    RemasterEmeraldPlayerStepResult *out_result);

#ifdef __cplusplus
}
#endif

#endif
