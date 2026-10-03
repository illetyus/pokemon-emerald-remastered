#include "remaster/core.h"
#include "remaster/mechanics.h"
#include "remaster/platform.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct Probe {
    uint32_t event_ids[16];
    int64_t arg0[16];
    int64_t arg1[16];
    size_t count;
} Probe;

static void on_event(
    void *userdata,
    uint32_t event_id,
    int64_t arg0,
    int64_t arg1)
{
    Probe *probe = (Probe *)userdata;

    if (probe == 0 || probe->count >= 16)
        return;

    probe->event_ids[probe->count] = event_id;
    probe->arg0[probe->count] = arg0;
    probe->arg1[probe->count] = arg1;
    probe->count++;
}

static int fail(const char *message)
{
    fprintf(stderr, "platform/mechanics contract failed: %s\n", message);
    return 1;
}

int main(void)
{
    Probe probe;
    RemasterPlatformVTable platform;
    RemasterState state;
    RemasterMechanicsConfig config;
    RemasterMechanicsConfig observed;
    uint32_t events;

    memset(&probe, 0, sizeof(probe));
    memset(&platform, 0, sizeof(platform));

    platform.userdata = &probe;
    platform.emit_presentation_event = on_event;
    remaster_platform_install(&platform);

    remaster_core_init(&state);

    events = remaster_core_step(&state, REMASTER_INPUT_MOVE_UP);
    if (events != REMASTER_EVENT_BLOCKED)
        return fail("blocked event return mismatch");
    if (probe.count != 1 || probe.event_ids[0] != REMASTER_EVENT_BLOCKED)
        return fail("blocked event callback mismatch");
    if (probe.arg0[0] != 1 || probe.arg1[0] != 1)
        return fail("blocked event coordinates mismatch");

    events = remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    if (events != REMASTER_EVENT_MOVED)
        return fail("move return mismatch");
    if (probe.count != 2 || probe.event_ids[1] != REMASTER_EVENT_MOVED)
        return fail("move callback mismatch");
    if (probe.arg0[1] != 2 || probe.arg1[1] != 1)
        return fail("move callback coordinates mismatch");

    remaster_platform_install(0);

    remaster_mechanics_reset();
    observed = remaster_mechanics_get();
    if (observed.generation != REMASTER_MECHANICS_EMERALD_AUTHENTIC)
        return fail("default mechanics generation mismatch");

    memset(&config, 0, sizeof(config));
    config.generation = REMASTER_MECHANICS_SELECTIVE_MODERN;
    config.physical_special_split = 1;
    config.alternative_trade_evolutions = 1;
    config.show_move_effectiveness = 1;
    config.fast_battle_animations_option = 1;

    remaster_mechanics_set(&config);
    observed = remaster_mechanics_get();

    if (observed.generation != REMASTER_MECHANICS_SELECTIVE_MODERN ||
        observed.physical_special_split != 1 ||
        observed.alternative_trade_evolutions != 1 ||
        observed.show_move_effectiveness != 1 ||
        observed.fast_battle_animations_option != 1)
        return fail("mechanics profile round-trip mismatch");

    puts("Platform event and mechanics contracts passed.");
    return 0;
}
