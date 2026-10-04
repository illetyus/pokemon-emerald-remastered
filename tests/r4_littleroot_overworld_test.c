#include "remaster/emerald_overworld.h"
#include "r4_littleroot_fixture.h"

#include <stdio.h>
#include <string.h>

enum { MAX_FIXTURE_TILES = 1024 };

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "r4_littleroot_overworld_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int is_warp_tile(
    const RemasterR4FixtureMap *map,
    int32_t x,
    int32_t y)
{
    size_t i;

    for (i = 0; i < map->warp_count; ++i) {
        if (map->warps[i].x == x && map->warps[i].y == y)
            return 1;
    }

    return 0;
}

static int find_path_to_north_edge(
    const RemasterR4FixtureMap *map,
    int16_t start_x,
    int16_t start_y,
    uint8_t *out_directions,
    size_t *out_count)
{
    int visited[MAX_FIXTURE_TILES];
    int previous[MAX_FIXTURE_TILES];
    uint8_t previous_direction[MAX_FIXTURE_TILES];
    int queue[MAX_FIXTURE_TILES];
    uint8_t reverse_path[MAX_FIXTURE_TILES];
    int head = 0;
    int tail = 0;
    int start;
    int goal = -1;
    int tile_count;
    int i;
    static const uint8_t directions[4] = {
        REMASTER_EMERALD_DIR_NORTH,
        REMASTER_EMERALD_DIR_WEST,
        REMASTER_EMERALD_DIR_EAST,
        REMASTER_EMERALD_DIR_SOUTH
    };

    if (map == 0 || out_directions == 0 || out_count == 0)
        return 0;

    tile_count = map->view.width * map->view.height;
    if (tile_count <= 0 || tile_count > MAX_FIXTURE_TILES)
        return 0;

    if (start_x < 0 || start_y < 0
        || start_x >= map->view.width
        || start_y >= map->view.height)
        return 0;

    memset(visited, 0, sizeof(visited));
    for (i = 0; i < MAX_FIXTURE_TILES; ++i) {
        previous[i] = -1;
        previous_direction[i] = REMASTER_EMERALD_DIR_NONE;
    }

    start = (int)start_y * map->view.width + (int)start_x;
    visited[start] = 1;
    queue[tail++] = start;

    while (head < tail) {
        const int current = queue[head++];
        const int32_t x = current % map->view.width;
        const int32_t y = current / map->view.width;
        size_t direction_index;

        if (y == 0) {
            goal = current;
            break;
        }

        for (direction_index = 0; direction_index < 4; ++direction_index) {
            const uint8_t direction = directions[direction_index];
            int32_t target_x = x;
            int32_t target_y = y;
            int target;
            uint8_t current_elevation;

            remaster_emerald_move_coords(
                direction,
                &target_x,
                &target_y);

            if (target_x < 0 || target_y < 0
                || target_x >= map->view.width
                || target_y >= map->view.height)
                continue;

            if (is_warp_tile(map, target_x, target_y))
                continue;

            target =
                (int)target_y * map->view.width + (int)target_x;
            if (visited[target])
                continue;

            current_elevation = remaster_emerald_map_elevation_at(
                &map->view,
                x,
                y);

            if (!remaster_emerald_map_can_enter_direction(
                    &map->view,
                    x,
                    y,
                    target_x,
                    target_y,
                    direction,
                    current_elevation))
                continue;

            visited[target] = 1;
            previous[target] = current;
            previous_direction[target] = direction;
            queue[tail++] = target;
        }
    }

    if (goal < 0)
        return 0;

    {
        size_t count = 0;
        int cursor = goal;

        while (cursor != start) {
            if (cursor < 0 || count >= MAX_FIXTURE_TILES)
                return 0;

            reverse_path[count++] = previous_direction[cursor];
            cursor = previous[cursor];
        }

        for (i = 0; i < (int)count; ++i)
            out_directions[i] = reverse_path[count - 1u - (size_t)i];

        *out_count = count;
    }

    return 1;
}

int main(void)
{
    const RemasterR4FixtureMap *house;
    const RemasterR4FixtureMap *town;
    const RemasterR4FixtureMap *route101;
    const RemasterR4FixtureMap *target;
    RemasterEmeraldMovementContext movement;
    RemasterEmeraldPlayerStepResult result;
    RemasterEmeraldOverworldState state;
    RemasterEmeraldWarpState destination;
    RemasterEmeraldSave save;
    uint8_t path[MAX_FIXTURE_TILES];
    size_t path_count = 0;
    size_t i;
    int16_t connection_x;

    if (!check(
            gRemasterR4LittlerootMapCount == 3,
            "fixture must expose exactly three acceptance maps"))
        return 1;

    house = &gRemasterR4LittlerootMaps[0];
    town = &gRemasterR4LittlerootMaps[1];
    route101 = &gRemasterR4LittlerootMaps[2];

    if (!check(
            strcmp(
                house->id,
                "MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F") == 0
            && strcmp(town->id, "MAP_LITTLEROOT_TOWN") == 0
            && strcmp(route101->id, "MAP_ROUTE101") == 0,
            "fixture map order/identity mismatch"))
        return 1;

    memset(&save, 0, sizeof(save));
    memset(&state, 0, sizeof(state));
    memset(&movement, 0, sizeof(movement));
    memset(&result, 0, sizeof(result));
    memset(&destination, 0, sizeof(destination));

    state.map_group = (int8_t)house->group_num;
    state.map_num = (int8_t)house->map_num;
    state.map_layout_id = house->layout_num;
    state.player_x = 8;
    state.player_y = 7;
    state.warp_id = -1;
    state.warp_x = -1;
    state.warp_y = -1;

    if (!check(
            remaster_emerald_overworld_set(&save, &state),
            "failed to seed Brendan house state"))
        return 1;

    movement.map = &house->view;

    if (!check(
            remaster_emerald_player_step(
                &save,
                &movement,
                house->connections,
                house->connection_count,
                house->warps,
                house->warp_count,
                REMASTER_EMERALD_DIR_SOUTH,
                &result),
            "house exit step failed"))
        return 1;

    if (!check(
            result.kind == REMASTER_EMERALD_PLAYER_STEP_WARP
            && result.x == 8
            && result.y == 8
            && result.warp_index < house->warp_count,
            "real Brendan house door did not resolve as a warp"))
        return 1;

    {
        const RemasterEmeraldWarpEventDef *warp =
            &house->warps[result.warp_index];

        destination.map_group = (int8_t)warp->dest_map_group;
        destination.map_num = (int8_t)warp->dest_map_num;
        destination.warp_id = (int8_t)warp->dest_warp_id;
        destination.x = -1;
        destination.y = -1;
    }

    target = remaster_r4_fixture_find_map(
        destination.map_group,
        destination.map_num);

    if (!check(
            target == town,
            "house exit did not target real LittlerootTown"))
        return 1;

    if (!check(
            remaster_emerald_apply_warp(
                &save,
                destination,
                town->layout_num,
                town->weather_id,
                town->map_type_id,
                town->requires_flash,
                (int16_t)town->view.width,
                (int16_t)town->view.height,
                town->warps,
                town->warp_count),
            "real house -> Littleroot warp application failed"))
        return 1;

    if (!check(
            remaster_emerald_overworld_get(&save, &state)
            && state.map_group == town->group_num
            && state.map_num == town->map_num
            && state.player_x == 5
            && state.player_y == 8,
            "Littleroot destination warp coordinates mismatch"))
        return 1;

    if (!check(
            find_path_to_north_edge(
                town,
                state.player_x,
                state.player_y,
                path,
                &path_count)
            && path_count > 0,
            "no collision-valid Littleroot path to Route 101 edge"))
        return 1;

    movement.map = &town->view;

    for (i = 0; i < path_count; ++i) {
        if (!check(
                remaster_emerald_player_step(
                    &save,
                    &movement,
                    town->connections,
                    town->connection_count,
                    town->warps,
                    town->warp_count,
                    path[i],
                    &result),
                "Littleroot path step call failed"))
            return 1;

        if (!check(
                result.kind == REMASTER_EMERALD_PLAYER_STEP_MOVED,
                "Littleroot BFS path produced non-movement outcome"))
            return 1;
    }

    if (!check(
            remaster_emerald_overworld_get(&save, &state)
            && state.player_y == 0,
            "Littleroot path did not reach north map edge"))
        return 1;

    connection_x = state.player_x;

    if (!check(
            remaster_emerald_player_step(
                &save,
                &movement,
                town->connections,
                town->connection_count,
                town->warps,
                town->warp_count,
                REMASTER_EMERALD_DIR_NORTH,
                &result),
            "north connection step call failed"))
        return 1;

    if (!check(
            result.kind == REMASTER_EMERALD_PLAYER_STEP_CONNECTION
            && result.connection_index < town->connection_count,
            "Littleroot north edge did not request Route 101 connection"))
        return 1;

    {
        const RemasterEmeraldConnectionDef *connection =
            &town->connections[result.connection_index];

        target = remaster_r4_fixture_find_map(
            connection->dest_map_group,
            connection->dest_map_num);

        if (!check(
                target == route101,
                "Littleroot north connection did not target Route101"))
            return 1;

        if (!check(
                remaster_emerald_apply_connection_transition(
                    &save,
                    connection,
                    route101->layout_num,
                    route101->weather_id,
                    route101->map_type_id,
                    route101->requires_flash),
                "Littleroot -> Route101 connection application failed"))
            return 1;
    }

    if (!check(
            remaster_emerald_overworld_get(&save, &state)
            && state.map_group == route101->group_num
            && state.map_num == route101->map_num
            && state.player_x == connection_x
            && state.player_y == route101->view.height - 1,
            "Route101 destination state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_map_collision_at(
                &route101->view,
                state.player_x,
                state.player_y) == 0,
            "Route101 connection landed on an impassable tile"))
        return 1;

    printf(
        "R4 real Littleroot overworld acceptance passed (%lu local steps).\n",
        (unsigned long)path_count);
    return 0;
}
