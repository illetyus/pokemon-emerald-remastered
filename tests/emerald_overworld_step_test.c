#include "remaster/emerald_overworld.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_overworld_step_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int seed_state(
    RemasterEmeraldSave *save,
    int16_t x,
    int16_t y)
{
    RemasterEmeraldOverworldState state;

    memset(&state, 0, sizeof(state));
    state.map_group = 0;
    state.map_num = 0;
    state.player_x = x;
    state.player_y = y;
    state.warp_id = -1;
    state.warp_x = -1;
    state.warp_y = -1;

    return remaster_emerald_overworld_set(save, &state);
}

int main(void)
{
    const uint16_t blocks[9] = {
        0x0000, 0x0000, 0x0000,
        0x0400, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000
    };
    const uint16_t border[4] = {0, 0, 0, 0};
    const uint16_t attrs[1] = {0};
    RemasterEmeraldMapView map;
    RemasterEmeraldMovementContext movement;
    RemasterEmeraldWarpEventDef warps[1];
    RemasterEmeraldConnectionDef connections[1];
    RemasterEmeraldPlayerStepResult result;
    RemasterEmeraldOverworldState state;
    RemasterEmeraldSave save;

    memset(&save, 0, sizeof(save));
    memset(&map, 0, sizeof(map));
    memset(&movement, 0, sizeof(movement));
    memset(warps, 0, sizeof(warps));
    memset(connections, 0, sizeof(connections));
    memset(&result, 0, sizeof(result));

    map.width = 3;
    map.height = 3;
    map.blocks = blocks;
    map.block_count = 9;
    map.border = border;
    map.border_count = 4;
    map.primary_attributes = attrs;
    map.primary_attribute_count = 1;

    movement.map = &map;

    warps[0].x = 1;
    warps[0].y = 2;
    warps[0].elevation = 0;
    warps[0].dest_map_group = 1;
    warps[0].dest_map_num = 2;
    warps[0].dest_warp_id = 0;

    connections[0].direction = REMASTER_EMERALD_DIR_NORTH;
    connections[0].offset = 0;
    connections[0].dest_map_group = 2;
    connections[0].dest_map_num = 3;
    connections[0].dest_width = 3;
    connections[0].dest_height = 3;

    if (!check(seed_state(&save, 1, 1), "seed normal step failed"))
        return 1;

    if (!check(
            remaster_emerald_player_step(
                &save,
                &movement,
                connections,
                1,
                warps,
                1,
                REMASTER_EMERALD_DIR_EAST,
                &result),
            "normal step call failed"))
        return 1;

    if (!check(
            result.kind == REMASTER_EMERALD_PLAYER_STEP_MOVED
            && result.collision == REMASTER_EMERALD_COLLISION_NONE
            && result.x == 2
            && result.y == 1,
            "normal step result mismatch"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.player_x == 2 && state.player_y == 1,
            "normal step did not update save coordinates"))
        return 1;

    if (!check(seed_state(&save, 1, 1), "seed blocked step failed"))
        return 1;

    if (!check(
            remaster_emerald_player_step(
                &save,
                &movement,
                connections,
                1,
                warps,
                1,
                REMASTER_EMERALD_DIR_WEST,
                &result),
            "blocked step call failed"))
        return 1;

    if (!check(
            result.kind == REMASTER_EMERALD_PLAYER_STEP_BLOCKED
            && result.collision == REMASTER_EMERALD_COLLISION_IMPASSABLE,
            "blocked step result mismatch"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.player_x == 1 && state.player_y == 1,
            "blocked step mutated save coordinates"))
        return 1;

    if (!check(seed_state(&save, 1, 1), "seed warp step failed"))
        return 1;

    if (!check(
            remaster_emerald_player_step(
                &save,
                &movement,
                connections,
                1,
                warps,
                1,
                REMASTER_EMERALD_DIR_SOUTH,
                &result),
            "warp step call failed"))
        return 1;

    if (!check(
            result.kind == REMASTER_EMERALD_PLAYER_STEP_WARP
            && result.warp_index == 0
            && result.x == 1
            && result.y == 2,
            "warp step result mismatch"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.player_x == 1 && state.player_y == 2,
            "warp step must first commit local tile position"))
        return 1;

    if (!check(seed_state(&save, 1, 0), "seed connection step failed"))
        return 1;

    if (!check(
            remaster_emerald_player_step(
                &save,
                &movement,
                connections,
                1,
                warps,
                1,
                REMASTER_EMERALD_DIR_NORTH,
                &result),
            "connection step call failed"))
        return 1;

    if (!check(
            result.kind == REMASTER_EMERALD_PLAYER_STEP_CONNECTION
            && result.connection_index == 0
            && result.x == 1
            && result.y == 0,
            "connection step result mismatch"))
        return 1;

    remaster_emerald_overworld_get(&save, &state);
    if (!check(
            state.player_x == 1 && state.player_y == 0
            && state.map_group == 0
            && state.map_num == 0,
            "connection request must not mutate source map state"))
        return 1;

    {
        const uint16_t ledge_blocks[4] = {
            0x0000,
            0x0001,
            0x0000,
            0x0000
        };
        const uint16_t ledge_attrs[2] = {
            0x0000,
            0x003B
        };
        RemasterEmeraldMapView ledge_map;
        RemasterEmeraldMovementContext ledge_movement;

        memset(&ledge_map, 0, sizeof(ledge_map));
        memset(&ledge_movement, 0, sizeof(ledge_movement));

        ledge_map.width = 1;
        ledge_map.height = 4;
        ledge_map.blocks = ledge_blocks;
        ledge_map.block_count = 4;
        ledge_map.border = border;
        ledge_map.border_count = 4;
        ledge_map.primary_attributes = ledge_attrs;
        ledge_map.primary_attribute_count = 2;

        ledge_movement.map = &ledge_map;

        if (!check(seed_state(&save, 0, 0), "seed ledge step failed"))
            return 1;

        if (!check(
                remaster_emerald_player_step(
                    &save,
                    &ledge_movement,
                    0,
                    0,
                    0,
                    0,
                    REMASTER_EMERALD_DIR_SOUTH,
                    &result),
                "ledge player step call failed"))
            return 1;

        if (!check(
                result.kind == REMASTER_EMERALD_PLAYER_STEP_LEDGE_JUMP
                && result.collision == REMASTER_EMERALD_COLLISION_LEDGE_JUMP
                && result.x == 0
                && result.y == 2,
                "ledge jump must land two tiles away"))
            return 1;

        if (!check(
                remaster_emerald_overworld_get(&save, &state)
                && state.player_x == 0
                && state.player_y == 2,
                "ledge jump did not persist landing coordinates"))
            return 1;
    }

    puts("Emerald overworld player-step test passed.");
    return 0;
}
