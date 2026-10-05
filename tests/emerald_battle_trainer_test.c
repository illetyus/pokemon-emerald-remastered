#include "remaster/emerald_battle.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldTrainer trainer;
    const RemasterEmeraldTrainer *catalog_trainer;
    RemasterEmeraldBattleRng rng_a;
    RemasterEmeraldBattleRng rng_b;
    RemasterEmeraldPartyPokemon party_a[REMASTER_EMERALD_BATTLE_PARTY_SIZE];
    RemasterEmeraldPartyPokemon party_b[REMASTER_EMERALD_BATTLE_PARTY_SIZE];
    uint16_t moves[REMASTER_EMERALD_MAX_MOVES];
    uint8_t pp[REMASTER_EMERALD_MAX_MOVES];
    uint8_t ivs[REMASTER_EMERALD_STAT_COUNT];
    uint8_t count_a;
    uint8_t count_b;
    size_t i;

    if (!check(
            remaster_emerald_battle_trainer_count() == 857u,
            "generated trainer catalog must cover IDs 0 through 856"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(1);
    if (!check(
            catalog_trainer != 0
                && catalog_trainer->trainer_id == 1
                && catalog_trainer->trainer_class == 2
                && strcmp(catalog_trainer->trainer_name, "SAWYER") == 0
                && catalog_trainer->ai_flags
                    == (REMASTER_EMERALD_AI_CHECK_BAD_MOVE
                        | REMASTER_EMERALD_AI_TRY_TO_FAINT
                        | REMASTER_EMERALD_AI_CHECK_VIABILITY)
                && catalog_trainer->party_size == 1
                && catalog_trainer->party[0].species == 74
                && catalog_trainer->party[0].level == 21,
            "Sawyer must round-trip from pinned Vanilla+ trainer data"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(11);
    if (!check(
            catalog_trainer != 0
                && strcmp(catalog_trainer->trainer_name, "MARCEL") == 0
                && catalog_trainer->items[0] == 21
                && catalog_trainer->party_size == 2,
            "trainer items and party size must survive catalog generation"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(38);
    if (!check(
            catalog_trainer != 0
                && strcmp(catalog_trainer->trainer_name, "FELIX") == 0
                && catalog_trainer->party_flags
                    == REMASTER_EMERALD_TRAINER_PARTY_CUSTOM_MOVESET
                && catalog_trainer->party[0].moves[0] == 94,
            "custom trainer moves must survive catalog generation"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(51);
    if (!check(
            catalog_trainer != 0
                && strcmp(catalog_trainer->trainer_name, "GABBY & TY") == 0
                && catalog_trainer->double_battle
                && catalog_trainer->party_size == 2,
            "double-battle trainer metadata must survive catalog generation"))
        return 1;

    if (!check(
            remaster_emerald_battle_trainer_find(857) == 0,
            "out-of-range trainer IDs must not resolve"))
        return 1;

    if (!check(
            remaster_emerald_battle_trainer_reward(1, 1) == 840u,
            "Sawyer reward must match Hiker class x level formula"))
        return 1;
    if (!check(
            remaster_emerald_battle_trainer_reward(1, 2) == 1680u,
            "money multiplier must scale trainer reward"))
        return 1;
    if (!check(
            remaster_emerald_battle_trainer_reward(51, 1) == 1632u,
            "double trainer reward must apply the Vanilla x2 factor"))
        return 1;

    remaster_emerald_battle_rng_seed(&rng_a, 0x12345678u);
    remaster_emerald_battle_rng_seed(&rng_b, 0x12345678u);
    if (!check(
            remaster_emerald_battle_build_trainer_party(
                &rng_a, 1, party_a, &count_a)
                && remaster_emerald_battle_build_trainer_party(
                    &rng_b, 1, party_b, &count_b),
            "Sawyer trainer party should build"))
        return 1;
    if (!check(
            count_a == 1
                && count_b == 1
                && memcmp(party_a, party_b, sizeof(party_a)) == 0
                && rng_a.state == rng_b.state
                && rng_a.calls == rng_b.calls,
            "fixed-seed trainer party construction must be deterministic"))
        return 1;
    if (!check(
            remaster_emerald_box_pokemon_species(&party_a[0].box) == 74
                && party_a[0].level == 21,
            "Sawyer party must construct the pinned Geodude"))
        return 1;
    remaster_emerald_box_pokemon_moves(&party_a[0].box, moves, pp);
    if (!check(
            moves[0] != 0,
            "default-move trainer parties must receive level-up moves"))
        return 1;
    remaster_emerald_box_pokemon_ivs(&party_a[0].box, ivs);
    for (i = 0; i < REMASTER_EMERALD_STAT_COUNT; ++i) {
        if (!check(ivs[i] == 0, "trainer IV 0 must scale to stat IV 0"))
            return 1;
    }

    remaster_emerald_battle_rng_seed(&rng_a, 0x2468ACE0u);
    if (!check(
            remaster_emerald_battle_build_trainer_party(
                &rng_a, 38, party_a, &count_a),
            "Felix custom-move party should build"))
        return 1;
    remaster_emerald_box_pokemon_moves(&party_a[0].box, moves, pp);
    if (!check(
            count_a == 2 && moves[0] == 94,
            "custom trainer moves must replace default level-up moves"))
        return 1;

    remaster_emerald_battle_rng_seed(&rng_a, 0xCAFEBABEu);
    if (!check(
            remaster_emerald_battle_build_trainer_party(
                &rng_a, 114, party_a, &count_a),
            "held-item trainer party should build"))
        return 1;
    if (!check(
            remaster_emerald_box_pokemon_held_item(&party_a[0].box) == 110,
            "Cindy's Zigzagoon must carry the pinned Nugget"))
        return 1;

    remaster_emerald_battle_rng_seed(&rng_a, 0x0BADF00Du);
    if (!check(
            remaster_emerald_battle_build_trainer_party(
                &rng_a, 71, party_a, &count_a),
            "max-IV trainer party should build"))
        return 1;
    remaster_emerald_box_pokemon_ivs(&party_a[0].box, ivs);
    for (i = 0; i < REMASTER_EMERALD_STAT_COUNT; ++i) {
        if (!check(ivs[i] == 31, "trainer IV 255 must scale to stat IV 31"))
            return 1;
    }

    memset(&trainer, 0, sizeof(trainer));
    trainer.trainer_id = 1;
    trainer.trainer_class = 1;
    trainer.ai_flags =
        REMASTER_EMERALD_AI_CHECK_BAD_MOVE
        | REMASTER_EMERALD_AI_TRY_TO_FAINT
        | REMASTER_EMERALD_AI_CHECK_VIABILITY;
    trainer.party_size = 1;
    trainer.party[0].species = 1;
    trainer.party[0].level = 21;

    if (!check(
            remaster_emerald_battle_trainer_validate(&trainer),
            "default-moves trainer contract should validate"))
        return 1;

    trainer.party[0].moves[0] = 1;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "custom move data requires the custom-moves flag"))
        return 1;

    trainer.party_flags = REMASTER_EMERALD_TRAINER_PARTY_CUSTOM_MOVESET;
    if (!check(
            remaster_emerald_battle_trainer_validate(&trainer),
            "custom move trainer contract should validate with flag"))
        return 1;

    trainer.party[0].held_item = 1;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "held item data requires the held-item flag"))
        return 1;

    trainer.party_flags |= REMASTER_EMERALD_TRAINER_PARTY_HELD_ITEM;
    if (!check(
            remaster_emerald_battle_trainer_validate(&trainer),
            "held-item custom-move trainer contract should validate"))
        return 1;

    trainer.party[0].level = 101;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "trainer level must remain within Gen III bounds"))
        return 1;

    trainer.party[0].level = 21;
    trainer.party_size = 0;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "empty trainer parties must be rejected"))
        return 1;

    puts("r13 trainer contract test passed");
    return 0;
}
