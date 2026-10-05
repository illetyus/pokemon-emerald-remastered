#include "remaster/emerald_items.h"
#include "remaster/emerald_save.h"

#include <stdio.h>
#include <string.h>

enum {
    SB2_ENCRYPTION_KEY = 0x00AC,
    SB1_ITEMS = 0x0560,
    SB1_BALLS = 0x0650,
    SB1_TMHM = 0x0690,
    SB1_BERRIES = 0x0790,
    SB1_KEY_ITEMS = 0x05D8
};

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_bag_test: %s\n", message);
        return 0;
    }
    return 1;
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

static uint16_t get16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldSave before;
    RemasterEmeraldItemSlot slot;
    const RemasterEmeraldItemInfo *info;
    const uint16_t key = 0xC3D4;

    memset(&save, 0, sizeof(save));
    put32(save.save_block2 + SB2_ENCRYPTION_KEY, UINT32_C(0xA1B2C3D4));

    if (!check(
            remaster_emerald_bag_pocket_capacity(
                REMASTER_EMERALD_POCKET_ITEMS) == 30
            && remaster_emerald_bag_pocket_capacity(
                REMASTER_EMERALD_POCKET_KEY_ITEMS) == 30
            && remaster_emerald_bag_pocket_capacity(
                REMASTER_EMERALD_POCKET_POKE_BALLS) == 16
            && remaster_emerald_bag_pocket_capacity(
                REMASTER_EMERALD_POCKET_TM_HM) == 64
            && remaster_emerald_bag_pocket_capacity(
                REMASTER_EMERALD_POCKET_BERRIES) == 46,
            "bag pocket capacities mismatch"))
        return 1;

    info = remaster_emerald_item_info(13); /* Potion */
    if (!check(
            info != 0 && info->pocket == REMASTER_EMERALD_POCKET_ITEMS,
            "Potion pocket mismatch"))
        return 1;

    info = remaster_emerald_item_info(4); /* Poke Ball */
    if (!check(
            info != 0 && info->pocket == REMASTER_EMERALD_POCKET_POKE_BALLS,
            "Poke Ball pocket mismatch"))
        return 1;

    info = remaster_emerald_item_info(289); /* TM01 */
    if (!check(
            info != 0 && info->pocket == REMASTER_EMERALD_POCKET_TM_HM,
            "TM01 pocket mismatch"))
        return 1;

    info = remaster_emerald_item_info(133); /* Cheri Berry */
    if (!check(
            info != 0 && info->pocket == REMASTER_EMERALD_POCKET_BERRIES,
            "Cheri Berry pocket mismatch"))
        return 1;

    info = remaster_emerald_item_info(259); /* Mach Bike */
    if (!check(
            info != 0 && info->pocket == REMASTER_EMERALD_POCKET_KEY_ITEMS,
            "Mach Bike pocket mismatch"))
        return 1;

    if (!check(
            remaster_emerald_bag_add(&save, 13, 100),
            "100 Potions should split across normal-item slots"))
        return 1;

    if (!check(
            remaster_emerald_bag_count(&save, 13) == 100,
            "Potion count after split add mismatch"))
        return 1;

    if (!check(
            remaster_emerald_bag_slot_get(
                &save,
                REMASTER_EMERALD_POCKET_ITEMS,
                0,
                &slot)
            && slot.item_id == 13
            && slot.quantity == 99,
            "first Potion slot mismatch"))
        return 1;

    if (!check(
            remaster_emerald_bag_slot_get(
                &save,
                REMASTER_EMERALD_POCKET_ITEMS,
                1,
                &slot)
            && slot.item_id == 13
            && slot.quantity == 1,
            "second Potion slot mismatch"))
        return 1;

    if (!check(
            get16(save.save_block1 + SB1_ITEMS) == 13
            && get16(save.save_block1 + SB1_ITEMS + 2)
                == (uint16_t)(99u ^ key),
            "bag quantity must be stored XOR-encrypted with SaveBlock2 key"))
        return 1;

    if (!check(
            remaster_emerald_bag_remove(&save, 13, 60)
            && remaster_emerald_bag_count(&save, 13) == 40,
            "Potion removal mismatch"))
        return 1;

    before = save;
    if (!check(
            !remaster_emerald_bag_add(&save, 133, 1000),
            "1000 berries must exceed single berry-slot capacity"))
        return 1;

    if (!check(
            memcmp(&before, &save, sizeof(save)) == 0,
            "failed berry add must be atomic"))
        return 1;

    before = save;
    if (!check(
            !remaster_emerald_bag_add(&save, 289, 100),
            "TM pocket must not split one TM into multiple slots"))
        return 1;

    if (!check(
            memcmp(&before, &save, sizeof(save)) == 0,
            "failed TM add must be atomic"))
        return 1;

    if (!check(
            remaster_emerald_bag_add(&save, 4, 12)
            && remaster_emerald_bag_add(&save, 133, 999)
            && remaster_emerald_bag_add(&save, 259, 1),
            "representative pocket adds failed"))
        return 1;

    if (!check(
            get16(save.save_block1 + SB1_BALLS) == 4
            && get16(save.save_block1 + SB1_TMHM) == 0
            && get16(save.save_block1 + SB1_BERRIES) == 133
            && get16(save.save_block1 + SB1_KEY_ITEMS) == 259,
            "bag pocket save offsets mismatch"))
        return 1;

    if (!check(
            !remaster_emerald_bag_remove(&save, 4, 13),
            "removing more Poke Balls than owned must fail"))
        return 1;

    puts("R11 bag encryption/capacity regression passed.");
    return 0;
}
