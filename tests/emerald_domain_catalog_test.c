#include "remaster/emerald_items.h"
#include "remaster/emerald_pokemon.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_domain_catalog_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    const RemasterEmeraldSpeciesInfo *species;
    const RemasterEmeraldMoveInfo *move;
    const RemasterEmeraldItemInfo *item;
    const RemasterEmeraldEvolution *evolution;
    const RemasterEmeraldLevelUpMove *learnset;
    size_t learnset_count = 0;
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[6] = {31, 31, 31, 31, 31, 31};
    uint8_t evs[6] = {0, 252, 0, 0, 252, 0};

    if (!check(
            remaster_emerald_species_info_count() == 412,
            "species catalog must expose internal IDs 0..411")
        || !check(
            remaster_emerald_move_info_count() == 355,
            "move catalog must expose IDs 0..354")
        || !check(
            remaster_emerald_item_info_count() == 377,
            "item catalog must expose IDs 0..376"))
        return 1;

    species = remaster_emerald_species_info(1); /* Bulbasaur */
    if (!check(
            species != 0
            && species->base_hp == 45
            && species->base_attack == 49
            && species->base_defense == 49
            && species->base_speed == 45
            && species->base_sp_attack == 65
            && species->base_sp_defense == 65
            && species->type1 == 12
            && species->type2 == 3
            && species->growth_rate == 3
            && species->abilities[0] == 65,
            "Bulbasaur source data mismatch"))
        return 1;

    species = remaster_emerald_species_info(303); /* Shedinja */
    if (!check(
            species != 0
            && species->base_hp == 1
            && species->abilities[0] == 25,
            "Shedinja source data mismatch"))
        return 1;

    species = remaster_emerald_species_info(277); /* Treecko */
    if (!check(
            species != 0
            && species->base_hp == 40
            && species->base_attack == 45
            && species->base_speed == 70
            && species->growth_rate == 3,
            "Treecko source data mismatch"))
        return 1;

    if (!check(
            remaster_emerald_level_from_experience(277, 135) == 5
            && remaster_emerald_level_from_experience(277, 134) == 4,
            "Medium Slow experience threshold mismatch"))
        return 1;

    if (!check(
            remaster_emerald_calculate_stats(
                277,
                50,
                3, /* Adamant */
                ivs,
                evs,
                &stats),
            "Treecko stat calculation failed"))
        return 1;

    if (!check(
            stats.hp == 115
            && stats.attack == 106
            && stats.sp_attack == 105,
            "Treecko Gen III stat/nature calculation mismatch"))
        return 1;

    memset(evs, 0, sizeof(evs));
    if (!check(
            remaster_emerald_calculate_stats(
                303,
                50,
                0,
                ivs,
                evs,
                &stats)
            && stats.hp == 1,
            "Shedinja HP special case mismatch"))
        return 1;

    if (!check(
            remaster_emerald_species_ability(1, 0) == 65
            && remaster_emerald_species_ability(303, 0) == 25,
            "species ability resolution mismatch"))
        return 1;

    move = remaster_emerald_move_info(33); /* Tackle */
    if (!check(
            move != 0
            && move->effect == 0
            && move->power == 35
            && move->type == 0
            && move->accuracy == 95
            && move->pp == 35,
            "Tackle source data mismatch"))
        return 1;

    move = remaster_emerald_move_info(85); /* Thunderbolt */
    if (!check(
            move != 0
            && move->power == 95
            && move->type == 13
            && move->accuracy == 100
            && move->pp == 15,
            "Thunderbolt source data mismatch"))
        return 1;

    item = remaster_emerald_item_info(13); /* Potion */
    if (!check(
            item != 0
            && item->price == 300
            && item->pocket == REMASTER_EMERALD_POCKET_ITEMS
            && item->hold_effect_param == 20,
            "Potion source data mismatch"))
        return 1;

    item = remaster_emerald_item_info(4); /* Poke Ball */
    if (!check(
            item != 0
            && item->price == 200
            && item->pocket == REMASTER_EMERALD_POCKET_POKE_BALLS,
            "Poke Ball source data mismatch"))
        return 1;

    learnset = remaster_emerald_species_level_up_moves(
        1, &learnset_count); /* Bulbasaur */
    if (!check(
            learnset != 0
            && learnset_count >= 4
            && learnset[2].level == 7
            && learnset[2].move_id == 73,
            "Bulbasaur level-up learnset mismatch"))
        return 1;

        evolution = remaster_emerald_species_evolutions(64); /* Kadabra */
    if (!check(
            evolution != 0
            && evolution[0].method == 4
            && evolution[0].param == 37
            && evolution[0].target_species == 65,
            "pinned Vanilla+ Kadabra evolution mismatch"))
        return 1;

    evolution = remaster_emerald_species_evolutions(328); /* Feebas */
    if (!check(
            evolution != 0
            && evolution[0].method == 15
            && evolution[0].param == 170
            && evolution[0].target_species == 329,
            "Feebas beauty evolution mismatch"))
        return 1;

    puts("R11 species/move/item/evolution catalog regression passed.");
    return 0;
}
