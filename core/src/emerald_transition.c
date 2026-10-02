#include "remaster/emerald_transition.h"

int remaster_emerald_apply_warp(
    RemasterEmeraldSave *save,
    RemasterEmeraldWarpState destination,
    uint16_t target_layout_id,
    int16_t target_width,
    int16_t target_height,
    const RemasterEmeraldWarpEventDef *target_warps,
    size_t target_warp_count)
{
    RemasterEmeraldOverworldState state;
    int16_t player_x;
    int16_t player_y;

    if (save == 0 || target_width <= 0 || target_height <= 0)
        return 0;

    if (!remaster_emerald_overworld_get(save, &state))
        return 0;

    /*
     * Vanilla SetPlayerCoordsFromWarp precedence:
     * 1. valid destination warp id -> coordinates of that warp
     * 2. explicit non-negative x/y
     * 3. center of map
     */
    if (destination.warp_id >= 0
        && (size_t)destination.warp_id < target_warp_count
        && target_warps != 0) {
        player_x = target_warps[(size_t)destination.warp_id].x;
        player_y = target_warps[(size_t)destination.warp_id].y;
    } else if (destination.x >= 0 && destination.y >= 0) {
        player_x = destination.x;
        player_y = destination.y;
    } else {
        player_x = (int16_t)(target_width / 2);
        player_y = (int16_t)(target_height / 2);
    }

    state.map_group = destination.map_group;
    state.map_num = destination.map_num;
    state.warp_id = destination.warp_id;
    state.warp_x = destination.x;
    state.warp_y = destination.y;
    state.player_x = player_x;
    state.player_y = player_y;
    state.map_layout_id = target_layout_id;

    remaster_emerald_clear_temp_field_event_data(save);

    return remaster_emerald_overworld_set(save, &state);
}
