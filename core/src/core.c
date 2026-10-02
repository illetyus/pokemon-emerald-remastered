#include "remaster/core.h"

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
    if (state == 0)
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

    if (state == 0)
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

static uint64_t fnv1a_u32(uint64_t hash, uint32_t value)
{
    unsigned int i;

    for (i = 0; i < 4; ++i) {
        hash ^= (uint8_t)((value >> (i * 8u)) & 0xffu);
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

uint64_t remaster_core_state_hash(const RemasterState *state)
{
    uint64_t hash = UINT64_C(1469598103934665603);

    if (state == 0)
        return 0;

    /*
     * Hash fields explicitly in a defined byte order. Never hash raw struct
     * memory: compiler padding would make cross-platform comparisons unsafe.
     */
    hash = fnv1a_u32(hash, (uint32_t)state->tile_x);
    hash = fnv1a_u32(hash, (uint32_t)state->tile_y);
    hash = fnv1a_u32(hash, state->step_count);
    hash = fnv1a_u32(hash, state->interaction_count);
    hash = fnv1a_u32(hash, state->event_flags);
    hash ^= state->encounter_pending;
    hash *= UINT64_C(1099511628211);

    return hash;
}

static void write_u32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xffu);
    dst[1] = (uint8_t)((value >> 8u) & 0xffu);
    dst[2] = (uint8_t)((value >> 16u) & 0xffu);
    dst[3] = (uint8_t)((value >> 24u) & 0xffu);
}

static uint32_t read_u32_le(const uint8_t *src)
{
    return ((uint32_t)src[0])
         | ((uint32_t)src[1] << 8u)
         | ((uint32_t)src[2] << 16u)
         | ((uint32_t)src[3] << 24u);
}

size_t remaster_core_state_size(void)
{
    return REMASTER_CORE_STATE_BYTES;
}

int remaster_core_save(const RemasterState *state, uint8_t *buffer, size_t buffer_size)
{
    if (state == 0 || buffer == 0 || buffer_size < REMASTER_CORE_STATE_BYTES)
        return 0;

    write_u32_le(buffer + 0, (uint32_t)state->tile_x);
    write_u32_le(buffer + 4, (uint32_t)state->tile_y);
    write_u32_le(buffer + 8, state->step_count);
    write_u32_le(buffer + 12, state->interaction_count);
    write_u32_le(buffer + 16, state->event_flags);
    buffer[20] = state->encounter_pending ? 1u : 0u;

    return 1;
}

int remaster_core_load(RemasterState *state, const uint8_t *buffer, size_t buffer_size)
{
    if (state == 0 || buffer == 0 || buffer_size < REMASTER_CORE_STATE_BYTES)
        return 0;

    state->tile_x = (int32_t)read_u32_le(buffer + 0);
    state->tile_y = (int32_t)read_u32_le(buffer + 4);
    state->step_count = read_u32_le(buffer + 8);
    state->interaction_count = read_u32_le(buffer + 12);
    state->event_flags = read_u32_le(buffer + 16);
    state->encounter_pending = buffer[20] ? 1u : 0u;

    return 1;
}
