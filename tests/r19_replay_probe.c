#include "remaster/emerald_overworld.h"

#include <stdio.h>
#include <string.h>

/* Explicit synthetic I1 setup, not the real-map/story evidence owned by I3. */
static const uint16_t kBlocks[9] = {0, 0x0400, 0, 0, 0, 0, 0, 0, 0};
static const uint16_t kAttributes[1] = {0};
static const uint16_t kBorder[4] = {0, 0, 0, 0};

static int snapshot(const RemasterEmeraldSave *save, unsigned kind, unsigned collision)
{
    RemasterEmeraldOverworldState state;
    if (!remaster_emerald_overworld_get(save, &state))
        return 0;
    printf("{\"x\":%d,\"y\":%d,\"kind\":%u,\"collision\":%u}\n",
           (int)state.player_x, (int)state.player_y, kind, collision);
    return !ferror(stdout);
}

int main(int argc, char **argv)
{
    RemasterEmeraldSave save;
    RemasterEmeraldOverworldState state;
    RemasterEmeraldMovementContext movement;
    RemasterEmeraldMapView map;
    char line[64];
    unsigned index = 0;
    if (argc != 2 || strcmp(argv[1], "synthetic-movement-v1") != 0) {
        fputs("unknown setup recipe\n", stderr);
        return 1;
    }
    memset(&save, 0, sizeof(save));
    memset(&state, 0, sizeof(state));
    memset(&map, 0, sizeof(map));
    memset(&movement, 0, sizeof(movement));
    state.player_x = 1;
    state.player_y = 1;
    state.map_group = 0;
    state.map_num = 9;
    state.map_layout_id = 1;
    map.width = map.height = 3;
    map.blocks = kBlocks;
    map.block_count = 9;
    map.border = kBorder;
    map.border_count = 4;
    map.primary_attributes = kAttributes;
    map.primary_attribute_count = 1;
    movement.map = &map;
    if (!remaster_emerald_overworld_set(&save, &state) || !snapshot(&save, 0, 0))
        return 1;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        RemasterEmeraldOverworldActionResult action;
        uint8_t direction;
        ++index;
        if (index > 4096)
            goto rejected;
        if (strcmp(line, "step south\n") == 0)
            direction = REMASTER_EMERALD_DIR_SOUTH;
        else if (strcmp(line, "step north\n") == 0)
            direction = REMASTER_EMERALD_DIR_NORTH;
        else if (strcmp(line, "step west\n") == 0)
            direction = REMASTER_EMERALD_DIR_WEST;
        else if (strcmp(line, "step east\n") == 0)
            direction = REMASTER_EMERALD_DIR_EAST;
        else
            goto rejected;
        if (!remaster_emerald_overworld_step_action(&save, &movement, NULL, 0,
                NULL, 0, NULL, 0, direction, &action)
            || !snapshot(&save, (unsigned)action.kind, (unsigned)action.collision))
            goto rejected;
    }
    return ferror(stdin) || ferror(stdout) || index == 0 ? 1 : 0;
rejected:
    fprintf(stderr, "rejected command at index %u\n", index);
    return 1;
}
