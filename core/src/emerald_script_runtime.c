#include "remaster/emerald_script_runtime.h"

#include <string.h>

static int runtime_is_busy(const RemasterEmeraldScriptRuntime *runtime)
{
    if (runtime == 0)
        return 0;

    return runtime->has_pending_request
        || runtime->vm.status == REMASTER_EMERALD_SCRIPT_RUNNING
        || runtime->vm.status == REMASTER_EMERALD_SCRIPT_YIELDED
        || runtime->vm.status == REMASTER_EMERALD_SCRIPT_STEP_LIMIT
        || runtime->vm.status == REMASTER_EMERALD_SCRIPT_ERROR;
}

void remaster_emerald_script_runtime_init(
    RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRegistry *registry)
{
    if (runtime == 0)
        return;

    memset(runtime, 0, sizeof(*runtime));
    runtime->next_request_sequence = 1;
    runtime->vm.save = save;
    runtime->vm.registry = registry;
    runtime->vm.status = REMASTER_EMERALD_SCRIPT_HALTED;
}

void remaster_emerald_script_runtime_start(
    RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRegistry *registry,
    uint32_t program_index,
    uint32_t entry_pc)
{
    if (runtime == 0)
        return;

    remaster_emerald_script_runtime_init(runtime, save, registry);

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

static int map_entry_matches(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldMapScriptEntry *entry,
    int *out_valid)
{
    uint16_t lhs;
    uint16_t rhs;
    int valid_lhs = 0;
    int valid_rhs = 0;

    lhs = remaster_emerald_script_value_or_var(
        &runtime->vm,
        entry->lhs,
        &valid_lhs);
    rhs = remaster_emerald_script_value_or_var(
        &runtime->vm,
        entry->rhs,
        &valid_rhs);

    if (!valid_lhs || !valid_rhs) {
        if (out_valid != 0)
            *out_valid = 0;
        return 0;
    }

    if (out_valid != 0)
        *out_valid = 1;
    return lhs == rhs;
}

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_script_id(
    RemasterEmeraldScriptRuntime *runtime,
    const char *script_id)
{
    const RemasterEmeraldScriptRegistry *registry;
    uint16_t special_vars[REMASTER_EMERALD_SPECIAL_VAR_COUNT];
    uint8_t special_flags[REMASTER_EMERALD_SPECIAL_FLAG_BYTES];
    size_t i;

    if (runtime == 0 || script_id == 0)
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;

    if (runtime_is_busy(runtime))
        return REMASTER_EMERALD_SCRIPT_DISPATCH_BUSY;

    registry = runtime->vm.registry;
    if (registry == 0 || registry->programs == 0)
        return REMASTER_EMERALD_SCRIPT_DISPATCH_UNKNOWN_SCRIPT;

    for (i = 0; i < registry->program_count; ++i) {
        const char *candidate = registry->programs[i].script_id;

        if (candidate == 0 || strcmp(candidate, script_id) != 0)
            continue;

        memcpy(
            special_vars,
            runtime->vm.special_vars,
            sizeof(special_vars));
        memcpy(
            special_flags,
            runtime->vm.special_flags,
            sizeof(special_flags));

        remaster_emerald_script_init_program(
            &runtime->vm,
            runtime->vm.save,
            registry,
            (uint32_t)i,
            0);

        memcpy(
            runtime->vm.special_vars,
            special_vars,
            sizeof(special_vars));
        memcpy(
            runtime->vm.special_flags,
            special_flags,
            sizeof(special_flags));

        if (runtime->vm.status == REMASTER_EMERALD_SCRIPT_ERROR)
            return REMASTER_EMERALD_SCRIPT_DISPATCH_STATE_ERROR;

        return REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED;
    }

    return REMASTER_EMERALD_SCRIPT_DISPATCH_UNKNOWN_SCRIPT;
}

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_run_map_hook(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldMapScriptEntry *entries,
    size_t entry_count,
    RemasterEmeraldMapScriptHook hook,
    size_t max_steps,
    RemasterEmeraldScriptStatus *out_status)
{
    size_t i;

    if (out_status != 0)
        *out_status = REMASTER_EMERALD_SCRIPT_HALTED;

    if (runtime == 0 || entries == 0)
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;

    for (i = 0; i < entry_count; ++i) {
        RemasterEmeraldScriptDispatchResult result;
        int valid = 1;

        if (entries[i].hook != hook)
            continue;

        if (hook == REMASTER_EMERALD_MAP_SCRIPT_ON_FRAME_TABLE
            || hook == REMASTER_EMERALD_MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE)
        {
            if (!map_entry_matches(runtime, &entries[i], &valid)) {
                if (!valid)
                    return REMASTER_EMERALD_SCRIPT_DISPATCH_STATE_ERROR;
                continue;
            }
        }

        result = remaster_emerald_script_runtime_dispatch_script_id(
            runtime,
            entries[i].script_id);
        if (result != REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED)
            return result;

        if (hook == REMASTER_EMERALD_MAP_SCRIPT_ON_FRAME_TABLE)
            return result;

        if (out_status != 0)
            *out_status = remaster_emerald_script_runtime_run(
                runtime,
                max_steps);
        else
            (void)remaster_emerald_script_runtime_run(runtime, max_steps);

        return result;
    }

    return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;
}

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_try_frame_table(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldMapScriptEntry *entries,
    size_t entry_count)
{
    size_t i;

    if (runtime == 0 || entries == 0)
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;

    if (runtime_is_busy(runtime))
        return REMASTER_EMERALD_SCRIPT_DISPATCH_BUSY;

    for (i = 0; i < entry_count; ++i) {
        int valid = 1;

        if (entries[i].hook
            != REMASTER_EMERALD_MAP_SCRIPT_ON_FRAME_TABLE)
        {
            continue;
        }

        if (!map_entry_matches(runtime, &entries[i], &valid)) {
            if (!valid)
                return REMASTER_EMERALD_SCRIPT_DISPATCH_STATE_ERROR;
            continue;
        }

        return remaster_emerald_script_runtime_dispatch_script_id(
            runtime,
            entries[i].script_id);
    }

    return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;
}

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_object(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldObjectEventDef *event)
{
    if (runtime == 0 || event == 0 || event->script_id == 0)
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;

    if (!remaster_emerald_object_event_visible(runtime->vm.save, event))
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;

    return remaster_emerald_script_runtime_dispatch_script_id(
        runtime,
        event->script_id);
}

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_coord(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldCoordEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation)
{
    RemasterEmeraldCoordMatch match;

    if (runtime == 0 || events == 0)
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;

    match = remaster_emerald_find_coord_event(
        runtime->vm.save,
        events,
        event_count,
        x,
        y,
        elevation);

    if (match.kind != REMASTER_EMERALD_COORD_MATCH_SCRIPT
        || match.event_index >= event_count
        || events[match.event_index].script_id == 0)
    {
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;
    }

    return remaster_emerald_script_runtime_dispatch_script_id(
        runtime,
        events[match.event_index].script_id);
}

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_background(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldBackgroundEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    uint8_t facing_direction)
{
    RemasterEmeraldBackgroundMatch match;

    if (runtime == 0 || events == 0)
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;

    match = remaster_emerald_find_background_event(
        runtime->vm.save,
        events,
        event_count,
        x,
        y,
        elevation,
        facing_direction);

    if (match.kind != REMASTER_EMERALD_BG_MATCH_SCRIPT
        || match.event_index >= event_count
        || events[match.event_index].script_id == 0)
    {
        return REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH;
    }

    return remaster_emerald_script_runtime_dispatch_script_id(
        runtime,
        events[match.event_index].script_id);
}

