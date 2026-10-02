#include "remaster/core.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    RemasterState state;
    uint64_t first_hash;
    uint64_t second_hash;

    remaster_core_init(&state);

    assert(state.tile_x == 1);
    assert(state.tile_y == 1);

    /* Wall collision must not advance logical state. */
    remaster_core_step(&state, REMASTER_INPUT_MOVE_UP);
    assert(state.tile_x == 1);
    assert(state.tile_y == 1);
    assert(state.step_count == 0);

    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    assert(state.tile_x == 3);
    assert(state.tile_y == 1);
    assert(state.step_count == 2);

    remaster_core_step(&state, REMASTER_INPUT_INTERACT);
    assert((state.event_flags & 1u) != 0u);
    assert(state.interaction_count == 1);

    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    assert(state.step_count == 5);
    assert(state.encounter_pending == 1);

    first_hash = remaster_core_state_hash(&state);

    /* The same input sequence must produce the same gameplay truth. */
    remaster_core_init(&state);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_UP);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_INTERACT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);

    second_hash = remaster_core_state_hash(&state);
    assert(first_hash == second_hash);

    printf("R0 core smoke test passed. state_hash=%llu\n",
           (unsigned long long)second_hash);

    return 0;
}
