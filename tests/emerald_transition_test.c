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
    RemasterEmeraldConnectionDef connections[3];
    size_t connection_index = 0;
    uint16_t value = 0;
    int flag = 0;

    memset(&save, 0, sizeof(save));
    memset(&state, 0, sizeof(state));
    memset(&destination, 0, sizeof(destination));
    memset(&dynamic_warp, 0, sizeof(dynamic_warp));
    memset(&observed_dynamic, 0, sizeof(observed_dynamic));
    memset(target_warps, 0, sizeof(target_warps));
    memset(connections, 0, sizeof(connections));

    state.map_group = 0;
    state.map_num = 0;
    state.player_x = 1;
    state.player_y = 1;
    state.money = 1234;
    state.party_count = 2;
    state.saved_music = 777;
    state.weather = REMASTER_EMERALD_WEATHER_SHADE;
    state.weather_cycle_stage = 2;
    state.flash_level = 4;

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
    if (!check(
            remaster_emerald_flag_set(
                &save,
                REMASTER_EMERALD_FLAG_SYS_USE_FLASH,
                1),
            "Flash flag seed failed"))
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
                REMASTER_EMERALD_WEATHER_ROUTE119_CYCLE,
                REMASTER_EMERALD_MAP_TYPE_INDOOR,
                0,
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
            && state.map_layout_id == 77
            && state.saved_music == 0
            && state.weather == REMASTER_EMERALD_WEATHER_RAIN_THUNDERSTORM
            && state.weather_cycle_stage == 2
            && state.flash_level == 0,
            "warp-id destination state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&save, 0x001F, &flag) && flag == 0
            && remaster_emerald_flag_get(&save, 0x0020, &flag) && flag == 1
            && remaster_emerald_var_get(&save, 0x4000, &value) && value == 0
            && remaster_emerald_var_get(&save, 0x4010, &value) && value == 8,
            "map-entry temp reset mismatch"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(
                &save,
                REMASTER_EMERALD_FLAG_SYS_USE_FLASH,
                &flag)
            && flag == 1,
            "indoor map should preserve Flash flag"))
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
                REMASTER_EMERALD_WEATHER_ROUTE123_CYCLE,
                REMASTER_EMERALD_MAP_TYPE_UNDERGROUND,
                1,
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
            && state.map_layout_id == 88
            && state.weather == REMASTER_EMERALD_WEATHER_RAIN
            && state.flash_level == 1,
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
                REMASTER_EMERALD_WEATHER_SUNNY,
                REMASTER_EMERALD_MAP_TYPE_ROUTE,
                0,
                21,
                19,
                target_warps,
                2),
            "center fallback transition failed"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.player_x == 10
            && state.player_y == 9
            && state.weather == REMASTER_EMERALD_WEATHER_SUNNY
            && state.flash_level == 0,
            "center fallback coordinates mismatch"))
        return 1;


    if (!check(
            remaster_emerald_flag_get(
                &save,
                REMASTER_EMERALD_FLAG_SYS_USE_FLASH,
                &flag)
            && flag == 0,
            "outdoor map should clear Flash flag"))
        return 1;

    destination.map_group = 6;
    destination.map_num = 7;
    destination.warp_id = -1;
    destination.x = 2;
    destination.y = 2;

    if (!check(
            remaster_emerald_apply_warp(
                &save,
                destination,
                100,
                REMASTER_EMERALD_WEATHER_NONE,
                REMASTER_EMERALD_MAP_TYPE_UNDERGROUND,
                1,
                12,
                12,
                target_warps,
                2),
            "dark-cave transition failed"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.weather == REMASTER_EMERALD_WEATHER_NONE
            && state.flash_level == 7,
            "dark cave without Flash should use default darkness level"))
        return 1;


    /*
     * Camera/map connections are not ordinary warps. Vanilla selects the
     * first direction-compatible connection whose perpendicular coordinate
     * lies inside the overlap interval, then translates the local position by
     * the connection offset.
     */
    remaster_emerald_overworld_get(&save, &state);
    state.map_group = 10;
    state.map_num = 1;
    state.player_x = 19;
    state.player_y = 5;
    state.weather_cycle_stage = 0;
    state.saved_music = 333;
    if (!check(
            remaster_emerald_overworld_set(&save, &state),
            "connection state seed failed"))
        return 1;

    connections[0].direction = REMASTER_EMERALD_DIR_EAST;
    connections[0].offset = 20;
    connections[0].dest_map_group = 11;
    connections[0].dest_map_num = 1;
    connections[0].dest_width = 15;
    connections[0].dest_height = 6;

    connections[1].direction = REMASTER_EMERALD_DIR_EAST;
    connections[1].offset = 2;
    connections[1].dest_map_group = 11;
    connections[1].dest_map_num = 2;
    connections[1].dest_width = 15;
    connections[1].dest_height = 8;

    connections[2].direction = REMASTER_EMERALD_DIR_NORTH;
    connections[2].offset = -3;
    connections[2].dest_map_group = 12;
    connections[2].dest_map_num = 4;
    connections[2].dest_width = 30;
    connections[2].dest_height = 12;

    if (!check(
            remaster_emerald_find_incoming_connection(
                connections,
                3,
                REMASTER_EMERALD_DIR_EAST,
                state.player_x,
                state.player_y,
                20,
                10,
                &connection_index)
            && connection_index == 1,
            "connection lookup did not select overlapping east connection"))
        return 1;

    if (!check(
            remaster_emerald_flag_set(&save, 0x001F, 1)
            && remaster_emerald_var_set(&save, 0x4000, 55)
            && remaster_emerald_flag_set(
                &save,
                REMASTER_EMERALD_FLAG_SYS_USE_FLASH,
                1),
            "connection entry state seed failed"))
        return 1;

    if (!check(
            remaster_emerald_apply_connection_transition(
                &save,
                &connections[connection_index],
                123,
                REMASTER_EMERALD_WEATHER_RAIN,
                REMASTER_EMERALD_MAP_TYPE_ROUTE,
                0),
            "east connection transition failed"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.map_group == 11
            && state.map_num == 2
            && state.warp_id == -1
            && state.warp_x == -1
            && state.warp_y == -1
            && state.player_x == 0
            && state.player_y == 3
            && state.map_layout_id == 123
            && state.weather == REMASTER_EMERALD_WEATHER_RAIN
            && state.saved_music == 0,
            "east connection destination mismatch"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&save, 0x001F, &flag)
            && flag == 0
            && remaster_emerald_var_get(&save, 0x4000, &value)
            && value == 0,
            "connection transition did not clear temp field state"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(
                &save,
                REMASTER_EMERALD_FLAG_SYS_USE_FLASH,
                &flag)
            && flag == 1,
            "camera transition must preserve Flash usage flag"))
        return 1;

    /*
     * Negative offsets are legal. North transition subtracts the offset from
     * X and enters at the target map's bottom row.
     */
    state.player_x = 4;
    state.player_y = 0;
    if (!check(
            remaster_emerald_overworld_set(&save, &state),
            "north connection state seed failed"))
        return 1;

    if (!check(
            remaster_emerald_find_incoming_connection(
                connections,
                3,
                REMASTER_EMERALD_DIR_NORTH,
                state.player_x,
                state.player_y,
                20,
                10,
                &connection_index)
            && connection_index == 2,
            "negative-offset north connection lookup failed"))
        return 1;

    if (!check(
            remaster_emerald_apply_connection_transition(
                &save,
                &connections[connection_index],
                124,
                REMASTER_EMERALD_WEATHER_SUNNY,
                REMASTER_EMERALD_MAP_TYPE_ROUTE,
                0),
            "north connection transition failed"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.map_group == 12
            && state.map_num == 4
            && state.player_x == 7
            && state.player_y == 11
            && state.map_layout_id == 124,
            "north connection coordinate translation mismatch"))
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
