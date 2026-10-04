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
