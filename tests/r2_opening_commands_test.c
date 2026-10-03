#include "remaster/emerald_script_host.h"
#include "remaster/emerald_script_runtime.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "r2_opening_commands_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int acknowledge(
    RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldScriptRequest *out_request)
{
    RemasterEmeraldScriptCompletion completion;

    if (!remaster_emerald_script_runtime_pending_request(
            runtime,
            out_request))
    {
        return 0;
    }

    memset(&completion, 0, sizeof(completion));
    completion.type = out_request->type;
    completion.sequence = out_request->sequence;
    completion.local_id = out_request->local_id;
    completion.map_id = out_request->map_id;
    completion.accepted = 1;
    return remaster_emerald_script_runtime_complete(runtime, &completion);
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldScriptRuntime runtime;
    RemasterEmeraldScriptRequest request;
    RemasterEmeraldOverworldState state;

    const RemasterEmeraldScriptInstruction script[] = {
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR,
            .a = 0x8004,
            .b = 5,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR,
            .a = 0x8005,
            .b = 8,
        },
        { .opcode = REMASTER_EMERALD_SCRIPT_LOCK_ALL },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SET_METATILE,
            .a = 0x8004,
            .b = 0x8005,
            .value_u32 = 0x123,
            .target = 1,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SET_FOLLOWER,
            .a = 7,
            .b = 0x7E,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_FOLLOWER_INTO_POKEBALL,
        },
        {
            .opcode =
                REMASTER_EMERALD_SCRIPT_UPDATE_FOLLOWER_POKEMON_GRAPHIC,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_INCREMENT_GAME_STAT,
            .a = 3,
        },
        {
            .opcode =
                REMASTER_EMERALD_SCRIPT_BUFFER_LEAD_MON_SPECIES_NAME,
            .a = 1,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SAVE_BGM,
            .a = 77,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_FADE_DEFAULT_BGM,
        },
        { .opcode = REMASTER_EMERALD_SCRIPT_RELEASE_ALL },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_WARP,
            .a = 0x8004,
            .b = 0x8005,
            .map_id = 0x0104,
        },
        { .opcode = REMASTER_EMERALD_SCRIPT_END },
    };
    const RemasterEmeraldScriptProgram programs[] = {
        {
            .script_id = "R2_Opening_Commands",
            .instructions = script,
            .instruction_count = sizeof(script) / sizeof(script[0]),
        },
    };
    const RemasterEmeraldScriptRegistry registry = {
        programs,
        1,
    };

    memset(&save, 0, sizeof(save));
    memset(&state, 0, sizeof(state));
    if (!check(
            remaster_emerald_overworld_set(&save, &state),
            "failed to seed overworld state"))
        return 1;

    remaster_emerald_script_runtime_start(
        &runtime,
        &save,
        &registry,
        0,
        0);

#define RUN_AND_ACK(expected_type, expected_action)                           \
    do {                                                                       \
        if (!check(                                                            \
                remaster_emerald_script_runtime_run(&runtime, 100)             \
                    == REMASTER_EMERALD_SCRIPT_YIELDED,                        \
                "expected command did not yield"))                            \
            return 1;                                                          \
        if (!check(                                                            \
                remaster_emerald_script_runtime_pending_request(               \
                    &runtime,                                                  \
                    &request)                                                  \
                    && request.type == (expected_type)                         \
                    && request.action == (expected_action),                    \
                "typed opening request mismatch"))                            \
            return 1;                                                          \
        if (!check(acknowledge(&runtime, &request), "request ack failed"))     \
            return 1;                                                          \
    } while (0)

    RUN_AND_ACK(
        REMASTER_EMERALD_SCRIPT_REQUEST_WORLD,
        REMASTER_EMERALD_SCRIPT_WORLD_LOCK_ALL);

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED
            && remaster_emerald_script_runtime_pending_request(
                &runtime,
                &request)
            && request.type == REMASTER_EMERALD_SCRIPT_REQUEST_WORLD
            && request.action == REMASTER_EMERALD_SCRIPT_WORLD_SET_METATILE
            && request.x == 5
            && request.y == 8
            && request.value_u16 == 0x123
            && request.quantity == 1,
            "setmetatile request did not preserve VarGet operands"))
        return 1;
    if (!check(acknowledge(&runtime, &request), "setmetatile ack failed"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED
            && remaster_emerald_script_runtime_pending_request(
                &runtime,
                &request)
            && request.type == REMASTER_EMERALD_SCRIPT_REQUEST_WORLD
            && request.action == REMASTER_EMERALD_SCRIPT_WORLD_SET_FOLLOWER
            && request.local_id == 7
            && request.value_u16 == 0x7E,
            "setfollower request mismatch"))
        return 1;
    if (!check(acknowledge(&runtime, &request), "setfollower ack failed"))
        return 1;

    RUN_AND_ACK(
        REMASTER_EMERALD_SCRIPT_REQUEST_WORLD,
        REMASTER_EMERALD_SCRIPT_WORLD_FOLLOWER_INTO_POKEBALL);
    RUN_AND_ACK(
        REMASTER_EMERALD_SCRIPT_REQUEST_WORLD,
        REMASTER_EMERALD_SCRIPT_WORLD_UPDATE_FOLLOWER_GRAPHIC);
    RUN_AND_ACK(
        REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN,
        REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_INCREMENT_GAME_STAT);
    RUN_AND_ACK(
        REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN,
        REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_BUFFER_LEAD_MON_SPECIES_NAME);

    RUN_AND_ACK(
        REMASTER_EMERALD_SCRIPT_REQUEST_BGM,
        REMASTER_EMERALD_SCRIPT_BGM_FADE_DEFAULT);

    if (!check(
            remaster_emerald_overworld_get(&save, &state)
                && state.saved_music == 77,
            "savebgm did not update authoritative saved music"))
        return 1;
    RUN_AND_ACK(
        REMASTER_EMERALD_SCRIPT_REQUEST_WORLD,
        REMASTER_EMERALD_SCRIPT_WORLD_RELEASE_ALL);

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED
            && remaster_emerald_script_runtime_pending_request(
                &runtime,
                &request)
            && request.type == REMASTER_EMERALD_SCRIPT_REQUEST_WARP
            && request.map_id == 0x0104
            && request.x == 5
            && request.y == 8,
            "warp request did not resolve VarGet coordinates"))
        return 1;
    if (!check(acknowledge(&runtime, &request), "warp ack failed"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_HALTED,
            "opening command script did not halt"))
        return 1;

#undef RUN_AND_ACK

    puts("R2 opening command adapter test passed.");
    return 0;
}
