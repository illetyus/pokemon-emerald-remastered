#ifndef REMASTER_CORE_H
#define REMASTER_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterInput {
    REMASTER_INPUT_NONE = 0,
    REMASTER_INPUT_MOVE_UP,
    REMASTER_INPUT_MOVE_DOWN,
    REMASTER_INPUT_MOVE_LEFT,
    REMASTER_INPUT_MOVE_RIGHT,
    REMASTER_INPUT_INTERACT
} RemasterInput;

typedef struct RemasterState {
    int32_t tile_x;
    int32_t tile_y;
    uint32_t step_count;
    uint32_t interaction_count;
    uint32_t event_flags;
    uint8_t encounter_pending;
} RemasterState;

/* Initializes the deterministic R0 reference state. */
void remaster_core_init(RemasterState *state);

/*
 * Advances gameplay truth by one discrete input.
 * Presentation code must never directly mutate RemasterState.
 */
void remaster_core_step(RemasterState *state, RemasterInput input);

/* Stable test hash for comparing adapters/renderers against the same core state. */
uint64_t remaster_core_state_hash(const RemasterState *state);

#ifdef __cplusplus
}
#endif

#endif
