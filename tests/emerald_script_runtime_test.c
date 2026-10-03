#include "remaster/emerald_script_host.h"
#include "remaster/emerald_script_runtime.h"
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

    puts("Emerald script runtime yield/resume test passed.");
    return 0;
}
