#include "remaster/emerald_state.h"

#include <string.h>

enum {
    SB1_POS_X = 0x0000,
    SB1_POS_Y = 0x0002,
    SB1_LOCATION = 0x0004,
    SB1_CONTINUE_GAME_WARP = 0x000C,
    SB1_DYNAMIC_WARP = 0x0014,
    SB1_LAST_HEAL_WARP = 0x001C,
    SB1_ESCAPE_WARP = 0x0024,
    SB1_SAVED_MUSIC = 0x002C,
    SB1_WEATHER = 0x002E,
    SB1_WEATHER_CYCLE_STAGE = 0x002F,
    SB1_FLASH_LEVEL = 0x0030,
    SB1_MAP_LAYOUT_ID = 0x0032,
    SB1_PARTY_COUNT = 0x0234,
    SB1_MONEY = 0x0490,
    SB1_COINS = 0x0494,
    SB1_REGISTERED_ITEM = 0x0496,

    /*
     * Production AGBCC layout: the fork's widened ObjectEvent.graphicsId
     * makes struct ObjectEvent 0x28 bytes instead of the stale 0x24 comment.
     * Sixteen saved ObjectEvents therefore shift all following SaveBlock1
     * fields by 0x40 bytes.
     */
    SB1_FLAGS = 0x12B0,
    SB1_VARS = 0x13DC,

    SB2_PLAYER_GENDER = 0x0008,
    SB2_ENCRYPTION_KEY = 0x00AC
};

static uint16_t state_read_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static int16_t state_read_s16_le(const uint8_t *p)
{
    return (int16_t)state_read_u16_le(p);
}

static uint32_t state_read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8u)
        | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
}

static void state_write_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8u) & 0xffu);
}

static void state_write_s16_le(uint8_t *p, int16_t value)
{
    state_write_u16_le(p, (uint16_t)value);
}

static void state_write_u32_le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8u) & 0xffu);
    p[2] = (uint8_t)((value >> 16u) & 0xffu);
    p[3] = (uint8_t)((value >> 24u) & 0xffu);
}

int remaster_emerald_overworld_get(
    const RemasterEmeraldSave *save,
    RemasterEmeraldOverworldState *out_state)
{
    uint32_t key;

    if (save == 0 || out_state == 0)
        return 0;

    key = state_read_u32_le(save->save_block2 + SB2_ENCRYPTION_KEY);

    out_state->player_x =
        state_read_s16_le(save->save_block1 + SB1_POS_X);
    out_state->player_y =
        state_read_s16_le(save->save_block1 + SB1_POS_Y);

    out_state->map_group =
        (int8_t)save->save_block1[SB1_LOCATION + 0u];
    out_state->map_num =
        (int8_t)save->save_block1[SB1_LOCATION + 1u];
    out_state->warp_id =
        (int8_t)save->save_block1[SB1_LOCATION + 2u];
    out_state->warp_x =
        state_read_s16_le(save->save_block1 + SB1_LOCATION + 4u);
    out_state->warp_y =
        state_read_s16_le(save->save_block1 + SB1_LOCATION + 6u);

    out_state->saved_music =
        state_read_u16_le(save->save_block1 + SB1_SAVED_MUSIC);
    out_state->weather = save->save_block1[SB1_WEATHER];
    out_state->weather_cycle_stage =
        save->save_block1[SB1_WEATHER_CYCLE_STAGE];
    out_state->flash_level = save->save_block1[SB1_FLASH_LEVEL];
    out_state->map_layout_id =
        state_read_u16_le(save->save_block1 + SB1_MAP_LAYOUT_ID);
    out_state->party_count = save->save_block1[SB1_PARTY_COUNT];

    out_state->money =
        state_read_u32_le(save->save_block1 + SB1_MONEY) ^ key;
    out_state->coins =
        (uint16_t)(
            state_read_u16_le(save->save_block1 + SB1_COINS)
            ^ (uint16_t)key);
    out_state->registered_item =
        state_read_u16_le(save->save_block1 + SB1_REGISTERED_ITEM);

    return 1;
}

int remaster_emerald_overworld_set(
    RemasterEmeraldSave *save,
    const RemasterEmeraldOverworldState *state)
{
    uint32_t key;

    if (save == 0 || state == 0)
        return 0;

    key = state_read_u32_le(save->save_block2 + SB2_ENCRYPTION_KEY);

    state_write_s16_le(save->save_block1 + SB1_POS_X, state->player_x);
    state_write_s16_le(save->save_block1 + SB1_POS_Y, state->player_y);

    save->save_block1[SB1_LOCATION + 0u] = (uint8_t)state->map_group;
    save->save_block1[SB1_LOCATION + 1u] = (uint8_t)state->map_num;
    save->save_block1[SB1_LOCATION + 2u] = (uint8_t)state->warp_id;
    state_write_s16_le(
        save->save_block1 + SB1_LOCATION + 4u,
        state->warp_x);
    state_write_s16_le(
        save->save_block1 + SB1_LOCATION + 6u,
        state->warp_y);

    state_write_u16_le(
        save->save_block1 + SB1_SAVED_MUSIC,
        state->saved_music);
    save->save_block1[SB1_WEATHER] = state->weather;
    save->save_block1[SB1_WEATHER_CYCLE_STAGE] =
        state->weather_cycle_stage;
    save->save_block1[SB1_FLASH_LEVEL] = state->flash_level;
    state_write_u16_le(
        save->save_block1 + SB1_MAP_LAYOUT_ID,
        state->map_layout_id);
    save->save_block1[SB1_PARTY_COUNT] = state->party_count;

    state_write_u32_le(
        save->save_block1 + SB1_MONEY,
        state->money ^ key);
    state_write_u16_le(
        save->save_block1 + SB1_COINS,
        (uint16_t)(state->coins ^ (uint16_t)key));
    state_write_u16_le(
        save->save_block1 + SB1_REGISTERED_ITEM,
        state->registered_item);

    return 1;
}

int remaster_emerald_flag_get(
    const RemasterEmeraldSave *save,
    uint16_t flag_id,
    int *out_value)
{
    uint8_t value;

    if (save == 0 || out_value == 0
        || flag_id == 0
        || flag_id >= REMASTER_EMERALD_PERSISTENT_FLAG_COUNT)
        return 0;

    value = save->save_block1[SB1_FLAGS + flag_id / 8u];
    *out_value = (value >> (flag_id % 8u)) & 1u;
    return 1;
}

int remaster_emerald_flag_set(
    RemasterEmeraldSave *save,
    uint16_t flag_id,
    int value)
{
    uint8_t *byte;
    uint8_t mask;

    if (save == 0
        || flag_id == 0
        || flag_id >= REMASTER_EMERALD_PERSISTENT_FLAG_COUNT)
        return 0;

    byte = &save->save_block1[SB1_FLAGS + flag_id / 8u];
    mask = (uint8_t)(1u << (flag_id % 8u));

    if (value)
        *byte |= mask;
    else
        *byte &= (uint8_t)~mask;

    return 1;
}

int remaster_emerald_var_get(
    const RemasterEmeraldSave *save,
    uint16_t var_id,
    uint16_t *out_value)
{
    uint16_t index;

    if (save == 0 || out_value == 0
        || var_id < REMASTER_EMERALD_VARS_START
        || var_id >= REMASTER_EMERALD_VARS_START + REMASTER_EMERALD_VAR_COUNT)
        return 0;

    index = (uint16_t)(var_id - REMASTER_EMERALD_VARS_START);
    *out_value = state_read_u16_le(
        save->save_block1 + SB1_VARS + (size_t)index * 2u);
    return 1;
}

int remaster_emerald_var_set(
    RemasterEmeraldSave *save,
    uint16_t var_id,
    uint16_t value)
{
    uint16_t index;

    if (save == 0
        || var_id < REMASTER_EMERALD_VARS_START
        || var_id >= REMASTER_EMERALD_VARS_START + REMASTER_EMERALD_VAR_COUNT)
        return 0;

    index = (uint16_t)(var_id - REMASTER_EMERALD_VARS_START);
    state_write_u16_le(
        save->save_block1 + SB1_VARS + (size_t)index * 2u,
        value);
    return 1;
}


void remaster_emerald_clear_temp_field_event_data(
    RemasterEmeraldSave *save)
{
    if (save == 0)
        return;

    /*
     * Vanilla:
     * - flags 0x00..0x1F are temporary (4 bytes)
     * - vars 0x4000..0x400F are temporary (16 u16 values / 32 bytes)
     * Both are cleared every time a map is loaded.
     */
    memset(save->save_block1 + SB1_FLAGS, 0, 4u);
    memset(save->save_block1 + SB1_VARS, 0, 16u * 2u);
}


static int read_warp_state(
    const RemasterEmeraldSave *save,
    size_t offset,
    RemasterEmeraldWarpState *out_warp)
{
    if (save == 0 || out_warp == 0)
        return 0;

    out_warp->map_group = (int8_t)save->save_block1[offset + 0u];
    out_warp->map_num = (int8_t)save->save_block1[offset + 1u];
    out_warp->warp_id = (int8_t)save->save_block1[offset + 2u];
    out_warp->x = state_read_s16_le(save->save_block1 + offset + 4u);
    out_warp->y = state_read_s16_le(save->save_block1 + offset + 6u);
    return 1;
}

static int write_warp_state(
    RemasterEmeraldSave *save,
    size_t offset,
    const RemasterEmeraldWarpState *warp)
{
    if (save == 0 || warp == 0)
        return 0;

    save->save_block1[offset + 0u] = (uint8_t)warp->map_group;
    save->save_block1[offset + 1u] = (uint8_t)warp->map_num;
    save->save_block1[offset + 2u] = (uint8_t)warp->warp_id;
    save->save_block1[offset + 3u] = 0u;
    state_write_s16_le(save->save_block1 + offset + 4u, warp->x);
    state_write_s16_le(save->save_block1 + offset + 6u, warp->y);
    return 1;
}

int remaster_emerald_player_gender_get(
    const RemasterEmeraldSave *save,
    uint8_t *out_gender)
{
    if (save == 0 || out_gender == 0)
        return 0;

    *out_gender = save->save_block2[SB2_PLAYER_GENDER];
    return 1;
}


int remaster_emerald_continue_game_warp_get(
    const RemasterEmeraldSave *save,
    RemasterEmeraldWarpState *out_warp)
{
    return read_warp_state(save, SB1_CONTINUE_GAME_WARP, out_warp);
}

int remaster_emerald_continue_game_warp_set(
    RemasterEmeraldSave *save,
    const RemasterEmeraldWarpState *warp)
{
    return write_warp_state(save, SB1_CONTINUE_GAME_WARP, warp);
}

int remaster_emerald_dynamic_warp_get(
    const RemasterEmeraldSave *save,
    RemasterEmeraldWarpState *out_warp)
{
    return read_warp_state(save, SB1_DYNAMIC_WARP, out_warp);
}

int remaster_emerald_dynamic_warp_set(
    RemasterEmeraldSave *save,
    const RemasterEmeraldWarpState *warp)
{
    return write_warp_state(save, SB1_DYNAMIC_WARP, warp);
}


int remaster_emerald_last_heal_warp_get(
    const RemasterEmeraldSave *save,
    RemasterEmeraldWarpState *out_warp)
{
    return read_warp_state(save, SB1_LAST_HEAL_WARP, out_warp);
}

int remaster_emerald_last_heal_warp_set(
    RemasterEmeraldSave *save,
    const RemasterEmeraldWarpState *warp)
{
    return write_warp_state(save, SB1_LAST_HEAL_WARP, warp);
}

int remaster_emerald_escape_warp_get(
    const RemasterEmeraldSave *save,
    RemasterEmeraldWarpState *out_warp)
{
    return read_warp_state(save, SB1_ESCAPE_WARP, out_warp);
}

int remaster_emerald_escape_warp_set(
    RemasterEmeraldSave *save,
    const RemasterEmeraldWarpState *warp)
{
    return write_warp_state(save, SB1_ESCAPE_WARP, warp);
}
