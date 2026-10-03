#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_state_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldOverworldState input;
    RemasterEmeraldOverworldState output;
    uint16_t value;
    int flag;
    RemasterEmeraldWarpState continue_warp;
    RemasterEmeraldWarpState dynamic_warp;
    RemasterEmeraldWarpState heal_warp;
    RemasterEmeraldWarpState escape_warp;
    RemasterEmeraldWarpState observed_warp;

    memset(&save, 0, sizeof(save));
    memset(&input, 0, sizeof(input));
    memset(&continue_warp, 0, sizeof(continue_warp));
    memset(&dynamic_warp, 0, sizeof(dynamic_warp));
    memset(&heal_warp, 0, sizeof(heal_warp));
    memset(&escape_warp, 0, sizeof(escape_warp));
    memset(&observed_warp, 0, sizeof(observed_warp));

    /* SaveBlock2 encryptionKey = 0x12345678. */
    save.save_block2[0xAC] = 0x78;
    save.save_block2[0xAD] = 0x56;
    save.save_block2[0xAE] = 0x34;
    save.save_block2[0xAF] = 0x12;

    input.player_x = 14;
    input.player_y = 27;
    input.map_group = 3;
    input.map_num = 9;
    input.warp_id = 2;
    input.warp_x = 6;
    input.warp_y = 11;
    input.map_layout_id = 0x123;
    input.saved_music = 321;
    input.weather = 5;
    input.weather_cycle_stage = 3;
    input.flash_level = 2;
    input.party_count = 6;
    input.money = 999999;
    input.coins = 4321;
    input.registered_item = 77;

    if (!check(
            remaster_emerald_overworld_set(&save, &input),
            "overworld write failed"))
        return 1;

    memset(&output, 0, sizeof(output));
    if (!check(
            remaster_emerald_overworld_get(&save, &output),
            "overworld read failed"))
        return 1;

    if (!check(
            output.player_x == input.player_x
            && output.player_y == input.player_y
            && output.map_group == input.map_group
            && output.map_num == input.map_num
            && output.warp_id == input.warp_id
            && output.warp_x == input.warp_x
            && output.warp_y == input.warp_y,
            "map/player state mismatch"))
        return 1;

    if (!check(
            output.map_layout_id == input.map_layout_id
            && output.saved_music == input.saved_music
            && output.weather == input.weather
            && output.weather_cycle_stage == input.weather_cycle_stage
            && output.flash_level == input.flash_level
            && output.party_count == input.party_count,
            "overworld metadata mismatch"))
        return 1;

    if (!check(
            output.money == 999999
            && output.coins == 4321
            && output.registered_item == 77,
            "currency/item state mismatch"))
        return 1;


    continue_warp.map_group = 1;
    continue_warp.map_num = 2;
    continue_warp.warp_id = 3;
    continue_warp.x = 4;
    continue_warp.y = 5;

    dynamic_warp.map_group = 6;
    dynamic_warp.map_num = 7;
    dynamic_warp.warp_id = 1;
    dynamic_warp.x = 8;
    dynamic_warp.y = 9;

    heal_warp.map_group = 10;
    heal_warp.map_num = 11;
    heal_warp.warp_id = 2;
    heal_warp.x = 12;
    heal_warp.y = 13;

    escape_warp.map_group = 14;
    escape_warp.map_num = 15;
    escape_warp.warp_id = -1;
    escape_warp.x = 16;
    escape_warp.y = 17;

    if (!check(
            remaster_emerald_continue_game_warp_set(&save, &continue_warp)
            && remaster_emerald_dynamic_warp_set(&save, &dynamic_warp)
            && remaster_emerald_last_heal_warp_set(&save, &heal_warp)
            && remaster_emerald_escape_warp_set(&save, &escape_warp),
            "persistent warp writes failed"))
        return 1;

    if (!check(
            remaster_emerald_continue_game_warp_get(&save, &observed_warp)
            && observed_warp.map_group == 1
            && observed_warp.map_num == 2
            && observed_warp.warp_id == 3
            && observed_warp.x == 4
            && observed_warp.y == 5,
            "continue-game warp mismatch"))
        return 1;

    if (!check(
            remaster_emerald_dynamic_warp_get(&save, &observed_warp)
            && observed_warp.map_group == 6
            && observed_warp.map_num == 7
            && observed_warp.warp_id == 1
            && observed_warp.x == 8
            && observed_warp.y == 9,
            "dynamic warp mismatch"))
        return 1;

    if (!check(
            remaster_emerald_last_heal_warp_get(&save, &observed_warp)
            && observed_warp.map_group == 10
            && observed_warp.map_num == 11
            && observed_warp.warp_id == 2
            && observed_warp.x == 12
            && observed_warp.y == 13,
            "last-heal warp mismatch"))
        return 1;

    if (!check(
            remaster_emerald_escape_warp_get(&save, &observed_warp)
            && observed_warp.map_group == 14
            && observed_warp.map_num == 15
            && observed_warp.warp_id == -1
            && observed_warp.x == 16
            && observed_warp.y == 17,
            "escape warp mismatch"))
        return 1;

    if (!check(remaster_emerald_flag_set(&save, 0x123, 1), "flag set failed"))
        return 1;
    if (!check(remaster_emerald_flag_get(&save, 0x123, &flag), "flag get failed"))
        return 1;
    if (!check(flag == 1, "flag did not persist"))
        return 1;
    if (!check(remaster_emerald_flag_set(&save, 0x123, 0), "flag clear failed"))
        return 1;
    if (!check(remaster_emerald_flag_get(&save, 0x123, &flag) && flag == 0, "flag clear mismatch"))
        return 1;

    if (!check(
            remaster_emerald_var_set(&save, 0x405A, 0xBEEF),
            "var set failed"))
        return 1;
    if (!check(
            remaster_emerald_var_get(&save, 0x405A, &value)
            && value == 0xBEEF,
            "var get mismatch"))
        return 1;

    if (!check(
            !remaster_emerald_flag_get(&save, 0, &flag),
            "flag zero should remain invalid"))
        return 1;
    if (!check(
            !remaster_emerald_var_get(&save, 0x3FFF, &value),
            "pre-VARS_START id should be rejected"))
        return 1;

    if (!check(remaster_emerald_flag_set(&save, 0x001F, 1), "temp flag set failed"))
        return 1;
    if (!check(remaster_emerald_flag_set(&save, 0x0020, 1), "persistent flag set failed"))
        return 1;
    if (!check(remaster_emerald_var_set(&save, 0x4000, 0xAAAA), "temp var set failed"))
        return 1;
    if (!check(remaster_emerald_var_set(&save, 0x4010, 0xBBBB), "persistent var set failed"))
        return 1;

    remaster_emerald_clear_temp_field_event_data(&save);

    if (!check(
            remaster_emerald_flag_get(&save, 0x001F, &flag) && flag == 0
            && remaster_emerald_flag_get(&save, 0x0020, &flag) && flag == 1
            && remaster_emerald_var_get(&save, 0x4000, &value) && value == 0
            && remaster_emerald_var_get(&save, 0x4010, &value) && value == 0xBBBB,
            "temporary event data reset mismatch"))
        return 1;

    puts("Emerald overworld state compatibility test passed.");
    return 0;
}
