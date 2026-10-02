#include "remaster/core.h"

#include <assert.h>
#include <stdio.h>

static void run_reference_sequence(RemasterState *state)
{
    remaster_core_step(state, REMASTER_INPUT_MOVE_UP);
    remaster_core_step(state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(state, REMASTER_INPUT_INTERACT);
    remaster_core_step(state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(state, REMASTER_INPUT_MOVE_DOWN);
}

int main(void)
{
    RemasterState state;
    RemasterState restored;
    uint8_t save_data[REMASTER_CORE_STATE_BYTES];
    uint64_t first_hash;
    uint64_t second_hash;

    remaster_core_init(&state);

    assert(state.tile_x == 1);
    assert(state.tile_y == 1);

    run_reference_sequence(&state);

    assert(state.tile_x == 3);
    assert(state.tile_y == 4);
    assert(state.step_count == 5);
    assert(state.interaction_count == 1);
    assert((state.event_flags & 1u) != 0u);
    assert(state.encounter_pending == 1);

    first_hash = remaster_core_state_hash(&state);

    assert(remaster_core_state_size() == REMASTER_CORE_STATE_BYTES);
    assert(remaster_core_save(&state, save_data, sizeof(save_data)) == 1);

    remaster_core_init(&restored);
    assert(remaster_core_load(&restored, save_data, sizeof(save_data)) == 1);

    second_hash = remaster_core_state_hash(&restored);
    assert(first_hash == second_hash);

    /* Both original and restored state must continue identically. */
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&restored, REMASTER_INPUT_MOVE_RIGHT);
    assert(remaster_core_state_hash(&state) == remaster_core_state_hash(&restored));

    /* Guard invalid buffers instead of reading/writing partial state. */
    assert(remaster_core_save(&state, save_data, REMASTER_CORE_STATE_BYTES - 1u) == 0);
    assert(remaster_core_load(&restored, save_data, REMASTER_CORE_STATE_BYTES - 1u) == 0);

    printf("R0 core smoke test passed. state_hash=%llu\n",
           (unsigned long long)remaster_core_state_hash(&state));

    return 0;
}
