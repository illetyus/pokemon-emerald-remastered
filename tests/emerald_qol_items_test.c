#include "remaster/emerald_items.h"
#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_qol.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    ITEM_NONE = 0,
    ITEM_ORANGE_MAIL = 121,
    SB1_REGISTERED_ITEM = 0x0496,
    SB1_PC_ITEMS = 0x0498,
    SB1_VP5_META = 0x35D8,
    VP5_META_BYTES = 21,
    PC_ITEMS_COUNT = 50,
    ITEM_SLOT_BYTES = 4
};

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static void write16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static void write32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

static size_t collect_items(uint8_t pocket, uint16_t *out, size_t wanted)
{
    size_t found = 0;
    size_t item_id;

    for (item_id = 1;
         item_id < remaster_emerald_item_info_count() && found < wanted;
         ++item_id) {
        const RemasterEmeraldItemInfo *info =
            remaster_emerald_item_info((uint16_t)item_id);
        if (info != 0 && info->pocket == pocket)
            out[found++] = (uint16_t)item_id;
    }
    return found;
}

static int expected_before(
    uint16_t a,
    uint16_t b,
    RemasterEmeraldQolItemSortMode mode)
{
    const RemasterEmeraldItemInfo *ia = remaster_emerald_item_info(a);
    const RemasterEmeraldItemInfo *ib = remaster_emerald_item_info(b);
    uint32_t ka;
    uint32_t kb;

    assert(ia != 0 && ib != 0);
    switch (mode) {
    case REMASTER_EMERALD_QOL_ITEM_SORT_NAME:
        if (ia->name_sort_rank != ib->name_sort_rank)
            return ia->name_sort_rank < ib->name_sort_rank;
        break;
    case REMASTER_EMERALD_QOL_ITEM_SORT_TYPE:
        ka = ((uint32_t)ia->pocket << 8u) | ia->type;
        kb = ((uint32_t)ib->pocket << 8u) | ib->type;
        if (ka != kb)
            return ka < kb;
        break;
    case REMASTER_EMERALD_QOL_ITEM_SORT_VALUE:
        if (ia->price != ib->price)
            return ia->price > ib->price;
        break;
    default:
        break;
    }
    return a < b;
}

static void expected_sort3(
    uint16_t ids[3],
    RemasterEmeraldQolItemSortMode mode)
{
    size_t i;
    for (i = 1; i < 3; ++i) {
        uint16_t key = ids[i];
        size_t j = i;
        while (j > 0 && expected_before(key, ids[j - 1], mode)) {
            ids[j] = ids[j - 1];
            --j;
        }
        ids[j] = key;
    }
}

static RemasterEmeraldBoxPokemon make_box(uint16_t species)
{
    RemasterEmeraldBoxPokemon mon;
    memset(&mon, 0, sizeof(mon));
    mon.personality = 0x12345678u;
    mon.ot_id = 0x89ABCDEFu;
    assert(remaster_emerald_box_pokemon_set_species(&mon, species));
    return mon;
}

static void set_egg(RemasterEmeraldBoxPokemon *mon)
{
    uint32_t word =
        (uint32_t)mon->substruct[3][4]
        | ((uint32_t)mon->substruct[3][5] << 8u)
        | ((uint32_t)mon->substruct[3][6] << 16u)
        | ((uint32_t)mon->substruct[3][7] << 24u);
    word |= 1u << 30u;
    write32(&mon->substruct[3][4], word);
}

static void test_catalog_name_ranks(void)
{
    uint8_t seen[377] = {0};
    size_t i;

    assert(remaster_emerald_item_info_count() == 377);
    for (i = 0; i < remaster_emerald_item_info_count(); ++i) {
        const RemasterEmeraldItemInfo *info =
            remaster_emerald_item_info((uint16_t)i);
        assert(info != 0);
        assert(info->name_sort_rank < 377);
        assert(!seen[info->name_sort_rank]);
        seen[info->name_sort_rank] = 1;
    }
}

static void assert_bag_ids(
    const RemasterEmeraldSave *save,
    uint8_t pocket,
    const uint16_t *ids,
    size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        RemasterEmeraldItemSlot slot;
        assert(remaster_emerald_bag_slot_get(save, pocket, i, &slot));
        assert(slot.item_id == ids[i]);
    }
}

static void test_bag_sort_and_auto_sort(void)
{
    RemasterEmeraldSave save;
    uint16_t source[3];
    uint16_t expected[3];

    assert(collect_items(REMASTER_EMERALD_POCKET_ITEMS, source, 3) == 3);
    memset(&save, 0, sizeof(save));
    assert(remaster_emerald_bag_add(&save, source[0], 3));
    assert(remaster_emerald_bag_add(&save, source[1], 9));
    assert(remaster_emerald_bag_add(&save, source[2], 5));

    assert(remaster_emerald_qol_bag_sort(
        &save,
        REMASTER_EMERALD_POCKET_ITEMS,
        REMASTER_EMERALD_QOL_ITEM_SORT_QUANTITY));
    expected[0] = source[1];
    expected[1] = source[2];
    expected[2] = source[0];
    assert_bag_ids(&save, REMASTER_EMERALD_POCKET_ITEMS, expected, 3);

    memcpy(expected, source, sizeof(expected));
    expected_sort3(expected, REMASTER_EMERALD_QOL_ITEM_SORT_VALUE);
    assert(remaster_emerald_qol_bag_sort(
        &save,
        REMASTER_EMERALD_POCKET_ITEMS,
        REMASTER_EMERALD_QOL_ITEM_SORT_VALUE));
    assert_bag_ids(&save, REMASTER_EMERALD_POCKET_ITEMS, expected, 3);

    memcpy(expected, source, sizeof(expected));
    expected_sort3(expected, REMASTER_EMERALD_QOL_ITEM_SORT_TYPE);
    assert(remaster_emerald_qol_bag_sort(
        &save,
        REMASTER_EMERALD_POCKET_ITEMS,
        REMASTER_EMERALD_QOL_ITEM_SORT_TYPE));
    assert_bag_ids(&save, REMASTER_EMERALD_POCKET_ITEMS, expected, 3);

    memcpy(expected, source, sizeof(expected));
    expected_sort3(expected, REMASTER_EMERALD_QOL_ITEM_SORT_NAME);
    assert(remaster_emerald_qol_bag_sort(
        &save,
        REMASTER_EMERALD_POCKET_ITEMS,
        REMASTER_EMERALD_QOL_ITEM_SORT_NAME));
    assert_bag_ids(&save, REMASTER_EMERALD_POCKET_ITEMS, expected, 3);

    assert(remaster_emerald_bag_remove(&save, source[1], 9));
    assert(remaster_emerald_qol_bag_sort(
        &save,
        REMASTER_EMERALD_POCKET_ITEMS,
        REMASTER_EMERALD_QOL_ITEM_SORT_NAME));
    {
        RemasterEmeraldItemSlot slot;
        assert(remaster_emerald_bag_slot_get(
            &save, REMASTER_EMERALD_POCKET_ITEMS, 2, &slot));
        assert(slot.item_id == ITEM_NONE);
    }

    assert(!remaster_emerald_qol_bag_sort(
        &save,
        REMASTER_EMERALD_POCKET_TM_HM,
        REMASTER_EMERALD_QOL_ITEM_SORT_NAME));
    assert(!remaster_emerald_qol_bag_sort(
        &save,
        REMASTER_EMERALD_POCKET_BERRIES,
        REMASTER_EMERALD_QOL_ITEM_SORT_NAME));

    assert(remaster_emerald_qol_bag_set_sort_mode(
        &save,
        REMASTER_EMERALD_POCKET_ITEMS,
        REMASTER_EMERALD_QOL_ITEM_SORT_VALUE));
    assert(remaster_emerald_qol_bag_sort_mode(
        &save, REMASTER_EMERALD_POCKET_ITEMS)
        == REMASTER_EMERALD_QOL_ITEM_SORT_VALUE);
    assert(remaster_emerald_qol_bag_set_auto_sort_enabled(
        &save, REMASTER_EMERALD_POCKET_ITEMS, 1));
    assert(remaster_emerald_qol_bag_auto_sort_enabled(
        &save, REMASTER_EMERALD_POCKET_ITEMS));
    assert(remaster_emerald_qol_bag_auto_sort(
        &save, REMASTER_EMERALD_POCKET_ITEMS));
    assert(!remaster_emerald_qol_bag_set_auto_sort_enabled(
        &save, REMASTER_EMERALD_POCKET_TM_HM, 1));
    assert(!remaster_emerald_qol_bag_set_auto_sort_enabled(
        &save, REMASTER_EMERALD_POCKET_BERRIES, 1));
}

static void set_pc_slot(
    RemasterEmeraldSave *save,
    size_t slot,
    uint16_t item_id,
    uint16_t quantity)
{
    uint8_t *raw;
    assert(slot < PC_ITEMS_COUNT);
    raw = save->save_block1 + SB1_PC_ITEMS + slot * ITEM_SLOT_BYTES;
    write16(raw, item_id);
    write16(raw + 2, quantity);
}

static uint16_t pc_slot_item(const RemasterEmeraldSave *save, size_t slot)
{
    assert(slot < PC_ITEMS_COUNT);
    return read16(save->save_block1 + SB1_PC_ITEMS + slot * ITEM_SLOT_BYTES);
}

static uint16_t pc_slot_quantity(const RemasterEmeraldSave *save, size_t slot)
{
    assert(slot < PC_ITEMS_COUNT);
    return read16(
        save->save_block1 + SB1_PC_ITEMS + slot * ITEM_SLOT_BYTES + 2);
}

static void test_pc_item_sort(void)
{
    RemasterEmeraldSave save;
    uint16_t ids[3];

    assert(collect_items(REMASTER_EMERALD_POCKET_ITEMS, ids, 3) == 3);
    memset(&save, 0, sizeof(save));
    set_pc_slot(&save, 0, ids[0], 2);
    set_pc_slot(&save, 2, ids[1], 9);
    set_pc_slot(&save, 4, ids[2], 5);

    assert(remaster_emerald_qol_pc_items_sort(
        &save, REMASTER_EMERALD_QOL_ITEM_SORT_QUANTITY));
    assert(pc_slot_item(&save, 0) == ids[1]);
    assert(pc_slot_quantity(&save, 0) == 9);
    assert(pc_slot_item(&save, 1) == ids[2]);
    assert(pc_slot_quantity(&save, 1) == 5);
    assert(pc_slot_item(&save, 2) == ids[0]);
    assert(pc_slot_quantity(&save, 2) == 2);
    assert(pc_slot_item(&save, 3) == ITEM_NONE);
}

static void test_pc_held_item_give_take(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldBoxPokemon mon;
    RemasterEmeraldBoxPokemon out;
    uint16_t ordinary[30];
    uint16_t key_item[1];
    uint16_t taken = ITEM_NONE;
    size_t i;

    assert(collect_items(REMASTER_EMERALD_POCKET_ITEMS, ordinary, 30) == 30);
    assert(collect_items(REMASTER_EMERALD_POCKET_KEY_ITEMS, key_item, 1) == 1);

    memset(&save, 0, sizeof(save));
    mon = make_box(1);
    assert(remaster_emerald_storage_set(&save, 0, 0, &mon));
    assert(remaster_emerald_bag_add(&save, ordinary[0], 1));
    assert(remaster_emerald_qol_pc_give_held_item(&save, 0, ordinary[0])
        == REMASTER_EMERALD_QOL_HELD_ITEM_OK);
    assert(remaster_emerald_bag_count(&save, ordinary[0]) == 0);
    assert(remaster_emerald_storage_get(&save, 0, 0, &out, 0));
    assert(remaster_emerald_box_pokemon_held_item(&out) == ordinary[0]);
    assert(remaster_emerald_qol_pc_give_held_item(&save, 0, ordinary[1])
        == REMASTER_EMERALD_QOL_HELD_ITEM_ALREADY_HELD);

    assert(remaster_emerald_qol_pc_take_held_item(&save, 0, &taken)
        == REMASTER_EMERALD_QOL_HELD_ITEM_OK);
    assert(taken == ordinary[0]);
    assert(remaster_emerald_bag_count(&save, ordinary[0]) == 1);
    assert(remaster_emerald_storage_get(&save, 0, 0, &out, 0));
    assert(remaster_emerald_box_pokemon_held_item(&out) == ITEM_NONE);

    assert(remaster_emerald_bag_add(&save, key_item[0], 1));
    assert(remaster_emerald_qol_pc_give_held_item(&save, 0, key_item[0])
        == REMASTER_EMERALD_QOL_HELD_ITEM_UNHOLDABLE);
    assert(remaster_emerald_qol_pc_give_held_item(&save, 0, ITEM_ORANGE_MAIL)
        == REMASTER_EMERALD_QOL_HELD_ITEM_MAIL);

    mon = make_box(1);
    set_egg(&mon);
    assert(remaster_emerald_storage_set(&save, 0, 0, &mon));
    assert(remaster_emerald_qol_pc_give_held_item(&save, 0, ordinary[0])
        == REMASTER_EMERALD_QOL_HELD_ITEM_EGG);

    memset(&save, 0, sizeof(save));
    mon = make_box(1);
    assert(remaster_emerald_box_pokemon_set_held_item(&mon, ITEM_ORANGE_MAIL));
    assert(remaster_emerald_storage_set(&save, 0, 0, &mon));
    assert(remaster_emerald_qol_pc_take_held_item(&save, 0, 0)
        == REMASTER_EMERALD_QOL_HELD_ITEM_MAIL);
    assert(remaster_emerald_storage_get(&save, 0, 0, &out, 0));
    assert(remaster_emerald_box_pokemon_held_item(&out) == ITEM_ORANGE_MAIL);

    memset(&save, 0, sizeof(save));
    for (i = 0; i < 30; ++i)
        assert(remaster_emerald_bag_add(&save, ordinary[i], 99));
    mon = make_box(1);
    assert(remaster_emerald_box_pokemon_set_held_item(&mon, ordinary[0]));
    assert(remaster_emerald_storage_set(&save, 0, 0, &mon));
    assert(remaster_emerald_qol_pc_take_held_item(&save, 0, 0)
        == REMASTER_EMERALD_QOL_HELD_ITEM_BAG_FULL);
    assert(remaster_emerald_bag_count(&save, ordinary[0]) == 99);
    assert(remaster_emerald_storage_get(&save, 0, 0, &out, 0));
    assert(remaster_emerald_box_pokemon_held_item(&out) == ordinary[0]);
}

static void test_vp5_quick_item_metadata(void)
{
    RemasterEmeraldSave save;
    uint16_t ids[5];
    uint8_t *meta;

    assert(collect_items(REMASTER_EMERALD_POCKET_ITEMS, ids, 5) == 5);
    memset(&save, 0, sizeof(save));
    meta = save.save_block1 + SB1_VP5_META;
    meta[VP5_META_BYTES + 0] = 0xA5;
    meta[VP5_META_BYTES + 1] = 0x5A;
    meta[VP5_META_BYTES + 2] = 0xC3;
    write16(save.save_block1 + SB1_REGISTERED_ITEM, ids[0]);
    assert(remaster_emerald_bag_add(&save, ids[0], 1));

    assert(remaster_emerald_qol_quick_item_count(&save) == 1);
    assert(remaster_emerald_qol_quick_item_get(&save, 0) == ids[0]);
    assert(meta[VP5_META_BYTES + 0] == 0xA5);
    assert(meta[VP5_META_BYTES + 1] == 0x5A);
    assert(meta[VP5_META_BYTES + 2] == 0xC3);

    assert(remaster_emerald_qol_quick_item_register(&save, ids[0]));
    assert(remaster_emerald_qol_quick_item_count(&save) == 1);
    assert(remaster_emerald_qol_quick_item_register(&save, ids[1]));
    assert(remaster_emerald_qol_quick_item_register(&save, ids[2]));
    assert(remaster_emerald_qol_quick_item_register(&save, ids[3]));
    assert(remaster_emerald_qol_quick_item_count(&save) == 4);
    assert(!remaster_emerald_qol_quick_item_register(&save, ids[4]));
    assert(read16(save.save_block1 + SB1_REGISTERED_ITEM) == ids[0]);

    assert(remaster_emerald_qol_quick_item_unregister(&save, ids[0]));
    assert(remaster_emerald_qol_quick_item_count(&save) == 3);
    assert(read16(save.save_block1 + SB1_REGISTERED_ITEM)
        == remaster_emerald_qol_quick_item_get(&save, 0));

    memset(&save, 0, sizeof(save));
    write16(save.save_block1 + SB1_REGISTERED_ITEM, ids[0]);
    assert(remaster_emerald_bag_add(&save, ids[0], 1));
    assert(remaster_emerald_qol_quick_item_count(&save) == 1);
    assert(remaster_emerald_qol_quick_item_register(&save, ids[1]));
    remaster_emerald_qol_quick_items_prune(&save);
    assert(remaster_emerald_qol_quick_item_count(&save) == 1);
    assert(remaster_emerald_qol_quick_item_get(&save, 0) == ids[0]);
    assert(read16(save.save_block1 + SB1_REGISTERED_ITEM) == ids[0]);
}

int main(void)
{
    test_catalog_name_ranks();
    test_bag_sort_and_auto_sort();
    test_pc_item_sort();
    test_pc_held_item_give_take();
    test_vp5_quick_item_metadata();
    puts("R16 item/Bag/PC QoL tests passed");
    return 0;
}
