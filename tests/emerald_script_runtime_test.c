#include "remaster/emerald_script_host.h"
#include "remaster/emerald_script_runtime.h"
#include "remaster/emerald_events.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_script_runtime_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldScriptRuntime runtime;
    RemasterEmeraldScriptRequest request;
    RemasterEmeraldScriptRequest observed;
    RemasterEmeraldScriptCompletion completion;
    int flag = 0;

    const RemasterEmeraldScriptInstruction instructions[] = {
        { .opcode = REMASTER_EMERALD_SCRIPT_WAIT_STATE },
        { .opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG, .a = 0x0100 },
        { .opcode = REMASTER_EMERALD_SCRIPT_WAIT_STATE },
        { .opcode = REMASTER_EMERALD_SCRIPT_END },
    };
    const RemasterEmeraldScriptProgram programs[] = {
        {
            .script_id = "Yield_Test",
            .instructions = instructions,
            .instruction_count = sizeof(instructions) / sizeof(instructions[0]),
        },
    };
    const RemasterEmeraldScriptRegistry registry = {
        programs,
        sizeof(programs) / sizeof(programs[0]),
    };

    memset(&save, 0, sizeof(save));
    memset(&runtime, 0, sizeof(runtime));

    remaster_emerald_script_runtime_start(
        &runtime,
        &save,
        &registry,
        0,
        0);

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED,
            "first waitstate should yield"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_pending_request(
                &runtime,
                &request),
            "yield should expose a pending request"))
        return 1;

    if (!check(
            request.type == REMASTER_EMERALD_SCRIPT_REQUEST_WAIT_STATE
                && request.sequence == 1
                && request.program_index == 0
                && request.pc == 0,
            "first request metadata mismatch"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&save, 0x0100, &flag)
                && flag == 0,
            "core mutated state before host completion"))
        return 1;

    completion.type = REMASTER_EMERALD_SCRIPT_REQUEST_WAIT_STATE;
    completion.sequence = 0;
    completion.result_u16 = 0;
    completion.accepted = 1;
    if (!check(
            !remaster_emerald_script_runtime_complete(
                &runtime,
                &completion),
            "stale completion should be rejected"))
        return 1;

    completion.type = REMASTER_EMERALD_SCRIPT_REQUEST_NONE;
    completion.sequence = request.sequence;
    if (!check(
            !remaster_emerald_script_runtime_complete(
                &runtime,
                &completion),
            "wrong completion type should be rejected"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_pending_request(
                &runtime,
                &observed)
                && observed.sequence == request.sequence
                && observed.type == request.type
                && runtime.vm.pc == 1,
            "rejected completion changed pending continuation"))
        return 1;

    completion.type = request.type;
    completion.sequence = request.sequence;
    completion.accepted = 1;
    if (!check(
            remaster_emerald_script_runtime_complete(
                &runtime,
                &completion),
            "matching completion should be accepted"))
        return 1;

    if (!check(
            !remaster_emerald_script_runtime_pending_request(
                &runtime,
                &observed),
            "accepted completion did not clear pending request"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED,
            "second waitstate should yield"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&save, 0x0100, &flag)
                && flag == 1,
            "resumed VM did not perform authoritative mutation"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_pending_request(
                &runtime,
                &observed)
                && observed.sequence == 2
                && observed.type
                    == REMASTER_EMERALD_SCRIPT_REQUEST_WAIT_STATE
                && observed.pc == 2,
            "second request sequence/location mismatch"))
        return 1;

    completion.type = REMASTER_EMERALD_SCRIPT_REQUEST_WAIT_STATE;
    completion.sequence = 1;
    completion.accepted = 1;
    if (!check(
            !remaster_emerald_script_runtime_complete(
                &runtime,
                &completion),
            "duplicate old completion should be rejected"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_pending_request(
                &runtime,
                &request)
                && request.sequence == 2,
            "duplicate completion disturbed current request"))
        return 1;

    completion.type = request.type;
    completion.sequence = request.sequence;
    completion.accepted = 1;
    if (!check(
            remaster_emerald_script_runtime_complete(
                &runtime,
                &completion),
            "second matching completion should be accepted"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_HALTED,
            "runtime did not halt after final completion"))
        return 1;


    {
        RemasterEmeraldScriptRuntime scheduler;
        RemasterEmeraldScriptStatus hook_status;
        RemasterEmeraldObjectEventDef object_event;
        RemasterEmeraldCoordEventDef coord_event;
        RemasterEmeraldBackgroundEventDef background_event;
        uint16_t value = 0;

        const RemasterEmeraldScriptInstruction load_script[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR, .a = 0x4020, .b = 1 },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptInstruction transition_script[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR, .a = 0x4021, .b = 2 },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptInstruction frame_script[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG, .a = 0x0101 },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptInstruction warp_script[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR, .a = 0x4022, .b = 3 },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptInstruction object_script[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG, .a = 0x0102 },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptInstruction coord_script[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG, .a = 0x0103 },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptInstruction background_script[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG, .a = 0x0104 },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptProgram scheduler_programs[] = {
            { "OnLoad", load_script, 2 },
            { "OnTransition", transition_script, 2 },
            { "OnFrame", frame_script, 2 },
            { "OnWarp", warp_script, 2 },
            { "ObjectScript", object_script, 2 },
            { "CoordScript", coord_script, 2 },
            { "BackgroundScript", background_script, 2 },
        };
        const RemasterEmeraldScriptRegistry scheduler_registry = {
            scheduler_programs,
            sizeof(scheduler_programs) / sizeof(scheduler_programs[0]),
        };
        const RemasterEmeraldMapScriptEntry map_scripts[] = {
            {
                REMASTER_EMERALD_MAP_SCRIPT_ON_LOAD,
                0,
                0,
                "OnLoad",
            },
            {
                REMASTER_EMERALD_MAP_SCRIPT_ON_TRANSITION,
                0,
                0,
                "OnTransition",
            },
            {
                REMASTER_EMERALD_MAP_SCRIPT_ON_FRAME_TABLE,
                0x4025,
                1,
                "OnFrame",
            },
            {
                REMASTER_EMERALD_MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE,
                0x4026,
                1,
                "OnWarp",
            },
        };

        memset(&save, 0, sizeof(save));
        remaster_emerald_script_runtime_init(
            &scheduler,
            &save,
            &scheduler_registry);

        if (!check(
                remaster_emerald_script_runtime_run_map_hook(
                    &scheduler,
                    map_scripts,
                    sizeof(map_scripts) / sizeof(map_scripts[0]),
                    REMASTER_EMERALD_MAP_SCRIPT_ON_LOAD,
                    100,
                    &hook_status)
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
                && hook_status == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_var_get(&save, 0x4020, &value)
                && value == 1,
                "OnLoad did not execute immediately"))
            return 1;

        if (!check(
                remaster_emerald_script_runtime_run_map_hook(
                    &scheduler,
                    map_scripts,
                    sizeof(map_scripts) / sizeof(map_scripts[0]),
                    REMASTER_EMERALD_MAP_SCRIPT_ON_TRANSITION,
                    100,
                    &hook_status)
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
                && hook_status == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_var_get(&save, 0x4021, &value)
                && value == 2,
                "OnTransition did not execute immediately"))
            return 1;

        if (!check(
                remaster_emerald_script_runtime_try_frame_table(
                    &scheduler,
                    map_scripts,
                    sizeof(map_scripts) / sizeof(map_scripts[0]))
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH,
                "frame table matched wrong var state"))
            return 1;

        if (!check(
                remaster_emerald_var_set(&save, 0x4025, 1)
                && remaster_emerald_script_runtime_try_frame_table(
                    &scheduler,
                    map_scripts,
                    sizeof(map_scripts) / sizeof(map_scripts[0]))
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
                && scheduler.vm.status == REMASTER_EMERALD_SCRIPT_RUNNING,
                "matching frame table did not schedule script"))
            return 1;

        if (!check(
                remaster_emerald_script_runtime_dispatch_script_id(
                    &scheduler,
                    "ObjectScript")
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_BUSY,
                "active frame script was overwritten"))
            return 1;

        if (!check(
                remaster_emerald_script_runtime_run(&scheduler, 100)
                    == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_flag_get(&save, 0x0101, &flag)
                && flag == 1,
                "scheduled frame script did not run"))
            return 1;

        if (!check(
                remaster_emerald_var_set(&save, 0x4026, 1)
                && remaster_emerald_script_runtime_run_map_hook(
                    &scheduler,
                    map_scripts,
                    sizeof(map_scripts) / sizeof(map_scripts[0]),
                    REMASTER_EMERALD_MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE,
                    100,
                    &hook_status)
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
                && hook_status == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_var_get(&save, 0x4022, &value)
                && value == 3,
                "OnWarp table did not execute matching script"))
            return 1;

        memset(&object_event, 0, sizeof(object_event));
        object_event.local_id = 1;
        object_event.script_id = "ObjectScript";
        if (!check(
                remaster_emerald_script_runtime_dispatch_object(
                    &scheduler,
                    &object_event)
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
                && remaster_emerald_script_runtime_run(&scheduler, 100)
                    == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_flag_get(&save, 0x0102, &flag)
                && flag == 1,
                "object interaction did not dispatch stable script id"))
            return 1;

        memset(&coord_event, 0, sizeof(coord_event));
        coord_event.kind = REMASTER_EMERALD_COORD_TRIGGER;
        coord_event.x = 4;
        coord_event.y = 5;
        coord_event.elevation = 0;
        coord_event.trigger = 0;
        coord_event.script_id = "CoordScript";
        if (!check(
                remaster_emerald_script_runtime_dispatch_coord(
                    &scheduler,
                    &coord_event,
                    1,
                    4,
                    5,
                    3)
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
                && remaster_emerald_script_runtime_run(&scheduler, 100)
                    == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_flag_get(&save, 0x0103, &flag)
                && flag == 1,
                "coord event did not dispatch stable script id"))
            return 1;

        memset(&background_event, 0, sizeof(background_event));
        background_event.x = 7;
        background_event.y = 8;
        background_event.elevation = 0;
        background_event.kind = REMASTER_EMERALD_BG_FACING_ANY;
        background_event.script_id = "BackgroundScript";
        if (!check(
                remaster_emerald_script_runtime_dispatch_background(
                    &scheduler,
                    &background_event,
                    1,
                    7,
                    8,
                    2,
                    REMASTER_EMERALD_BG_FACING_NORTH)
                    == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
                && remaster_emerald_script_runtime_run(&scheduler, 100)
                    == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_flag_get(&save, 0x0104, &flag)
                && flag == 1,
                "background event did not dispatch stable script id"))
            return 1;
    }

    puts("Emerald script runtime yield/resume test passed.");
    return 0;
}
