#include "remaster/emerald_state.h"
#include "remaster/emerald_transition.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_transition_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldOverworldState state;
    RemasterEmeraldWarpState destination;
    RemasterEmeraldWarpState dynamic_warp;
    RemasterEmeraldWarpState observed_dynamic;
    RemasterEmeraldWarpEventDef target_warps[2];
    uint16_t value = 0;
    int flag = 0;

    memset(&save, 0, sizeof(save));
    memset(&state, 0, sizeof(state));
    memset(&destination, 0, sizeof(destination));
    memset(&dynamic_warp, 0, sizeof(dynamic_warp));
    memset(&observed_dynamic, 0, sizeof(observed_dynamic));
    memset(target_warps, 0, sizeof(target_warps));

    state.map_group = 0;
    state.map_num = 0;
    state.player_x = 1;
    state.player_y = 1;
    state.money = 1234;
    state.party_count = 2;

    if (!check(
            remaster_emerald_overworld_set(&save, &state),
            "initial state set failed"))
        return 1;

    target_warps[0].x = 3;
    target_warps[0].y = 4;
    target_warps[1].x = 10;
    target_warps[1].y = 12;

    if (!check(remaster_emerald_flag_set(&save, 0x001F, 1), "temp flag seed failed"))
        return 1;
    if (!check(remaster_emerald_flag_set(&save, 0x0020, 1), "persistent flag seed failed"))
        return 1;
    if (!check(remaster_emerald_var_set(&save, 0x4000, 9), "temp var seed failed"))
        return 1;
    if (!check(remaster_emerald_var_set(&save, 0x4010, 8), "persistent var seed failed"))
        return 1;

    destination.map_group = 1;
    destination.map_num = 2;
    destination.warp_id = 1;
    destination.x = -1;
    destination.y = -1;

    if (!check(
            remaster_emerald_apply_warp(
                &save,
                destination,
                77,
                20,
                18,
                target_warps,
                2),
            "warp-id transition failed"))
        return 1;

    if (!check(remaster_emerald_overworld_get(&save, &state), "state reload failed"))
        return 1;

    if (!check(
            state.map_group == 1
            && state.map_num == 2
            && state.warp_id == 1
            && state.player_x == 10
            && state.player_y == 12
            && state.map_layout_id == 77,
            "warp-id destination state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&save, 0x001F, &flag) && flag == 0
            && remaster_emerald_flag_get(&save, 0x0020, &flag) && flag == 1
            && remaster_emerald_var_get(&save, 0x4000, &value) && value == 0
            && remaster_emerald_var_get(&save, 0x4010, &value) && value == 8,
            "map-entry temp reset mismatch"))
        return 1;

    destination.map_group = 3;
    destination.map_num = 4;
    destination.warp_id = -1;
    destination.x = 6;
    destination.y = 7;

    if (!check(
            remaster_emerald_apply_warp(
                &save,
                destination,
                88,
                30,
                22,
                target_warps,
                2),
            "explicit-coordinate transition failed"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.player_x == 6
            && state.player_y == 7
            && state.map_layout_id == 88,
            "explicit coordinates were not used"))
        return 1;

    destination.map_group = 5;
    destination.map_num = 6;
    destination.warp_id = -1;
    destination.x = -1;
    destination.y = -1;

    if (!check(
            remaster_emerald_apply_warp(
                &save,
                destination,
                99,
                21,
                19,
                target_warps,
                2),
            "center fallback transition failed"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.player_x == 10
            && state.player_y == 9,
            "center fallback coordinates mismatch"))
        return 1;

    dynamic_warp.map_group = 7;
    dynamic_warp.map_num = 8;
    dynamic_warp.warp_id = 0;
    dynamic_warp.x = 2;
    dynamic_warp.y = 3;

    if (!check(
            remaster_emerald_dynamic_warp_set(&save, &dynamic_warp)
            && remaster_emerald_dynamic_warp_get(&save, &observed_dynamic),
            "dynamic warp round-trip failed"))
        return 1;

    if (!check(
            observed_dynamic.map_group == 7
            && observed_dynamic.map_num == 8
            && observed_dynamic.warp_id == 0
            && observed_dynamic.x == 2
            && observed_dynamic.y == 3,
            "dynamic warp data mismatch"))
        return 1;

    puts("Emerald warp transition compatibility test passed.");
    return 0;
}
