#ifndef REMASTER_CORE_H
#define REMASTER_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REMASTER_CORE_STATE_BYTES 21u

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

void remaster_core_init(RemasterState *state);
void remaster_core_step(RemasterState *state, RemasterInput input);

uint64_t remaster_core_state_hash(const RemasterState *state);

/*
 * R0 portable state format. This is a prototype transport format, not the
 * final Emerald/Vanilla+ save-file specification.
 */
size_t remaster_core_state_size(void);
int remaster_core_save(const RemasterState *state, uint8_t *buffer, size_t buffer_size);
int remaster_core_load(RemasterState *state, const uint8_t *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
