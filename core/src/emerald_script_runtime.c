#include "remaster/emerald_script_runtime.h"

#include <string.h>

void remaster_emerald_script_runtime_start(
    RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRegistry *registry,
    uint32_t program_index,
    uint32_t entry_pc)
{
    if (runtime == 0)
        return;

    memset(runtime, 0, sizeof(*runtime));
    runtime->next_request_sequence = 1;

    remaster_emerald_script_init_program(
        &runtime->vm,
        save,
        registry,
        program_index,
        entry_pc);
}

RemasterEmeraldScriptStatus remaster_emerald_script_runtime_run(
    RemasterEmeraldScriptRuntime *runtime,
    size_t max_steps)
{
    RemasterEmeraldScriptStatus status;

    if (runtime == 0)
        return REMASTER_EMERALD_SCRIPT_ERROR;

    if (runtime->has_pending_request)
        return REMASTER_EMERALD_SCRIPT_YIELDED;

    status = remaster_emerald_script_run(&runtime->vm, max_steps);

    if (status == REMASTER_EMERALD_SCRIPT_YIELDED) {
        RemasterEmeraldScriptRequest *request = &runtime->pending_request;

        memset(request, 0, sizeof(*request));
        request->type = REMASTER_EMERALD_SCRIPT_REQUEST_WAIT_STATE;
        request->sequence = runtime->next_request_sequence++;
        if (runtime->next_request_sequence == 0)
            runtime->next_request_sequence = 1;
        request->program_index = runtime->vm.program_index;
        request->pc = runtime->vm.pc == 0 ? 0 : runtime->vm.pc - 1u;

        runtime->has_pending_request = 1;
    }

    return status;
}

int remaster_emerald_script_runtime_pending_request(
    const RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldScriptRequest *out_request)
{
    if (runtime == 0
        || out_request == 0
        || !runtime->has_pending_request)
    {
        return 0;
    }

    *out_request = runtime->pending_request;
    return 1;
}

int remaster_emerald_script_runtime_complete(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldScriptCompletion *completion)
{
    if (runtime == 0
        || completion == 0
        || !runtime->has_pending_request)
    {
        return 0;
    }

    if (completion->type != runtime->pending_request.type
        || completion->sequence != runtime->pending_request.sequence
        || !completion->accepted)
    {
        return 0;
    }

    runtime->has_pending_request = 0;
    memset(&runtime->pending_request, 0, sizeof(runtime->pending_request));

    if (runtime->vm.status == REMASTER_EMERALD_SCRIPT_YIELDED)
        runtime->vm.status = REMASTER_EMERALD_SCRIPT_RUNNING;

    return 1;
}
