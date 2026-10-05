#include "remaster/emerald_encounter.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_encounter_ecosystem_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int contains(
    const uint16_t *values,
    size_t count,
    uint16_t target)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        if (values[i] == target)
            return 1;
    }
    return 0;
}

static int seed_party_mon(
    RemasterEmeraldSave *save,
    size_t slot,
    uint16_t species,
    uint8_t level,
    uint16_t hp,
    int is_egg)
{
    RemasterEmeraldPartyPokemon mon;
    uint32_t word;

    memset(&mon, 0, sizeof(mon));
    if (!remaster_emerald_box_pokemon_set_species(&mon.box, species))
        return 0;

    mon.box.header_flags = 0x02u;
    word = (uint32_t)mon.box.substruct[3][4]
        | ((uint32_t)mon.box.substruct[3][5] << 8u)
        | ((uint32_t)mon.box.substruct[3][6] << 16u)
        | ((uint32_t)mon.box.substruct[3][7] << 24u);
    if (is_egg)
        word |= UINT32_C(1) << 30u;
    mon.box.substruct[3][4] = (uint8_t)word;
    mon.box.substruct[3][5] = (uint8_t)(word >> 8u);
    mon.box.substruct[3][6] = (uint8_t)(word >> 16u);
    mon.box.substruct[3][7] = (uint8_t)(word >> 24u);

    mon.level = level;
    mon.hp = hp;
    mon.max_hp = hp ? hp : 1;
    return remaster_emerald_party_set(save, slot, &mon);
}

int main(void)
{
    RemasterEmeraldEncounterRuntime runtime;
    const RemasterEmeraldEncounterMapInfo *route101;
    const RemasterEmeraldEncounterMapInfo *new_mauville;
    RemasterEmeraldSave save;
    uint16_t first_cycle[REMASTER_EMERALD_ENCOUNTER_MAX_LOCAL_POOL];
    uint16_t next_species;
    uint16_t bag_count;
    uint64_t calls_before;
    size_t i;
    size_t j;

    if (!check(
            remaster_emerald_encounter_map_count() == 124,
            "encounter catalog must contain 124 maps"))
        return 1;

    route101 = remaster_emerald_encounter_map_find(0, 16);
    if (!check(route101 != 0, "Route101 encounter map missing"))
        return 1;
    if (!check(
            route101->land.encounter_rate == 20
            && route101->land.anchor_count == 12
            && route101->land.anchors[0] == 290
            && route101->land.anchors[1] == 286,
            "Route101 source anchor data mismatch"))
        return 1;

    if (!check(
            remaster_emerald_encounter_species_habitat(1)
                == REMASTER_EMERALD_HABITAT_GRASSLAND,
            "Bulbasaur habitat mismatch"))
        return 1;
    if (!check(
            remaster_emerald_encounter_species_habitat(4)
                == REMASTER_EMERALD_HABITAT_MOUNTAIN,
            "Charmander habitat mismatch"))
        return 1;
    if (!check(
            remaster_emerald_encounter_species_habitat(122)
                == REMASTER_EMERALD_HABITAT_URBAN,
            "Mr. Mime habitat mismatch"))
        return 1;

    remaster_emerald_encounter_runtime_init(
        &runtime,
        UINT32_C(0xC0FFEE));

    if (!check(
            remaster_emerald_encounter_build_local_pool(
                &runtime,
                route101,
                REMASTER_EMERALD_ENCOUNTER_AREA_LAND,
                REMASTER_EMERALD_ROD_NONE),
            "Route101 local pool build failed"))
        return 1;

    bag_count = runtime.species_bag_count;
    if (!check(
            bag_count >= 3
            && bag_count <= REMASTER_EMERALD_ENCOUNTER_MAX_LOCAL_POOL,
            "Route101 bag size outside accepted range"))
        return 1;

    if (!check(
            contains(runtime.species_bag, bag_count, 286)
            && contains(runtime.species_bag, bag_count, 288)
            && contains(runtime.species_bag, bag_count, 290),
            "Route101 must preserve Vanilla anchor species"))
        return 1;

    for (i = 0; i < bag_count; ++i) {
        first_cycle[i] = remaster_emerald_encounter_choose_species(
            &runtime,
            route101,
            REMASTER_EMERALD_ENCOUNTER_AREA_LAND,
            REMASTER_EMERALD_ROD_NONE,
            1);
        if (!check(first_cycle[i] != 0, "bag emitted SPECIES_NONE"))
            return 1;

        for (j = 0; j < i; ++j) {
            if (!check(
                    first_cycle[j] != first_cycle[i],
                    "species repeated inside one bag cycle"))
                return 1;
        }
    }

    next_species = remaster_emerald_encounter_choose_species(
        &runtime,
        route101,
        REMASTER_EMERALD_ENCOUNTER_AREA_LAND,
        REMASTER_EMERALD_ROD_NONE,
        1);
    if (!check(
            next_species != 0
            && next_species != first_cycle[bag_count - 1],
            "bag boundary repeated previous last species"))
        return 1;

    new_mauville = remaster_emerald_encounter_map_find(24, 53);
    if (!check(new_mauville != 0, "New Mauville encounter map missing"))
        return 1;

    remaster_emerald_encounter_runtime_init(
        &runtime,
        UINT32_C(0xBEEF));
    if (!check(
            remaster_emerald_encounter_build_local_pool(
                &runtime,
                new_mauville,
                REMASTER_EMERALD_ENCOUNTER_AREA_LAND,
                REMASTER_EMERALD_ROD_NONE),
            "New Mauville local pool build failed"))
        return 1;
    if (!check(
            contains(
                runtime.species_bag,
                runtime.species_bag_count,
                122),
            "Mr. Mime New Mauville guarantee missing"))
        return 1;

    memset(&save, 0, sizeof(save));
    if (!check(remaster_emerald_party_set_count(&save, 4),
            "party count seed failed"))
        return 1;
    if (!check(seed_party_mon(&save, 0, 1, 40, 50, 0),
            "level 40 party seed failed"))
        return 1;
    if (!check(seed_party_mon(&save, 1, 1, 50, 50, 0),
            "level 50 party seed failed"))
        return 1;
    if (!check(seed_party_mon(&save, 2, 1, 30, 50, 0),
            "level 30 party seed failed"))
        return 1;
    if (!check(seed_party_mon(&save, 3, 1, 100, 50, 1),
            "egg exclusion seed failed"))
        return 1;

    remaster_emerald_encounter_runtime_init(
        &runtime,
        UINT32_C(0x1234));

    for (i = 0; i < 256; ++i) {
        const uint8_t level =
            remaster_emerald_encounter_choose_level(
                &runtime,
                &save);
        if (!check(
                level >= 25 && level <= 55,
                "Phase9 level escaped average +/-15 range"))
            return 1;
    }
    if (!check(
            runtime.rng.calls == 256,
            "each Phase9 level pick must consume one RNG call"))
        return 1;

    memset(&save, 0, sizeof(save));
    remaster_emerald_encounter_runtime_init(
        &runtime,
        UINT32_C(0x1234));
    calls_before = runtime.rng.calls;
    if (!check(
            remaster_emerald_encounter_choose_level(
                &runtime,
                &save) == 2,
            "empty eligible party must return level 2"))
        return 1;
    if (!check(
            runtime.rng.calls == calls_before,
            "empty eligible party must not consume RNG"))
        return 1;

    puts("R12 Phase9 ecosystem regression passed.");
    return 0;
}
