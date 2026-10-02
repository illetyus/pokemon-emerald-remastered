#ifndef REMASTER_EMERALD_TRANSITION_H
#define REMASTER_EMERALD_TRANSITION_H

#include "remaster/emerald_events.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mirrors the stateful part of Vanilla WarpIntoMap:
 * - apply destination WarpData to SaveBlock1.location
 * - load target map layout identity
 * - choose player coords from target warp id, explicit coords, or map center
 * - clear temporary flags/vars for the newly entered map
 *
 * Map data itself is supplied by the platform-neutral world catalog.
 */
int remaster_emerald_apply_warp(
    RemasterEmeraldSave *save,
    RemasterEmeraldWarpState destination,
    uint16_t target_layout_id,
    int16_t target_width,
    int16_t target_height,
    const RemasterEmeraldWarpEventDef *target_warps,
    size_t target_warp_count);

#ifdef __cplusplus
}
#endif

#endif
