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

    memset(&save, 0, sizeof(save));
    memset(&input, 0, sizeof(input));

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
    input.weather = 5;
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
            && output.weather == input.weather
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

    puts("Emerald overworld state compatibility test passed.");
    return 0;
}
