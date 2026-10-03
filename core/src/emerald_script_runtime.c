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

void remaster_emerald_script_runtime_set_special_registry(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldSpecialRegistry *special_registry)
{
    if (runtime == 0)
        return;

    runtime->special_registry = special_registry;
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

static const RemasterEmeraldSpecialBinding *find_special_binding(
    const RemasterEmeraldScriptRuntime *runtime,
    const char *special_id,
    uint32_t special_index)
{
    size_t i;

    if (runtime == 0
        || runtime->special_registry == 0
        || runtime->special_registry->bindings == 0
        || special_id == 0)
    {
        return 0;
    }

    for (i = 0; i < runtime->special_registry->binding_count; ++i) {
        const RemasterEmeraldSpecialBinding *binding =
            &runtime->special_registry->bindings[i];

        if (binding->special_id != 0
            && strcmp(binding->special_id, special_id) == 0
            && binding->special_index == special_index)
        {
            return binding;
        }
    }

    return 0;
}

static RemasterEmeraldScriptStatus fail_unknown_special(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldScriptInstruction *ins,
    uint32_t pc)
{
    const char *script_id = 0;

    if (runtime->vm.registry != 0
        && runtime->vm.registry->programs != 0
        && runtime->vm.program_index < runtime->vm.registry->program_count)
    {
        script_id =
            runtime->vm.registry->programs[runtime->vm.program_index].script_id;
    }

    runtime->vm.error.code = REMASTER_EMERALD_SCRIPT_ERROR_UNKNOWN_SPECIAL;
    runtime->vm.error.script_id = script_id;
    runtime->vm.error.program_index = runtime->vm.program_index;
    runtime->vm.error.pc = pc;
    runtime->vm.error.opcode =
        ins != 0 ? ins->opcode : REMASTER_EMERALD_SCRIPT_SPECIAL;
    runtime->vm.status = REMASTER_EMERALD_SCRIPT_ERROR;
    return runtime->vm.status;
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
        const RemasterEmeraldScriptInstruction *ins = 0;
        uint32_t yielded_pc =
            runtime->vm.pc == 0 ? 0 : runtime->vm.pc - 1u;

        if (runtime->vm.program != 0
            && yielded_pc < runtime->vm.program_count)
        {
            ins = &runtime->vm.program[yielded_pc];
        }

        memset(request, 0, sizeof(*request));
        request->type = REMASTER_EMERALD_SCRIPT_REQUEST_WAIT_STATE;
        request->sequence = runtime->next_request_sequence++;
        if (runtime->next_request_sequence == 0)
            runtime->next_request_sequence = 1;
        request->program_index = runtime->vm.program_index;
        request->pc = yielded_pc;

        if (ins != 0) {
            request->local_id = ins->a;
            request->map_id = ins->map_id;
            request->resource_id = ins->resource_id;
            request->x = ins->x;
            request->y = ins->y;
            request->value_u16 = ins->b;
            request->value_u32 = ins->value_u32;
            request->quantity = ins->b;

            switch (ins->opcode) {
            case REMASTER_EMERALD_SCRIPT_APPLY_MOVEMENT:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_MOVEMENT;
                request->action = REMASTER_EMERALD_SCRIPT_MOVEMENT_START;
                break;

            case REMASTER_EMERALD_SCRIPT_WAIT_MOVEMENT:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_MOVEMENT;
                request->action = REMASTER_EMERALD_SCRIPT_MOVEMENT_WAIT;
                break;

            case REMASTER_EMERALD_SCRIPT_ADD_OBJECT:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT;
                request->action = REMASTER_EMERALD_SCRIPT_OBJECT_ADD;
                break;

            case REMASTER_EMERALD_SCRIPT_REMOVE_OBJECT:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT;
                request->action = REMASTER_EMERALD_SCRIPT_OBJECT_REMOVE;
                break;

            case REMASTER_EMERALD_SCRIPT_SHOW_OBJECT:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT;
                request->action = REMASTER_EMERALD_SCRIPT_OBJECT_SHOW;
                break;

            case REMASTER_EMERALD_SCRIPT_HIDE_OBJECT:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT;
                request->action = REMASTER_EMERALD_SCRIPT_OBJECT_HIDE;
                break;

            case REMASTER_EMERALD_SCRIPT_SET_OBJECT_XY:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT;
                request->action = REMASTER_EMERALD_SCRIPT_OBJECT_SET_XY;
                break;

            case REMASTER_EMERALD_SCRIPT_TURN_OBJECT:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT;
                request->action = REMASTER_EMERALD_SCRIPT_OBJECT_TURN;
                break;

            case REMASTER_EMERALD_SCRIPT_FACE_PLAYER:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT;
                request->action = REMASTER_EMERALD_SCRIPT_OBJECT_FACE_PLAYER;
                break;

            case REMASTER_EMERALD_SCRIPT_MESSAGE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE;
                request->action = REMASTER_EMERALD_SCRIPT_MESSAGE_SHOW;
                request->value_u16 = ins->b;
                break;

            case REMASTER_EMERALD_SCRIPT_CLOSE_MESSAGE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE;
                request->action = REMASTER_EMERALD_SCRIPT_MESSAGE_CLOSE;
                break;

            case REMASTER_EMERALD_SCRIPT_WAIT_MESSAGE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE;
                request->action = REMASTER_EMERALD_SCRIPT_MESSAGE_WAIT;
                break;

            case REMASTER_EMERALD_SCRIPT_CHOICE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE;
                request->result_var = 0x800D;
                break;

            case REMASTER_EMERALD_SCRIPT_DELAY:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DELAY;
                break;

            case REMASTER_EMERALD_SCRIPT_PLAY_SOUND:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_SOUND;
                request->action = REMASTER_EMERALD_SCRIPT_ASYNC_START;
                break;

            case REMASTER_EMERALD_SCRIPT_WAIT_SOUND:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_SOUND;
                request->action = REMASTER_EMERALD_SCRIPT_ASYNC_WAIT;
                break;

            case REMASTER_EMERALD_SCRIPT_PLAY_FANFARE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_FANFARE;
                request->action = REMASTER_EMERALD_SCRIPT_ASYNC_START;
                break;

            case REMASTER_EMERALD_SCRIPT_WAIT_FANFARE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_FANFARE;
                request->action = REMASTER_EMERALD_SCRIPT_ASYNC_WAIT;
                break;

            case REMASTER_EMERALD_SCRIPT_PLAY_BGM:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_BGM;
                request->action = REMASTER_EMERALD_SCRIPT_ASYNC_START;
                break;

            case REMASTER_EMERALD_SCRIPT_FADE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_FADE;
                break;

            case REMASTER_EMERALD_SCRIPT_OPEN_DOOR:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOOR;
                request->action = REMASTER_EMERALD_SCRIPT_DOOR_OPEN;
                break;

            case REMASTER_EMERALD_SCRIPT_CLOSE_DOOR:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOOR;
                request->action = REMASTER_EMERALD_SCRIPT_DOOR_CLOSE;
                break;

            case REMASTER_EMERALD_SCRIPT_WAIT_DOOR:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOOR;
                request->action = REMASTER_EMERALD_SCRIPT_DOOR_WAIT;
                break;

            case REMASTER_EMERALD_SCRIPT_WARP:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_WARP;
                break;

            case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_ADD:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN;
                request->action =
                    REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_ITEM_ADD;
                request->value_u16 = ins->a;
                request->quantity = ins->b;
                request->result_var = 0x800D;
                break;

            case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_REMOVE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN;
                request->action =
                    REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_ITEM_REMOVE;
                request->value_u16 = ins->a;
                request->quantity = ins->b;
                request->result_var = 0x800D;
                break;

            case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_CHECK:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN;
                request->action =
                    REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_ITEM_CHECK;
                request->value_u16 = ins->a;
                request->quantity = ins->b;
                request->result_var = 0x800D;
                break;

            case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_SPACE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN;
                request->action =
                    REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_ITEM_SPACE;
                request->value_u16 = ins->a;
                request->quantity = ins->b;
                request->result_var = 0x800D;
                break;

            case REMASTER_EMERALD_SCRIPT_DOMAIN_GIVE_MON:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN;
                request->action =
                    REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_GIVE_MON;
                request->value_u16 = ins->a;
                request->quantity = ins->b;
                request->result_var = 0x800D;
                break;

            case REMASTER_EMERALD_SCRIPT_DOMAIN_HEAL_PARTY:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN;
                request->action =
                    REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_HEAL_PARTY;
                break;

            case REMASTER_EMERALD_SCRIPT_DOMAIN_PARTY_SIZE:
                request->type = REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN;
                request->action =
                    REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_PARTY_SIZE;
                request->result_var = 0x800D;
                break;

            case REMASTER_EMERALD_SCRIPT_SPECIAL:
            case REMASTER_EMERALD_SCRIPT_SPECIAL_VAR:
            {
                const RemasterEmeraldSpecialBinding *binding =
                    find_special_binding(
                        runtime,
                        ins->resource_id,
                        ins->value_u32);

                if (binding == 0) {
                    memset(request, 0, sizeof(*request));
                    return fail_unknown_special(
                        runtime,
                        ins,
                        yielded_pc);
                }

                request->type = binding->request_type;
                request->action = binding->action;
                request->resource_id = ins->resource_id;
                request->value_u32 = ins->value_u32;
                request->result_var =
                    ins->opcode == REMASTER_EMERALD_SCRIPT_SPECIAL_VAR
                        ? ins->a
                        : binding->result_var;
                break;
            }

            default:
                break;
            }
        }

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

    if ((runtime->pending_request.type
            == REMASTER_EMERALD_SCRIPT_REQUEST_MOVEMENT
         || runtime->pending_request.type
            == REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT)
        && (completion->local_id != runtime->pending_request.local_id
            || completion->map_id != runtime->pending_request.map_id))
    {
        return 0;
    }

    if (runtime->pending_request.result_var != 0u
        && !remaster_emerald_script_var_set(
            &runtime->vm,
            runtime->pending_request.result_var,
            completion->result_u16))
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

