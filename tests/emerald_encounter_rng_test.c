#include "remaster/emerald_encounter.h"

#include <stdio.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_encounter_rng_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    static const uint16_t expected[] = {
        0x4DCB, 0xE161, 0x4340, 0xFFF1,
        0xC219, 0x69BE, 0xF670, 0x3A9D
    };
    RemasterEmeraldEncounterRng rng;
    RemasterEmeraldEncounterRuntime runtime;
    size_t i;

    remaster_emerald_encounter_rng_seed(&rng, UINT32_C(0x1234));
    if (!check(rng.state == UINT32_C(0x1234) && rng.calls == 0,
            "seed state mismatch"))
        return 1;

    for (i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        if (!check(
                remaster_emerald_encounter_random(&rng) == expected[i],
                "Emerald Random() sequence mismatch"))
            return 1;
    }

    if (!check(
            rng.state == UINT32_C(0x3A9D5ABC) && rng.calls == 8,
            "RNG final state/call count mismatch"))
        return 1;

    remaster_emerald_encounter_rng_seed(&rng, UINT32_C(0x1234));
    if (!check(
            remaster_emerald_encounter_random32(&rng)
                == UINT32_C(0xE1614DCB),
            "Random32 low/high call ordering mismatch"))
        return 1;
    if (!check(rng.calls == 2, "Random32 must consume exactly two calls"))
        return 1;

    remaster_emerald_encounter_runtime_init(
        &runtime,
        UINT32_C(0x1234));
    if (!check(
            runtime.rng.state == UINT32_C(0x1234)
            && runtime.rng.calls == 0
            && runtime.species_bag_valid == 0
            && runtime.roamer_location_valid == 0,
            "runtime initialization mismatch"))
        return 1;

    puts("R12 encounter RNG regression passed.");
    return 0;
}
