#include "remaster/emerald_encounter.h"

#include <stdio.h>
#include <string.h>

enum {
    SB1_OUTBREAK_SPECIES = 0x2B90,
    SB1_OUTBREAK_MAP_NUM = 0x2B92,
    SB1_OUTBREAK_MAP_GROUP = 0x2B93,
    SB1_OUTBREAK_LEVEL = 0x2B94,
    SB1_OUTBREAK_MOVES = 0x2B98,
    SB1_OUTBREAK_PROBABILITY = 0x2BA1,

    SB1_ROAMER = 0x31DC,

    SPECIES_TREECKO = 277,
    SPECIES_LATIAS = 407,
    MOVE_POUND = 1,
    MOVE_LEER = 43
};

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_encounter_special_test: %s\n", message);
        return 0;
    }
    return 1;
}

static void put16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static void put32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

static int seed_simple_party(
    RemasterEmeraldSave *save,
    uint8_t level)
{
    RemasterEmeraldPartyPokemon mon;

    memset(&mon, 0, sizeof(mon));
    if (!remaster_emerald_box_pokemon_set_species(
            &mon.box,
            SPECIES_TREECKO))
        return 0;

    mon.box.header_flags = 0x02u;
    mon.level = level;
    mon.hp = 30;
    mon.max_hp = 30;

    return remaster_emerald_party_set_count(save, 1)
        && remaster_emerald_party_set(save, 0, &mon);
}

static int same_generated_mon(
    const RemasterEmeraldPartyPokemon *a,
    const RemasterEmeraldPartyPokemon *b)
{
    uint8_t raw_a[REMASTER_EMERALD_BOX_POKEMON_BYTES];
    uint8_t raw_b[REMASTER_EMERALD_BOX_POKEMON_BYTES];

    if (!remaster_emerald_box_pokemon_encode(
            &a->box, raw_a, sizeof(raw_a))
        || !remaster_emerald_box_pokemon_encode(
            &b->box, raw_b, sizeof(raw_b)))
        return 0;

    return memcmp(raw_a, raw_b, sizeof(raw_a)) == 0
        && a->status == b->status
        && a->level == b->level
        && a->mail == b->mail
        && a->hp == b->hp
        && a->max_hp == b->max_hp
        && a->attack == b->attack
        && a->defense == b->defense
        && a->speed == b->speed
        && a->sp_attack == b->sp_attack
        && a->sp_defense == b->sp_defense;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldEncounterRuntime runtime_a;
    RemasterEmeraldEncounterRuntime runtime_b;
    RemasterEmeraldPartyPokemon wild_a;
    RemasterEmeraldPartyPokemon wild_b;
    RemasterEmeraldEncounterResult result;
    uint16_t moves[4] = {0};
    uint8_t pp[4] = {0};
    uint8_t ivs[6] = {0};
    size_t i;

    memset(&save, 0, sizeof(save));
    save.save_block2[0x08] = 1;
    save.save_block2[0x0A] = 0x12;
    save.save_block2[0x0B] = 0x34;
    save.save_block2[0x0C] = 0x56;
    save.save_block2[0x0D] = 0x78;

    remaster_emerald_encounter_runtime_init(
        &runtime_a,
        UINT32_C(0x1234));
    remaster_emerald_encounter_runtime_init(
        &runtime_b,
        UINT32_C(0x1234));

    memset(&wild_a, 0, sizeof(wild_a));
    memset(&wild_b, 0, sizeof(wild_b));

    if (!check(
            remaster_emerald_encounter_create_wild(
                &runtime_a,
                &save,
                SPECIES_TREECKO,
                5,
                0,
                &wild_a),
            "Treecko wild creation failed"))
        return 1;
    if (!check(
            remaster_emerald_encounter_create_wild(
                &runtime_b,
                &save,
                SPECIES_TREECKO,
                5,
                0,
                &wild_b),
            "repeat Treecko wild creation failed"))
        return 1;

    if (!check(
            same_generated_mon(&wild_a, &wild_b)
            && runtime_a.rng.state == runtime_b.rng.state
            && runtime_a.rng.calls == runtime_b.rng.calls,
            "fixed-seed wild creation is not deterministic"))
        return 1;

    if (!check(
            remaster_emerald_box_pokemon_species(&wild_a.box)
                == SPECIES_TREECKO
            && wild_a.level == 5
            && remaster_emerald_box_pokemon_experience(&wild_a.box)
                == 135,
            "wild identity/level/experience mismatch"))
        return 1;

    if (!check(
            remaster_emerald_box_pokemon_nature(&wild_a.box)
                == (uint8_t)(wild_a.box.personality % 25u),
            "wild nature/personality mismatch"))
        return 1;

    remaster_emerald_box_pokemon_ivs(&wild_a.box, ivs);
    for (i = 0; i < 6; ++i) {
        if (!check(ivs[i] <= 31, "wild IV outside Gen III range"))
            return 1;
    }

    remaster_emerald_box_pokemon_moves(&wild_a.box, moves, pp);
    if (!check(
            moves[0] == MOVE_POUND
            && moves[1] == MOVE_LEER
            && moves[2] == 0
            && moves[3] == 0,
            "level-5 Treecko initial moveset mismatch"))
        return 1;

    /*
     * Outbreak has priority over the regular Phase9 land pool when no roamer
     * starts. Its saved four moves override the ordinary initial learnset.
     */
    memset(&save, 0, sizeof(save));
    if (!check(seed_simple_party(&save, 20), "party seed failed"))
        return 1;
    put16(
        save.save_block1 + SB1_OUTBREAK_SPECIES,
        SPECIES_TREECKO);
    save.save_block1[SB1_OUTBREAK_MAP_NUM] = 16;
    save.save_block1[SB1_OUTBREAK_MAP_GROUP] = 0;
    save.save_block1[SB1_OUTBREAK_LEVEL] = 12;
    put16(save.save_block1 + SB1_OUTBREAK_MOVES + 0, MOVE_POUND);
    put16(save.save_block1 + SB1_OUTBREAK_MOVES + 2, MOVE_LEER);
    put16(save.save_block1 + SB1_OUTBREAK_MOVES + 4, 0);
    put16(save.save_block1 + SB1_OUTBREAK_MOVES + 6, 0);
    save.save_block1[SB1_OUTBREAK_PROBABILITY] = 100;

    remaster_emerald_encounter_runtime_init(&runtime_a, 7);
    memset(&result, 0, sizeof(result));
    if (!check(
            remaster_emerald_encounter_generate_after_rate(
                &runtime_a,
                &save,
                0,
                16,
                REMASTER_EMERALD_ENCOUNTER_AREA_LAND,
                REMASTER_EMERALD_ROD_NONE,
                &result)
            && result.occurred
            && result.kind == REMASTER_EMERALD_ENCOUNTER_OUTBREAK
            && result.species == SPECIES_TREECKO
            && result.level == 12,
            "mass outbreak priority/result mismatch"))
        return 1;

    memset(moves, 0, sizeof(moves));
    memset(pp, 0, sizeof(pp));
    remaster_emerald_box_pokemon_moves(
        &result.pokemon.box,
        moves,
        pp);
    if (!check(
            moves[0] == MOVE_POUND
            && moves[1] == MOVE_LEER,
            "outbreak saved move override mismatch"))
        return 1;

    /*
     * Roamer check is before outbreak/regular encounters. Seed zero makes the
     * first Emerald Random() value zero, satisfying Random()%4 == 0.
     */
    memset(&save, 0, sizeof(save));
    if (!check(seed_simple_party(&save, 20), "roamer party seed failed"))
        return 1;

    put32(save.save_block1 + SB1_ROAMER + 0x00, UINT32_C(0x12345678));
    put32(save.save_block1 + SB1_ROAMER + 0x04, UINT32_C(0x89ABCDEF));
    put16(save.save_block1 + SB1_ROAMER + 0x08, SPECIES_LATIAS);
    put16(save.save_block1 + SB1_ROAMER + 0x0A, 100);
    save.save_block1[SB1_ROAMER + 0x0C] = 40;
    save.save_block1[SB1_ROAMER + 0x0D] = 0;
    save.save_block1[SB1_ROAMER + 0x13] = 1;

    remaster_emerald_encounter_runtime_init(&runtime_a, 0);
    remaster_emerald_encounter_roamer_set_location(
        &runtime_a,
        0,
        16);
    memset(&result, 0, sizeof(result));
    if (!check(
            remaster_emerald_encounter_generate_after_rate(
                &runtime_a,
                &save,
                0,
                16,
                REMASTER_EMERALD_ENCOUNTER_AREA_LAND,
                REMASTER_EMERALD_ROD_NONE,
                &result)
            && result.kind == REMASTER_EMERALD_ENCOUNTER_ROAMER
            && result.species == SPECIES_LATIAS
            && result.level == 40
            && result.pokemon.box.personality
                == UINT32_C(0x89ABCDEF),
            "roamer priority/state restoration mismatch"))
        return 1;

    /*
     * Fishing bypasses step encounter-rate/repel checks after the rod minigame
     * and remains deterministic for fixed state + seed.
     */
    memset(&save, 0, sizeof(save));
    if (!check(seed_simple_party(&save, 35), "fishing party seed failed"))
        return 1;
    remaster_emerald_encounter_runtime_init(
        &runtime_a,
        UINT32_C(0xCAFE));
    remaster_emerald_encounter_runtime_init(
        &runtime_b,
        UINT32_C(0xCAFE));

    {
        RemasterEmeraldEncounterResult a;
        RemasterEmeraldEncounterResult b;
        memset(&a, 0, sizeof(a));
        memset(&b, 0, sizeof(b));

        if (!check(
                remaster_emerald_encounter_fishing(
                    &runtime_a,
                    &save,
                    0,
                    17,
                    REMASTER_EMERALD_ROD_OLD,
                    &a)
                && remaster_emerald_encounter_fishing(
                    &runtime_b,
                    &save,
                    0,
                    17,
                    REMASTER_EMERALD_ROD_OLD,
                    &b),
                "Route102 Old Rod generation failed"))
            return 1;

        if (!check(
                a.occurred
                && a.kind == REMASTER_EMERALD_ENCOUNTER_FISHING
                && a.species == b.species
                && a.level == b.level
                && same_generated_mon(&a.pokemon, &b.pokemon)
                && runtime_a.rng.state == runtime_b.rng.state,
                "fixed-seed fishing result mismatch"))
            return 1;
    }

    puts("R12 wild/special encounter regression passed.");
    return 0;
}
