#ifndef REMASTER_EMERALD_STATE_H
#define REMASTER_EMERALD_STATE_H

#include "remaster/emerald_save.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_PERSISTENT_FLAG_BYTES = 300,
    REMASTER_EMERALD_PERSISTENT_FLAG_COUNT =
        REMASTER_EMERALD_PERSISTENT_FLAG_BYTES * 8,
    REMASTER_EMERALD_VARS_START = 0x4000,
    REMASTER_EMERALD_VAR_COUNT = 256
};

typedef struct RemasterEmeraldOverworldState {
    int16_t player_x;
    int16_t player_y;

    int8_t map_group;
    int8_t map_num;
    int8_t warp_id;
    int16_t warp_x;
    int16_t warp_y;

    uint16_t map_layout_id;
    uint8_t weather;
    uint8_t flash_level;
    uint8_t party_count;

    uint32_t money;
    uint16_t coins;
    uint16_t registered_item;
} RemasterEmeraldOverworldState;

int remaster_emerald_overworld_get(
    const RemasterEmeraldSave *save,
    RemasterEmeraldOverworldState *out_state);

int remaster_emerald_overworld_set(
    RemasterEmeraldSave *save,
    const RemasterEmeraldOverworldState *state);

int remaster_emerald_flag_get(
    const RemasterEmeraldSave *save,
    uint16_t flag_id,
    int *out_value);

int remaster_emerald_flag_set(
    RemasterEmeraldSave *save,
    uint16_t flag_id,
    int value);

int remaster_emerald_var_get(
    const RemasterEmeraldSave *save,
    uint16_t var_id,
    uint16_t *out_value);

int remaster_emerald_var_set(
    RemasterEmeraldSave *save,
    uint16_t var_id,
    uint16_t value);

#ifdef __cplusplus
}
#endif

#endif
