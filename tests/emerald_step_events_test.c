#include "remaster/emerald_overworld.h"

#include <stdio.h>
#include <string.h>

typedef struct ImmediateContext {
    RemasterEmeraldSave *save;
    int calls;
} ImmediateContext;

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_step_events_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int run_immediate(void *userdata, const char *script_id)
{
    ImmediateContext *context = (ImmediateContext *)userdata;

    if (context == 0 || context->save == 0 || script_id == 0)
        return 0;

    if (strcmp(script_id, "ImmediateScript") != 0)
        return 0;

    ++context->calls;
    return remaster_emerald_var_set(context->save, 0x4002, 1);
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldOverworldState state;
    RemasterEmeraldCoordEventDef coords[3];
    RemasterEmeraldWarpEventDef warps[1];
    RemasterEmeraldStepEventResult result;
    ImmediateContext immediate;

    memset(&save, 0, sizeof(save));
    memset(&state, 0, sizeof(state));
    memset(coords, 0, sizeof(coords));
    memset(warps, 0, sizeof(warps));
    memset(&result, 0, sizeof(result));
    memset(&immediate, 0, sizeof(immediate));

    state.player_x = 1;
    state.player_y = 1;
    state.weather = REMASTER_EMERALD_WEATHER_SUNNY;
    if (!check(
            remaster_emerald_overworld_set(&save, &state),
            "failed to seed overworld state"))
        return 1;

    coords[0].kind = REMASTER_EMERALD_COORD_WEATHER;
    coords[0].x = 1;
    coords[0].y = 1;
    coords[0].elevation = 0;
    coords[0].weather = 7; /* coord diagonal fog -> engine weather 9 */

    coords[1].kind = REMASTER_EMERALD_COORD_TRIGGER;
    coords[1].x = 1;
    coords[1].y = 1;
    coords[1].elevation = 0;
    coords[1].trigger = 0x4001;
    coords[1].index = 7;
    coords[1].script_id = "BlockingCoordScript";

    warps[0].x = 1;
    warps[0].y = 1;
    warps[0].elevation = 0;
    warps[0].dest_map_group = 1;
    warps[0].dest_map_num = 2;
    warps[0].dest_warp_id = 0;

    if (!check(
            remaster_emerald_var_set(&save, 0x4001, 7),
            "failed to seed coord trigger var"))
        return 1;

    if (!check(
            remaster_emerald_process_step_events(
                &save,
                coords,
                2,
                0,
                warps,
                1,
                1,
                1,
                0,
                0x69,
                0,
                0,
                &result),
            "coord-before-warp processing failed"))
        return 1;

    if (!check(
            result.kind == REMASTER_EMERALD_STEP_EVENT_COORD_SCRIPT
            && result.coord_event_index == 1
            && result.script_id != 0
            && strcmp(result.script_id, "BlockingCoordScript") == 0
            && result.warp_index == SIZE_MAX,
            "matching coord script must win before warp"))
        return 1;

    if (!check(
            remaster_emerald_overworld_get(&save, &state)
            && state.weather == REMASTER_EMERALD_WEATHER_FOG_DIAGONAL,
            "coord weather did not translate/apply before script"))
        return 1;

    if (!check(
            remaster_emerald_var_set(&save, 0x4001, 6),
            "failed to change coord trigger var"))
        return 1;

    if (!check(
            remaster_emerald_process_step_events(
                &save,
                coords,
                2,
                0,
                warps,
                1,
                1,
                1,
                0,
                0x69,
                0,
                0,
                &result)
            && result.kind == REMASTER_EMERALD_STEP_EVENT_WARP
            && result.warp_index == 0,
            "warp should run after non-blocking coord scan"))
        return 1;

    if (!check(
            remaster_emerald_process_step_events(
                &save,
                coords,
                2,
                0,
                warps,
                1,
                1,
                1,
                0,
                0x00,
                0,
                0,
                &result)
            && result.kind == REMASTER_EMERALD_STEP_EVENT_NONE,
            "warp event on non-warp metatile must not trigger"))
        return 1;

    memset(coords, 0, sizeof(coords));
    coords[0].kind = REMASTER_EMERALD_COORD_TRIGGER;
    coords[0].x = 1;
    coords[0].y = 1;
    coords[0].elevation = 0;
    coords[0].trigger = 0;
    coords[0].index = 0;
    coords[0].script_id = "ImmediateScript";

    coords[1].kind = REMASTER_EMERALD_COORD_TRIGGER;
    coords[1].x = 1;
    coords[1].y = 1;
    coords[1].elevation = 0;
    coords[1].trigger = 0x4002;
    coords[1].index = 1;
    coords[1].script_id = "AfterImmediate";

    if (!check(
            remaster_emerald_var_set(&save, 0x4002, 0),
            "failed to reset immediate-test var"))
        return 1;

    immediate.save = &save;
    immediate.calls = 0;

    if (!check(
            remaster_emerald_process_step_events(
                &save,
                coords,
                2,
                0,
                warps,
                1,
                1,
                1,
                0,
                0x69,
                &immediate,
                run_immediate,
                &result)
            && immediate.calls == 1
            && result.kind == REMASTER_EMERALD_STEP_EVENT_COORD_SCRIPT
            && result.coord_event_index == 1
            && strcmp(result.script_id, "AfterImmediate") == 0,
            "immediate script did not continue coord scan with mutated state"))
        return 1;

    if (!check(
            remaster_emerald_var_set(&save, 0x4002, 0),
            "failed to reset handoff var"))
        return 1;

    if (!check(
            remaster_emerald_process_step_events(
                &save,
                coords,
                2,
                0,
                warps,
                1,
                1,
                1,
                0,
                0x69,
                0,
                0,
                &result)
            && result.kind
                == REMASTER_EMERALD_STEP_EVENT_IMMEDIATE_SCRIPT
            && result.coord_event_index == 0
            && result.script_id != 0
            && strcmp(result.script_id, "ImmediateScript") == 0,
            "missing immediate-script host must yield explicit handoff"))
        return 1;

    {
        size_t directional_index = SIZE_MAX;

        if (!check(
                remaster_emerald_find_directional_warp(
                    warps,
                    1,
                    1,
                    1,
                    0,
                    0x65,
                    REMASTER_EMERALD_DIR_SOUTH,
                    &directional_index)
                && directional_index == 0,
                "south arrow warp did not match south movement"))
            return 1;

        directional_index = SIZE_MAX;
        if (!check(
                !remaster_emerald_find_directional_warp(
                    warps,
                    1,
                    1,
                    1,
                    0,
                    0x65,
                    REMASTER_EMERALD_DIR_NORTH,
                    &directional_index),
                "south arrow warp must reject north movement"))
            return 1;

        if (!check(
                remaster_emerald_find_directional_warp(
                    warps,
                    1,
                    1,
                    1,
                    0,
                    0x1B,
                    REMASTER_EMERALD_DIR_NORTH,
                    &directional_index),
                "abandoned-ship north stairs must use north arrow semantics"))
            return 1;
    }

    puts("Emerald step-event ordering test passed.");
    return 0;
}
