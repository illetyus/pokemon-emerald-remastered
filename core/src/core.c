#include "remaster/core.h"

#include <stddef.h>

enum {
    MAP_WIDTH = 8,
    MAP_HEIGHT = 8
};

/*
 * Tiny R0 collision fixture.
 * 1 = blocked, 0 = walkable.
 * This is test data, not Emerald map content.
 */
static const uint8_t kCollision[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1}
};

static int is_walkable(int32_t x, int32_t y)
{
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)
        return 0;

    return kCollision[y][x] == 0;
}

void remaster_core_init(RemasterState *state)
{
    if (state == NULL)
        return;

    state->tile_x = 1;
    state->tile_y = 1;
    state->step_count = 0;
    state->interaction_count = 0;
    state->event_flags = 0;
    state->encounter_pending = 0;
}

void remaster_core_step(RemasterState *state, RemasterInput input)
{
    int32_t next_x;
    int32_t next_y;

    if (state == NULL)
        return;

    next_x = state->tile_x;
    next_y = state->tile_y;
    state->encounter_pending = 0;

    switch (input) {
    case REMASTER_INPUT_MOVE_UP:
        next_y--;
        break;
    case REMASTER_INPUT_MOVE_DOWN:
        next_y++;
        break;
    case REMASTER_INPUT_MOVE_LEFT:
        next_x--;
        break;
    case REMASTER_INPUT_MOVE_RIGHT:
        next_x++;
        break;
    case REMASTER_INPUT_INTERACT:
        state->interaction_count++;
        /* R0 fixture event: interacting at (3, 1) raises bit 0. */
        if (state->tile_x == 3 && state->tile_y == 1)
            state->event_flags |= 1u;
        return;
    case REMASTER_INPUT_NONE:
    default:
        return;
    }

    if (!is_walkable(next_x, next_y))
        return;

    state->tile_x = next_x;
    state->tile_y = next_y;
    state->step_count++;

    /*
     * Deterministic encounter fixture: every 5 successful steps.
     * Later this is replaced by the imported Emerald encounter system.
     */
    if ((state->step_count % 5u) == 0u)
        state->encounter_pending = 1;
}

uint64_t remaster_core_state_hash(const RemasterState *state)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    const uint8_t *bytes;
    size_t i;

    if (state == NULL)
        return 0;

    bytes = (const uint8_t *)state;
    for (i = 0; i < sizeof(*state); ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}
