#include "remaster/emerald_items.h"
#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_qol.h"
#include "remaster/emerald_state.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    ITEM_POTION = 13,
    ITEM_ANTIDOTE = 14,
    ITEM_SUN_STONE = 93,
    ITEM_MOON_STONE = 94,
    ITEM_FIRE_STONE = 95,
    ITEM_THUNDER_STONE = 96,
    ITEM_WATER_STONE = 97,
    ITEM_LEAF_STONE = 98,
    ITEM_ORANGE_MAIL = 121,
    ITEM_KINGS_ROCK = 187,
    ITEM_TM01 = 289,
    ITEM_TM50 = 338,
    ITEM_HM01 = 339,
    ITEM_HM03 = 341,
    ITEM_HM08 = 346,
    MOVE_CUT = 15,
    MOVE_SURF = 57,
    FLAG_BADGE01_GET = 0x867,
    FLAG_BADGE05_GET = 0x86B,
    TYPE_FIGHTING = 1,
    TYPE_DARK = 17,
    SB1_REGISTERED_ITEM = 0x0496,
    STORAGE_BOXES = 0x0004
};

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

static RemasterEmeraldBoxPokemon make_box(
    uint16_t species, uint8_t level, uint32_t personality)
{
    RemasterEmeraldBoxPokemon mon;
    const RemasterEmeraldSpeciesInfo *info;

    memset(&mon, 0, sizeof(mon));
    mon.personality = personality;
    mon.ot_id = 0x12345678u;
    assert(remaster_emerald_box_pokemon_set_species(&mon, species));
    info = remaster_emerald_species_info(species);
    assert(info != 0);
    assert(remaster_emerald_box_pokemon_set_experience(
        &mon,
        remaster_emerald_experience_for_level(info->growth_rate, level)));
    return mon;
}

static void set_egg(RemasterEmeraldBoxPokemon *mon)
{
    uint32_t word =
        (uint32_t)mon->substruct[3][4]
        | ((uint32_t)mon->substruct[3][5] << 8u)
        | ((uint32_t)mon->substruct[3][6] << 16u)
        | ((uint32_t)mon->substruct[3][7] << 24u);
    word |= (1u << 30u);
    write32(&mon->substruct[3][4], word);
}

static RemasterEmeraldPartyPokemon make_party(
    uint16_t species, uint8_t level, uint16_t hp, uint32_t personality)
{
    RemasterEmeraldPartyPokemon mon;
    memset(&mon, 0, sizeof(mon));
    mon.box = make_box(species, level, personality);
    mon.level = level;
    mon.hp = hp;
    mon.max_hp = hp;
    mon.mail = 0xFFu;
    return mon;
}

static void test_policy(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldQolHmTool tool;
    const uint16_t stones[] = {
        ITEM_SUN_STONE, ITEM_MOON_STONE, ITEM_FIRE_STONE,
        ITEM_THUNDER_STONE, ITEM_WATER_STONE, ITEM_LEAF_STONE
    };
    size_t i;

    for (i = 0; i < sizeof(stones) / sizeof(stones[0]); ++i) {
        assert(remaster_emerald_qol_reusable_evolution_stone(stones[i]));
        assert(!remaster_emerald_qol_evolution_item_consumed(stones[i]));
    }
    assert(!remaster_emerald_qol_reusable_evolution_stone(ITEM_KINGS_ROCK));
    assert(remaster_emerald_qol_evolution_item_consumed(ITEM_KINGS_ROCK));
    assert(!remaster_emerald_qol_teach_item_consumed(ITEM_TM01));
    assert(!remaster_emerald_qol_teach_item_consumed(ITEM_TM50));
    assert(!remaster_emerald_qol_teach_item_consumed(ITEM_HM01));
    assert(!remaster_emerald_qol_teach_item_consumed(ITEM_HM08));
    assert(remaster_emerald_qol_teach_item_consumed(ITEM_POTION));
    assert(remaster_emerald_qol_move_replaceable(MOVE_SURF));

    assert(remaster_emerald_qol_field_poison_hp_after_step(10) == 9);
    assert(remaster_emerald_qol_field_poison_hp_after_step(1) == 1);
    assert(remaster_emerald_qol_flash_level_after_use() == 0);
    assert(remaster_emerald_qol_fishing_response_frames(0) == 90);
    assert(remaster_emerald_qol_fishing_response_frames(2) == 90);
    assert(remaster_emerald_qol_fishing_reaction_rounds(1) == 0);
    assert(remaster_emerald_qol_bike_toggle_allowed(1));
    assert(!remaster_emerald_qol_bike_toggle_allowed(0));
    assert(remaster_emerald_qol_running_allowed());

    assert(remaster_emerald_qol_hm_tool(ITEM_HM01, &tool));
    assert(tool.move_id == MOVE_CUT);
    assert(tool.badge_flag == FLAG_BADGE01_GET);
    assert(remaster_emerald_qol_hm_tool(ITEM_HM03, &tool));
    assert(tool.move_id == MOVE_SURF);
    assert(tool.badge_flag == FLAG_BADGE05_GET);

    memset(&save, 0, sizeof(save));
    assert(remaster_emerald_bag_add(&save, ITEM_HM03, 1));
    assert(!remaster_emerald_qol_hm_access(&save, ITEM_HM03));
    assert(remaster_emerald_flag_set(&save, FLAG_BADGE05_GET, 1));
    assert(remaster_emerald_qol_hm_access(&save, ITEM_HM03));
}

static void test_info(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldBoxPokemon mon;
    uint8_t evs[6] = {252, 0, 0, 252, 4, 0};
    size_t i;

    memset(&save, 0, sizeof(save));
    mon = make_box(25, 50, 7);
    for (i = 0; i < 6; ++i) {
        assert(remaster_emerald_box_pokemon_set_ev(&mon, i, evs[i]));
        assert(remaster_emerald_box_pokemon_set_iv(&mon, i, 31));
    }
    assert(remaster_emerald_qol_total_evs(&mon) == 508);
    assert(remaster_emerald_qol_hidden_power_type(&mon) == TYPE_DARK);
    for (i = 0; i < 6; ++i)
        assert(remaster_emerald_box_pokemon_set_iv(&mon, i, 0));
    assert(remaster_emerald_qol_hidden_power_type(&mon) == TYPE_FIGHTING);

    memcpy(save.save_block2, "PLAYER", 6);
    save.save_block2[6] = 0xFF;
    save.save_block2[0x0A] = 0x78;
    save.save_block2[0x0B] = 0x56;
    save.save_block2[0x0C] = 0x34;
    save.save_block2[0x0D] = 0x12;
    memcpy(mon.ot_name, save.save_block2, 7);
    mon.ot_id = 0x12345678u;
    assert(remaster_emerald_qol_nickname_owned_by_player(&save, &mon));
    mon.ot_id ^= 1u;
    assert(!remaster_emerald_qol_nickname_owned_by_player(&save, &mon));
    mon.ot_id ^= 1u;
    set_egg(&mon);
    assert(remaster_emerald_qol_box_pokemon_is_egg(&mon));
    assert(!remaster_emerald_qol_nickname_owned_by_player(&save, &mon));
}

static void test_storage(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldBoxPokemon a, b, c, egg, out;
    uint8_t raw_a[REMASTER_EMERALD_BOX_POKEMON_BYTES];
    int checksum_ok;

    memset(&save, 0, sizeof(save));
    a = make_box(25, 10, 101);
    b = make_box(1, 30, 202);
    c = make_box(277, 20, 303);
    egg = make_box(2, 5, 404);
    set_egg(&egg);
    assert(remaster_emerald_storage_set(&save, 0, 0, &a));
    memcpy(raw_a, save.pokemon_storage + STORAGE_BOXES, sizeof(raw_a));
    assert(remaster_emerald_storage_set(&save, 0, 1, &egg));
    assert(remaster_emerald_storage_set(&save, 0, 2, &c));
    assert(remaster_emerald_storage_set(&save, 0, 3, &b));

    assert(remaster_emerald_qol_storage_sort_current_box(
        &save, REMASTER_EMERALD_QOL_BOX_SORT_SPECIES));
    assert(remaster_emerald_storage_get(&save, 0, 0, &out, &checksum_ok));
    assert(remaster_emerald_box_pokemon_species(&out) == 1);
    assert(remaster_emerald_storage_get(&save, 0, 1, &out, &checksum_ok));
    assert(remaster_emerald_box_pokemon_species(&out) == 25);
    assert(memcmp(
        raw_a,
        save.pokemon_storage + STORAGE_BOXES
            + REMASTER_EMERALD_BOX_POKEMON_BYTES,
        sizeof(raw_a)) == 0);
    assert(remaster_emerald_storage_get(&save, 0, 2, &out, &checksum_ok));
    assert(remaster_emerald_box_pokemon_species(&out) == 277);
    assert(remaster_emerald_storage_get(&save, 0, 3, &out, &checksum_ok));
    assert(remaster_emerald_qol_box_pokemon_is_egg(&out));

    memset(&save, 0, sizeof(save));
    assert(remaster_emerald_storage_set(&save, 0, 0, &a));
    assert(remaster_emerald_storage_set(&save, 0, 3, &b));
    assert(remaster_emerald_storage_set(&save, 0, 7, &c));
    assert(remaster_emerald_qol_storage_compact_current_box(&save));
    assert(remaster_emerald_storage_get(&save, 0, 0, &out, &checksum_ok));
    assert(out.personality == 101);
    assert(remaster_emerald_storage_get(&save, 0, 1, &out, &checksum_ok));
    assert(out.personality == 202);
    assert(remaster_emerald_storage_get(&save, 0, 2, &out, &checksum_ok));
    assert(out.personality == 303);
}

static void test_transfer_and_items(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldPartyPokemon p0, p1, out_party;
    RemasterEmeraldBoxPokemon boxed, a, b;
    int checksum_ok;

    memset(&save, 0, sizeof(save));
    p0 = make_party(1, 20, 50, 501);
    p1 = make_party(25, 20, 50, 502);
    assert(remaster_emerald_party_set(&save, 0, &p0));
    assert(remaster_emerald_party_set(&save, 1, &p1));
    assert(remaster_emerald_party_set_count(&save, 2));
    assert(remaster_emerald_qol_quick_deposit_current_box(&save, 1)
        == REMASTER_EMERALD_QOL_TRANSFER_OK);
    assert(remaster_emerald_party_count(&save) == 1);
    assert(remaster_emerald_storage_get(&save, 0, 0, &boxed, &checksum_ok));
    assert(remaster_emerald_box_pokemon_species(&boxed) == 25);
    assert(remaster_emerald_qol_quick_deposit_current_box(&save, 0)
        == REMASTER_EMERALD_QOL_TRANSFER_LAST_USABLE);

    assert(remaster_emerald_qol_quick_withdraw_current_box(&save, 0)
        == REMASTER_EMERALD_QOL_TRANSFER_OK);
    assert(remaster_emerald_party_count(&save) == 2);
    assert(remaster_emerald_party_get(&save, 1, &out_party, &checksum_ok));
    assert(remaster_emerald_box_pokemon_species(&out_party.box) == 25);
    assert(out_party.hp == out_party.max_hp);
    assert(out_party.mail == 0xFFu);

    memset(&save, 0, sizeof(save));
    p0 = make_party(1, 20, 50, 601);
    p1 = make_party(25, 20, 50, 602);
    assert(remaster_emerald_box_pokemon_set_held_item(&p1.box, ITEM_ORANGE_MAIL));
    assert(remaster_emerald_party_set(&save, 0, &p0));
    assert(remaster_emerald_party_set(&save, 1, &p1));
    assert(remaster_emerald_party_set_count(&save, 2));
    assert(remaster_emerald_qol_quick_deposit_current_box(&save, 1)
        == REMASTER_EMERALD_QOL_TRANSFER_MAIL);

    a = make_box(1, 10, 701);
    b = make_box(25, 10, 702);
    assert(remaster_emerald_box_pokemon_set_held_item(&a, ITEM_POTION));
    assert(remaster_emerald_box_pokemon_set_held_item(&b, ITEM_ANTIDOTE));
    assert(remaster_emerald_qol_swap_held_items(&a, &b));
    assert(remaster_emerald_box_pokemon_held_item(&a) == ITEM_ANTIDOTE);
    assert(remaster_emerald_box_pokemon_held_item(&b) == ITEM_POTION);
    assert(remaster_emerald_box_pokemon_set_held_item(&a, ITEM_ORANGE_MAIL));
    assert(!remaster_emerald_qol_swap_held_items(&a, &b));
}

static void test_quick_items(void)
{
    RemasterEmeraldSave save;

    memset(&save, 0, sizeof(save));
    write16(save.save_block1 + SB1_REGISTERED_ITEM, ITEM_POTION);
    assert(remaster_emerald_bag_add(&save, ITEM_POTION, 1));
    assert(remaster_emerald_qol_quick_item_count(&save) == 1);
    assert(remaster_emerald_qol_quick_item_get(&save, 0) == ITEM_POTION);
    assert(remaster_emerald_qol_quick_item_register(&save, ITEM_ANTIDOTE));
    assert(remaster_emerald_qol_quick_item_count(&save) == 2);
    remaster_emerald_qol_quick_items_prune(&save);
    assert(remaster_emerald_qol_quick_item_count(&save) == 1);
    assert(remaster_emerald_qol_quick_item_unregister(&save, ITEM_POTION));
    assert(remaster_emerald_qol_quick_item_count(&save) == 0);
}

int main(void)
{
    test_policy();
    test_info();
    test_storage();
    test_transfer_and_items();
    test_quick_items();
    puts("R16 QoL behavior tests passed");
    return 0;
}
