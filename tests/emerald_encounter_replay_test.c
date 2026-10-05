#include "remaster/emerald_encounter.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

enum {
    VAR_REPEL_STEP_COUNT = 0x4021,
    SPECIES_TREECKO = 277
};

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_encounter_replay_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int seed_party(RemasterEmeraldSave *save)
{
    RemasterEmeraldPartyPokemon mon;

    memset(&mon, 0, sizeof(mon));
    if (!remaster_emerald_box_pokemon_set_species(
            &mon.box,
            SPECIES_TREECKO))
        return 0;

    mon.box.header_flags = 0x02u;
    mon.level = 20;
    mon.hp = 50;
    mon.max_hp = 50;

    return remaster_emerald_party_set_count(save, 1)
        && remaster_emerald_party_set(save, 0, &mon);
}

static int same_result(
    const RemasterEmeraldEncounterResult *a,
    const RemasterEmeraldEncounterResult *b)
{
    uint8_t raw_a[REMASTER_EMERALD_BOX_POKEMON_BYTES];
    uint8_t raw_b[REMASTER_EMERALD_BOX_POKEMON_BYTES];

    if (a->occurred != b->occurred
        || a->kind != b->kind
        || a->species != b->species
        || a->level != b->level
        || a->rng_calls_after != b->rng_calls_after)
        return 0;

    if (!a->occurred)
        return 1;

    if (!remaster_emerald_box_pokemon_encode(
            &a->pokemon.box,
            raw_a,
            sizeof(raw_a))
        || !remaster_emerald_box_pokemon_encode(
            &b->pokemon.box,
            raw_b,
            sizeof(raw_b)))
        return 0;

    return memcmp(raw_a, raw_b, sizeof(raw_a)) == 0;
}

int main(void)
{
    RemasterEmeraldSave save_a;
    RemasterEmeraldSave save_b;
    RemasterEmeraldEncounterRuntime runtime_a;
    RemasterEmeraldEncounterRuntime runtime_b;
    RemasterEmeraldEncounterStepContext step;
    RemasterEmeraldEncounterResult a;
    RemasterEmeraldEncounterResult b;
    uint64_t calls_after_encounter;
    uint16_t repel = 0;
    int i;

    memset(&save_a, 0, sizeof(save_a));
    memset(&save_b, 0, sizeof(save_b));
    if (!check(seed_party(&save_a) && seed_party(&save_b),
            "party seed failed"))
        return 1;

    remaster_emerald_encounter_runtime_init(&runtime_a, 0);
    remaster_emerald_encounter_runtime_init(&runtime_b, 0);

    memset(&step, 0, sizeof(step));
    step.map_group = 0;
    step.map_num = 16;
    step.current_behavior = 0x02;
    step.previous_behavior = 0x02;

    for (i = 0; i < 4; ++i) {
        memset(&a, 0, sizeof(a));
        memset(&b, 0, sizeof(b));

        if (!check(
                !remaster_emerald_encounter_step(
                    &runtime_a, &save_a, &step, &a)
                && !remaster_emerald_encounter_step(
                    &runtime_b, &save_b, &step, &b),
                "immunity step unexpectedly produced encounter"))
            return 1;

        if (!check(
                runtime_a.rng.calls == 0
                && runtime_b.rng.calls == 0
                && runtime_a.wild_immunity_steps == (uint8_t)(i + 1),
                "immunity step consumed RNG or counted incorrectly"))
            return 1;
    }

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (!check(
            remaster_emerald_encounter_step(
                &runtime_a, &save_a, &step, &a)
            && remaster_emerald_encounter_step(
                &runtime_b, &save_b, &step, &b),
            "seed-zero fifth Route101 step should encounter"))
        return 1;

    if (!check(
            a.kind == REMASTER_EMERALD_ENCOUNTER_REGULAR
            && a.species != 0
            && a.level >= 5
            && a.level <= 35
            && same_result(&a, &b),
            "fixed-seed fifth-step encounter mismatch"))
        return 1;

    if (!check(
            runtime_a.wild_immunity_steps == 0,
            "successful encounter must restart immunity counter"))
        return 1;

    calls_after_encounter = runtime_a.rng.calls;

    for (i = 0; i < 4; ++i) {
        memset(&a, 0, sizeof(a));
        if (!check(
                !remaster_emerald_encounter_step(
                    &runtime_a, &save_a, &step, &a),
                "post-encounter immunity failed"))
            return 1;
    }

    if (!check(
            runtime_a.rng.calls == calls_after_encounter,
            "post-encounter immunity must consume no encounter RNG"))
        return 1;

    /*
     * Repel expiry is a step script before CheckStandardWildEncounter. Even
     * with immunity already exhausted and seed-zero favorable odds, the expiry
     * step must stop before the rate roll.
     */
    runtime_a.wild_immunity_steps = 4;
    runtime_a.previous_behavior = 0x02;
    runtime_a.previous_behavior_valid = 1;
    remaster_emerald_encounter_rng_seed(&runtime_a.rng, 0);

    if (!check(
            remaster_emerald_var_set(
                &save_a,
                VAR_REPEL_STEP_COUNT,
                1),
            "repel seed failed"))
        return 1;

    memset(&a, 0, sizeof(a));
    if (!check(
            !remaster_emerald_encounter_step(
                &runtime_a,
                &save_a,
                &step,
                &a),
            "repel-expiry step must not encounter"))
        return 1;

    if (!check(
            runtime_a.rng.calls == 0,
            "repel-expiry step must occur before encounter RNG"))
        return 1;

    if (!check(
            remaster_emerald_var_get(
                &save_a,
                VAR_REPEL_STEP_COUNT,
                &repel)
            && repel == 0,
            "repel expiry did not persist zero"))
        return 1;

    puts("R12 fixed-seed encounter replay regression passed.");
    return 0;
}
