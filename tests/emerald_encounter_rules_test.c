#include "remaster/emerald_encounter.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

enum {
    VAR_REPEL_STEP_COUNT = 0x4021,
    ABILITY_STENCH = 1,
    ABILITY_SAND_VEIL = 8,
    ABILITY_ILLUMINATE = 35,
    ABILITY_ARENA_TRAP = 71,
    ABILITY_WHITE_SMOKE = 73,
    WEATHER_SANDSTORM = 8
};

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_encounter_rules_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int seed_party_mon(
    RemasterEmeraldSave *save,
    size_t slot,
    uint16_t species,
    uint8_t level,
    uint16_t hp,
    int secure_is_egg,
    int sanity_is_egg)
{
    RemasterEmeraldPartyPokemon mon;
    uint32_t iv_word;

    memset(&mon, 0, sizeof(mon));
    if (!remaster_emerald_box_pokemon_set_species(&mon.box, species))
        return 0;

    mon.box.header_flags = 0x02u;
    if (sanity_is_egg)
        mon.box.header_flags |= 0x04u;

    iv_word =
        (uint32_t)mon.box.substruct[3][4]
        | ((uint32_t)mon.box.substruct[3][5] << 8u)
        | ((uint32_t)mon.box.substruct[3][6] << 16u)
        | ((uint32_t)mon.box.substruct[3][7] << 24u);
    if (secure_is_egg)
        iv_word |= UINT32_C(1) << 30u;
    mon.box.substruct[3][4] = (uint8_t)iv_word;
    mon.box.substruct[3][5] = (uint8_t)(iv_word >> 8u);
    mon.box.substruct[3][6] = (uint8_t)(iv_word >> 16u);
    mon.box.substruct[3][7] = (uint8_t)(iv_word >> 24u);

    mon.level = level;
    mon.hp = hp;
    mon.max_hp = hp ? hp : 1u;
    return remaster_emerald_party_set(save, slot, &mon);
}

int main(void)
{
    RemasterEmeraldEncounterRateContext rate;
    RemasterEmeraldSave save;
    int wore_off = 0;
    uint16_t repel_steps = 0;

    memset(&rate, 0, sizeof(rate));

    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 320,
            "base rate x16 mismatch"))
        return 1;

    rate.biking = 1;
    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 256,
            "bike 80% modifier mismatch"))
        return 1;

    memset(&rate, 0, sizeof(rate));
    rate.flute_up = 1;
    rate.cleanse_tag = 1;
    rate.lead_ability = ABILITY_STENCH;
    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 160,
            "flute/Cleanse Tag/Stench ordering mismatch"))
        return 1;

    memset(&rate, 0, sizeof(rate));
    rate.flute_down = 1;
    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 160,
            "encounter-down flute mismatch"))
        return 1;

    memset(&rate, 0, sizeof(rate));
    rate.lead_ability = ABILITY_ILLUMINATE;
    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 640,
            "Illuminate modifier mismatch"))
        return 1;

    rate.ignore_ability = 1;
    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 320,
            "ignoreAbility mismatch"))
        return 1;

    memset(&rate, 0, sizeof(rate));
    rate.lead_ability = ABILITY_ARENA_TRAP;
    if (!check(
            remaster_emerald_encounter_modified_rate(100, &rate)
                == REMASTER_EMERALD_ENCOUNTER_MAX_RATE,
            "encounter-rate clamp mismatch"))
        return 1;

    memset(&rate, 0, sizeof(rate));
    rate.lead_ability = ABILITY_SAND_VEIL;
    rate.weather = WEATHER_SANDSTORM;
    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 160,
            "Sand Veil sandstorm modifier mismatch"))
        return 1;

    memset(&rate, 0, sizeof(rate));
    rate.lead_ability = ABILITY_WHITE_SMOKE;
    if (!check(
            remaster_emerald_encounter_modified_rate(20, &rate) == 160,
            "White Smoke modifier mismatch"))
        return 1;

    if (!check(
            remaster_emerald_encounter_area_from_behavior(0x02, 0)
                == REMASTER_EMERALD_ENCOUNTER_AREA_LAND,
            "tall grass must be land encounter behavior"))
        return 1;
    if (!check(
            remaster_emerald_encounter_area_from_behavior(0x10, 0)
                == REMASTER_EMERALD_ENCOUNTER_AREA_WATER,
            "pond water must be water encounter behavior"))
        return 1;
    if (!check(
            remaster_emerald_encounter_area_from_behavior(0x70, 1)
                == REMASTER_EMERALD_ENCOUNTER_AREA_WATER,
            "surfing bridge must use water encounter path"))
        return 1;
    if (!check(
            remaster_emerald_encounter_area_from_behavior(0x00, 0)
                == REMASTER_EMERALD_ENCOUNTER_AREA_NONE,
            "normal floor must not produce encounters"))
        return 1;

    memset(&save, 0, sizeof(save));
    if (!check(remaster_emerald_party_set_count(&save, 3),
            "party count seed failed"))
        return 1;
    if (!check(seed_party_mon(&save, 0, 1, 50, 0, 0, 0),
            "fainted lead seed failed"))
        return 1;
    if (!check(seed_party_mon(&save, 1, 1, 40, 80, 1, 0),
            "egg slot seed failed"))
        return 1;
    if (!check(seed_party_mon(&save, 2, 1, 30, 70, 0, 0),
            "eligible repel lead seed failed"))
        return 1;

    if (!check(
            remaster_emerald_var_set(
                &save,
                VAR_REPEL_STEP_COUNT,
                2),
            "repel var seed failed"))
        return 1;

    if (!check(
            !remaster_emerald_encounter_repel_allows(&save, 29)
            && remaster_emerald_encounter_repel_allows(&save, 30),
            "repel must use first living non-egg Pokémon level"))
        return 1;

    if (!check(
            remaster_emerald_encounter_update_repel(
                &save, 0, 0, 0, &wore_off)
            && !wore_off,
            "first repel decrement failed"))
        return 1;
    if (!check(
            remaster_emerald_var_get(
                &save,
                VAR_REPEL_STEP_COUNT,
                &repel_steps)
            && repel_steps == 1,
            "repel counter should be one"))
        return 1;

    if (!check(
            remaster_emerald_encounter_update_repel(
                &save, 0, 0, 0, &wore_off)
            && wore_off,
            "repel-wore-off edge failed"))
        return 1;
    if (!check(
            remaster_emerald_var_get(
                &save,
                VAR_REPEL_STEP_COUNT,
                &repel_steps)
            && repel_steps == 0,
            "repel counter should reach zero"))
        return 1;

    if (!check(
            remaster_emerald_var_set(
                &save,
                VAR_REPEL_STEP_COUNT,
                2),
            "repel reset failed"))
        return 1;
    if (!check(
            remaster_emerald_encounter_update_repel(
                &save, 0, 0, 1, &wore_off),
            "Union Room repel update call failed"))
        return 1;
    if (!check(
            remaster_emerald_var_get(
                &save,
                VAR_REPEL_STEP_COUNT,
                &repel_steps)
            && repel_steps == 2,
            "Union Room must not decrement repel"))
        return 1;

    puts("R12 encounter modifier/repel regression passed.");
    return 0;
}
