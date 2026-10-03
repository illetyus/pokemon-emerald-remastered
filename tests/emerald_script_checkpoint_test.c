#include "remaster/emerald_script_runtime.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_script_checkpoint_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldScriptRuntime runtime;
    RemasterEmeraldScriptRuntime restored;
    RemasterEmeraldScriptRequest request;
    RemasterEmeraldScriptCompletion completion;
    uint8_t checkpoint[1024];
    uint8_t invalid_checkpoint[1024];
    size_t checkpoint_size;
    uint16_t value = 0;

    const RemasterEmeraldScriptInstruction caller_script[] = {
        {
            .opcode = REMASTER_EMERALD_SCRIPT_CALL,
            .target = 0,
            .target_program = 1,
            .target_program_valid = 1,
        },
        { .opcode = REMASTER_EMERALD_SCRIPT_END },
    };
    const RemasterEmeraldScriptInstruction callee_script[] = {
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR,
            .a = 0x4020,
            .b = 9,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_MESSAGE,
            .b = 2,
            .resource_id = "Text_Checkpoint",
        },
        { .opcode = REMASTER_EMERALD_SCRIPT_RETURN },
    };
    const RemasterEmeraldScriptProgram programs[] = {
        {
            .script_id = "Checkpoint_Caller",
            .instructions = caller_script,
            .instruction_count = sizeof(caller_script) / sizeof(caller_script[0]),
        },
        {
            .script_id = "Checkpoint_Callee",
            .instructions = callee_script,
            .instruction_count = sizeof(callee_script) / sizeof(callee_script[0]),
        },
    };
    const RemasterEmeraldScriptRegistry registry = {
        programs,
        sizeof(programs) / sizeof(programs[0]),
    };

    memset(&save, 0, sizeof(save));
    memset(&runtime, 0, sizeof(runtime));
    memset(&restored, 0, sizeof(restored));
    memset(checkpoint, 0, sizeof(checkpoint));

    checkpoint_size = remaster_emerald_script_runtime_checkpoint_size();
    if (!check(
            checkpoint_size > 0 && checkpoint_size <= sizeof(checkpoint),
            "checkpoint size is invalid"))
        return 1;

    remaster_emerald_script_runtime_start(
        &runtime,
        &save,
        &registry,
        0,
        0);
    runtime.vm.comparison_result = REMASTER_EMERALD_COMPARE_GREATER;
    runtime.vm.special_vars[0] = 0xCAFE;
    runtime.vm.special_flags[0] = 0x5A;

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED,
            "fixture did not yield inside callee"))
        return 1;
    if (!check(
            runtime.vm.program_index == 1
                && runtime.vm.stack_depth == 1
                && remaster_emerald_script_runtime_pending_request(
                    &runtime,
                    &request)
                && request.type == REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE
                && request.resource_id != 0
                && strcmp(request.resource_id, "Text_Checkpoint") == 0,
            "pre-checkpoint yielded state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_checkpoint_write(
                &runtime,
                checkpoint,
                checkpoint_size),
            "checkpoint write failed"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_checkpoint_read(
                &restored,
                &registry,
                &save,
                checkpoint,
                checkpoint_size),
            "checkpoint read failed"))
        return 1;

    if (!check(
            restored.vm.status == REMASTER_EMERALD_SCRIPT_YIELDED
                && restored.vm.program_index == 1
                && restored.vm.pc == runtime.vm.pc
                && restored.vm.stack_depth == 1
                && restored.vm.stack[0].program_index == 0
                && restored.vm.stack[0].return_pc == 1
                && restored.vm.comparison_result
                    == REMASTER_EMERALD_COMPARE_GREATER
                && restored.vm.special_vars[0] == 0xCAFE
                && restored.vm.special_flags[0] == 0x5A,
            "restored VM state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_pending_request(
                &restored,
                &request)
                && request.type == REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE
                && request.sequence == 1
                && request.program_index == 1
                && request.pc == 1
                && request.resource_id != 0
                && strcmp(request.resource_id, "Text_Checkpoint") == 0,
            "pending request was not restored exactly"))
        return 1;

    memset(&completion, 0, sizeof(completion));
    completion.type = request.type;
    completion.sequence = request.sequence + 1;
    completion.accepted = 1;
    if (!check(
            !remaster_emerald_script_runtime_complete(
                &restored,
                &completion),
            "stale completion was accepted after restore"))
        return 1;

    completion.sequence = request.sequence;
    if (!check(
            remaster_emerald_script_runtime_complete(
                &restored,
                &completion),
            "matching completion was rejected after restore"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_run(&restored, 100)
                == REMASTER_EMERALD_SCRIPT_HALTED,
            "restored script did not return to caller and halt"))
        return 1;
    if (!check(
            remaster_emerald_var_get(&save, 0x4020, &value)
                && value == 9,
            "persistent save mutation changed across checkpoint"))
        return 1;

    /* A not-yet-run script is a valid RUNNING checkpoint. */
    remaster_emerald_script_runtime_start(
        &runtime,
        &save,
        &registry,
        0,
        0);
    runtime.vm.special_vars[1] = 0x1234;
    if (!check(
            remaster_emerald_script_runtime_checkpoint_write(
                &runtime,
                checkpoint,
                checkpoint_size)
                && remaster_emerald_script_runtime_checkpoint_read(
                    &restored,
                    &registry,
                    &save,
                    checkpoint,
                    checkpoint_size)
                && restored.vm.status == REMASTER_EMERALD_SCRIPT_RUNNING
                && restored.vm.program_index == 0
                && restored.vm.pc == 0
                && restored.vm.special_vars[1] == 0x1234
                && !restored.has_pending_request,
            "running checkpoint round-trip mismatch"))
        return 1;

    memcpy(invalid_checkpoint, checkpoint, checkpoint_size);
    invalid_checkpoint[4] = 0xFF;
    invalid_checkpoint[5] = 0xFF;
    memset(&restored, 0, sizeof(restored));
    restored.next_request_sequence = 777;
    if (!check(
            !remaster_emerald_script_runtime_checkpoint_read(
                &restored,
                &registry,
                &save,
                invalid_checkpoint,
                checkpoint_size)
                && restored.next_request_sequence == 777,
            "unknown checkpoint version mutated destination runtime"))
        return 1;

    if (!check(
            !remaster_emerald_script_runtime_checkpoint_read(
                &restored,
                &registry,
                &save,
                checkpoint,
                checkpoint_size - 1)
                && restored.next_request_sequence == 777,
            "truncated checkpoint mutated destination runtime"))
        return 1;

    remaster_emerald_script_runtime_init(&runtime, &save, &registry);
    if (!check(
            runtime.vm.status == REMASTER_EMERALD_SCRIPT_HALTED
                && !runtime.has_pending_request,
            "plain save/runtime init invented an active script"))
        return 1;

    puts("Emerald script checkpoint serialization test passed.");
    return 0;
}
