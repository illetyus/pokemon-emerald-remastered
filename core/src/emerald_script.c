#include "remaster/emerald_script.h"
#include "remaster/emerald_state.h"
#include "remaster/emerald_object_state.h"

#include <string.h>

enum {
    VARS_START = 0x4000,
    VARS_END = 0x40FF,
    SPECIAL_VARS_START = 0x8000,
    SPECIAL_VARS_END = 0x8015,

    SPECIAL_FLAGS_START = 0x4000,
    SPECIAL_FLAGS_END = 0x407F,
    VAR_RESULT = 0x800D,
    MAX_MONEY = 999999
};

static const uint8_t kConditionTable[6][3] = {
    /*             <  =  > */
    /* <  */      {1, 0, 0},
    /* =  */      {0, 1, 0},
    /* >  */      {0, 0, 1},
    /* <= */      {1, 1, 0},
    /* >= */      {0, 1, 1},
    /* != */      {1, 0, 1}
};

static uint8_t compare_u16(uint16_t a, uint16_t b)
{
    if (a < b)
        return REMASTER_EMERALD_COMPARE_LESS;
    if (a == b)
        return REMASTER_EMERALD_COMPARE_EQUAL;
    return REMASTER_EMERALD_COMPARE_GREATER;
}

static int condition_matches(
    uint8_t condition,
    uint8_t comparison_result)
{
    if (condition >= 6u || comparison_result >= 3u)
        return 0;

    return kConditionTable[condition][comparison_result] != 0u;
}

int remaster_emerald_script_var_get(
    const RemasterEmeraldScriptVm *vm,
    uint16_t id,
    uint16_t *out_value)
{
    if (vm == 0 || out_value == 0)
        return 0;

    if (id >= VARS_START && id <= VARS_END) {
        if (vm->save == 0)
            return 0;
        return remaster_emerald_var_get(vm->save, id, out_value);
    }

    if (id >= SPECIAL_VARS_START && id <= SPECIAL_VARS_END) {
        *out_value = vm->special_vars[id - SPECIAL_VARS_START];
        return 1;
    }

    return 0;
}

int remaster_emerald_script_var_set(
    RemasterEmeraldScriptVm *vm,
    uint16_t id,
    uint16_t value)
{
    if (vm == 0)
        return 0;

    if (id >= VARS_START && id <= VARS_END) {
        if (vm->save == 0)
            return 0;
        return remaster_emerald_var_set(vm->save, id, value);
    }

    if (id >= SPECIAL_VARS_START && id <= SPECIAL_VARS_END) {
        vm->special_vars[id - SPECIAL_VARS_START] = value;
        return 1;
    }

    return 0;
}

uint16_t remaster_emerald_script_value_or_var(
    const RemasterEmeraldScriptVm *vm,
    uint16_t operand,
    int *out_valid)
{
    uint16_t value = 0;

    if (operand < VARS_START) {
        if (out_valid != 0)
            *out_valid = 1;
        return operand;
    }

    if (remaster_emerald_script_var_get(vm, operand, &value)) {
        if (out_valid != 0)
            *out_valid = 1;
        return value;
    }

    if (out_valid != 0)
        *out_valid = 0;
    return 0;
}

int remaster_emerald_script_flag_get(
    const RemasterEmeraldScriptVm *vm,
    uint16_t id,
    int *out_value)
{
    if (vm == 0 || out_value == 0)
        return 0;

    if (id == 0u) {
        *out_value = 0;
        return 1;
    }

    if (id < SPECIAL_FLAGS_START) {
        if (vm->save == 0)
            return 0;
        return remaster_emerald_flag_get(vm->save, id, out_value);
    }

    if (id <= SPECIAL_FLAGS_END) {
        uint16_t index = (uint16_t)(id - SPECIAL_FLAGS_START);
        *out_value =
            (vm->special_flags[index / 8u] >> (index & 7u)) & 1u;
        return 1;
    }

    return 0;
}

int remaster_emerald_script_flag_set(
    RemasterEmeraldScriptVm *vm,
    uint16_t id,
    int value)
{
    if (vm == 0)
        return 0;

    /* Vanilla FlagSet/FlagClear(0) are no-ops. */
    if (id == 0u)
        return 1;

    if (id < SPECIAL_FLAGS_START) {
        if (vm->save == 0)
            return 0;
        return remaster_emerald_flag_set(vm->save, id, value);
    }

    if (id <= SPECIAL_FLAGS_END) {
        uint16_t index = (uint16_t)(id - SPECIAL_FLAGS_START);
        uint8_t mask = (uint8_t)(1u << (index & 7u));

        if (value)
            vm->special_flags[index / 8u] |= mask;
        else
            vm->special_flags[index / 8u] &= (uint8_t)~mask;

        return 1;
    }

    return 0;
}

static const char *current_script_id(
    const RemasterEmeraldScriptVm *vm)
{
    if (vm == 0
        || vm->registry == 0
        || vm->registry->programs == 0
        || vm->program_index >= vm->registry->program_count)
    {
        return 0;
    }

    return vm->registry->programs[vm->program_index].script_id;
}

static RemasterEmeraldScriptStatus script_fail(
    RemasterEmeraldScriptVm *vm,
    RemasterEmeraldScriptErrorCode code,
    uint32_t pc,
    RemasterEmeraldScriptOpcode opcode)
{
    if (vm == 0)
        return REMASTER_EMERALD_SCRIPT_ERROR;

    vm->error.code = code;
    vm->error.script_id = current_script_id(vm);
    vm->error.program_index = vm->program_index;
    vm->error.pc = pc;
    vm->error.opcode = opcode;
    vm->status = REMASTER_EMERALD_SCRIPT_ERROR;
    return vm->status;
}

static int select_registry_program(
    RemasterEmeraldScriptVm *vm,
    uint32_t program_index)
{
    const RemasterEmeraldScriptProgram *program;

    if (vm == 0 || vm->registry == 0 || vm->registry->programs == 0)
        return 0;
    if (program_index >= vm->registry->program_count)
        return 0;

    program = &vm->registry->programs[program_index];
    if (program->instructions == 0 || program->instruction_count == 0u)
        return 0;

    vm->program_index = program_index;
    vm->program = program->instructions;
    vm->program_count = program->instruction_count;
    return 1;
}

static int resolve_target(
    RemasterEmeraldScriptVm *vm,
    const RemasterEmeraldScriptInstruction *ins,
    uint32_t instruction_pc,
    uint32_t *out_program_index)
{
    uint32_t target_program;

    target_program = ins->target_program_valid
        ? ins->target_program
        : vm->program_index;

    if (vm->registry == 0) {
        if (target_program != 0u) {
            script_fail(
                vm,
                REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM,
                instruction_pc,
                ins->opcode);
            return 0;
        }
        if (ins->target >= vm->program_count) {
            script_fail(
                vm,
                REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PC,
                instruction_pc,
                ins->opcode);
            return 0;
        }
    } else {
        const RemasterEmeraldScriptProgram *program;

        if (vm->registry->programs == 0
            || target_program >= vm->registry->program_count)
        {
            script_fail(
                vm,
                REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM,
                instruction_pc,
                ins->opcode);
            return 0;
        }

        program = &vm->registry->programs[target_program];
        if (program->instructions == 0
            || ins->target >= program->instruction_count)
        {
            script_fail(
                vm,
                REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PC,
                instruction_pc,
                ins->opcode);
            return 0;
        }
    }

    *out_program_index = target_program;
    return 1;
}

static void apply_target(
    RemasterEmeraldScriptVm *vm,
    uint32_t program_index,
    uint32_t pc)
{
    if (vm->registry != 0)
        (void)select_registry_program(vm, program_index);

    vm->pc = pc;
}

void remaster_emerald_script_init(
    RemasterEmeraldScriptVm *vm,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptInstruction *program,
    size_t program_count,
    uint32_t entry_pc)
{
    if (vm == 0)
        return;

    memset(vm, 0, sizeof(*vm));

    vm->save = save;
    vm->program_index = 0;
    vm->program = program;
    vm->program_count = program_count;
    vm->pc = entry_pc;

    if (program == 0 || program_count == 0u) {
        (void)script_fail(
            vm,
            REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM,
            entry_pc,
            REMASTER_EMERALD_SCRIPT_NOP);
    } else if (entry_pc >= program_count) {
        (void)script_fail(
            vm,
            REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PC,
            entry_pc,
            REMASTER_EMERALD_SCRIPT_NOP);
    } else {
        vm->status = REMASTER_EMERALD_SCRIPT_RUNNING;
    }
}

void remaster_emerald_script_init_program(
    RemasterEmeraldScriptVm *vm,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRegistry *registry,
    uint32_t program_index,
    uint32_t entry_pc)
{
    if (vm == 0)
        return;

    memset(vm, 0, sizeof(*vm));
    vm->save = save;
    vm->registry = registry;
    vm->program_index = program_index;

    if (!select_registry_program(vm, program_index)) {
        vm->program_index = program_index;
        (void)script_fail(
            vm,
            REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM,
            entry_pc,
            REMASTER_EMERALD_SCRIPT_NOP);
        return;
    }

    vm->pc = entry_pc;
    if (entry_pc >= vm->program_count) {
        (void)script_fail(
            vm,
            REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PC,
            entry_pc,
            REMASTER_EMERALD_SCRIPT_NOP);
        return;
    }

    vm->status = REMASTER_EMERALD_SCRIPT_RUNNING;
}

int remaster_emerald_script_error_get(
    const RemasterEmeraldScriptVm *vm,
    RemasterEmeraldScriptError *out_error)
{
    if (vm == 0
        || out_error == 0
        || vm->error.code == REMASTER_EMERALD_SCRIPT_ERROR_NONE)
    {
        return 0;
    }

    *out_error = vm->error;
    return 1;
}

static int set_saved_weather(
    RemasterEmeraldScriptVm *vm,
    uint16_t operand)
{
    RemasterEmeraldOverworldState state;
    int valid = 0;
    uint16_t value;

    if (vm->save == 0)
        return 0;

    value = remaster_emerald_script_value_or_var(
        vm,
        operand,
        &valid);

    if (!valid || value > UINT8_MAX)
        return 0;

    if (!remaster_emerald_overworld_get(vm->save, &state))
        return 0;

    state.weather = (uint8_t)value;
    return remaster_emerald_overworld_set(vm->save, &state);
}

static int set_map_layout(
    RemasterEmeraldScriptVm *vm,
    uint16_t operand)
{
    RemasterEmeraldOverworldState state;
    int valid = 0;
    uint16_t value;

    if (vm->save == 0)
        return 0;

    value = remaster_emerald_script_value_or_var(
        vm,
        operand,
        &valid);

    if (!valid)
        return 0;

    if (!remaster_emerald_overworld_get(vm->save, &state))
        return 0;

    state.map_layout_id = value;
    return remaster_emerald_overworld_set(vm->save, &state);
}


static int add_money(
    RemasterEmeraldScriptVm *vm,
    uint32_t amount)
{
    RemasterEmeraldOverworldState state;
    uint32_t next;

    if (vm == 0 || vm->save == 0)
        return 0;
    if (!remaster_emerald_overworld_get(vm->save, &state))
        return 0;

    next = state.money + amount;
    if (next > MAX_MONEY || next < state.money)
        next = MAX_MONEY;

    state.money = next;
    return remaster_emerald_overworld_set(vm->save, &state);
}

static int remove_money(
    RemasterEmeraldScriptVm *vm,
    uint32_t amount)
{
    RemasterEmeraldOverworldState state;

    if (vm == 0 || vm->save == 0)
        return 0;
    if (!remaster_emerald_overworld_get(vm->save, &state))
        return 0;

    state.money = state.money < amount ? 0u : state.money - amount;
    return remaster_emerald_overworld_set(vm->save, &state);
}

static int check_money(
    RemasterEmeraldScriptVm *vm,
    uint32_t amount)
{
    RemasterEmeraldOverworldState state;

    if (vm == 0 || vm->save == 0)
        return 0;
    if (!remaster_emerald_overworld_get(vm->save, &state))
        return 0;

    return remaster_emerald_script_var_set(
        vm,
        VAR_RESULT,
        (uint16_t)(state.money >= amount ? 1u : 0u));
}

RemasterEmeraldScriptStatus remaster_emerald_script_run(
    RemasterEmeraldScriptVm *vm,
    size_t max_steps)
{
    size_t steps = 0;

    if (vm == 0)
        return REMASTER_EMERALD_SCRIPT_ERROR;

    if (vm->status == REMASTER_EMERALD_SCRIPT_ERROR
        || vm->status == REMASTER_EMERALD_SCRIPT_HALTED)
    {
        return vm->status;
    }

    vm->status = REMASTER_EMERALD_SCRIPT_RUNNING;

    while (steps < max_steps) {
        const RemasterEmeraldScriptInstruction *ins;
        uint32_t instruction_pc;
        uint32_t target_program;
        uint16_t lhs;
        uint16_t rhs;
        int value;
        int valid;

        if (vm->program == 0 || vm->pc >= vm->program_count) {
            vm->status = REMASTER_EMERALD_SCRIPT_HALTED;
            return vm->status;
        }

        instruction_pc = vm->pc;
        ins = &vm->program[vm->pc++];
        ++steps;

        switch (ins->opcode) {
        case REMASTER_EMERALD_SCRIPT_NOP:
            break;

        case REMASTER_EMERALD_SCRIPT_END:
            vm->status = REMASTER_EMERALD_SCRIPT_HALTED;
            return vm->status;

        case REMASTER_EMERALD_SCRIPT_RETURN:
            if (vm->stack_depth == 0u) {
                vm->status = REMASTER_EMERALD_SCRIPT_HALTED;
                return vm->status;
            } else {
                RemasterEmeraldScriptCallFrame frame =
                    vm->stack[--vm->stack_depth];

                if (vm->registry != 0) {
                    if (!select_registry_program(vm, frame.program_index)) {
                        return script_fail(
                            vm,
                            REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM,
                            instruction_pc,
                            ins->opcode);
                    }
                } else if (frame.program_index != 0u) {
                    return script_fail(
                        vm,
                        REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM,
                        instruction_pc,
                        ins->opcode);
                }

                if (frame.return_pc > vm->program_count) {
                    return script_fail(
                        vm,
                        REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PC,
                        instruction_pc,
                        ins->opcode);
                }

                vm->pc = frame.return_pc;
            }
            break;

        case REMASTER_EMERALD_SCRIPT_GOTO:
            if (!resolve_target(
                    vm,
                    ins,
                    instruction_pc,
                    &target_program))
            {
                return vm->status;
            }
            apply_target(vm, target_program, ins->target);
            break;

        case REMASTER_EMERALD_SCRIPT_CALL:
            if (!resolve_target(
                    vm,
                    ins,
                    instruction_pc,
                    &target_program))
            {
                return vm->status;
            }
            if (vm->stack_depth >= REMASTER_EMERALD_SCRIPT_STACK_DEPTH) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STACK_OVERFLOW,
                    instruction_pc,
                    ins->opcode);
            }
            vm->stack[vm->stack_depth].program_index = vm->program_index;
            vm->stack[vm->stack_depth].return_pc = vm->pc;
            ++vm->stack_depth;
            apply_target(vm, target_program, ins->target);
            break;

        case REMASTER_EMERALD_SCRIPT_GOTO_IF:
            if (condition_matches(ins->condition, vm->comparison_result)) {
                if (!resolve_target(
                        vm,
                        ins,
                        instruction_pc,
                        &target_program))
                {
                    return vm->status;
                }
                apply_target(vm, target_program, ins->target);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_CALL_IF:
            if (condition_matches(ins->condition, vm->comparison_result)) {
                if (!resolve_target(
                        vm,
                        ins,
                        instruction_pc,
                        &target_program))
                {
                    return vm->status;
                }
                if (vm->stack_depth >= REMASTER_EMERALD_SCRIPT_STACK_DEPTH) {
                    return script_fail(
                        vm,
                        REMASTER_EMERALD_SCRIPT_ERROR_STACK_OVERFLOW,
                        instruction_pc,
                        ins->opcode);
                }
                vm->stack[vm->stack_depth].program_index = vm->program_index;
                vm->stack[vm->stack_depth].return_pc = vm->pc;
                ++vm->stack_depth;
                apply_target(vm, target_program, ins->target);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_VAR:
            if (!remaster_emerald_script_var_set(vm, ins->a, ins->b)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_COPY_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->b, &rhs)
                || !remaster_emerald_script_var_set(vm, ins->a, rhs))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_OR_COPY_VAR:
            rhs = remaster_emerald_script_value_or_var(vm, ins->b, &valid);
            if (!valid
                || !remaster_emerald_script_var_set(vm, ins->a, rhs))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VALUE:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            vm->comparison_result = compare_u16(lhs, ins->b);
            break;

        case REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs)
                || !remaster_emerald_script_var_get(vm, ins->b, &rhs))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            vm->comparison_result = compare_u16(lhs, rhs);
            break;

        case REMASTER_EMERALD_SCRIPT_ADD_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs)
                || !remaster_emerald_script_var_set(
                    vm,
                    ins->a,
                    (uint16_t)(lhs + ins->b)))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SUB_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }

            rhs = remaster_emerald_script_value_or_var(vm, ins->b, &valid);
            if (!valid
                || !remaster_emerald_script_var_set(
                    vm,
                    ins->a,
                    (uint16_t)(lhs - rhs)))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_FLAG:
            if (!remaster_emerald_script_flag_set(vm, ins->a, 1)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_CLEAR_FLAG:
            if (!remaster_emerald_script_flag_set(vm, ins->a, 0)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_CHECK_FLAG:
            if (!remaster_emerald_script_flag_get(vm, ins->a, &value)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            vm->comparison_result = (uint8_t)(value ? 1u : 0u);
            break;

        case REMASTER_EMERALD_SCRIPT_CHECK_PLAYER_GENDER:
        {
            uint8_t gender = 0;

            if (!remaster_emerald_player_gender_get(vm->save, &gender)
                || !remaster_emerald_script_var_set(
                    vm,
                    VAR_RESULT,
                    (uint16_t)gender))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;
        }

        case REMASTER_EMERALD_SCRIPT_SET_WEATHER:
            if (!set_saved_weather(vm, ins->a)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_MAP_LAYOUT:
            if (!set_map_layout(vm, ins->a)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_WAIT_STATE:
            vm->status = REMASTER_EMERALD_SCRIPT_YIELDED;
            return vm->status;

        case REMASTER_EMERALD_SCRIPT_SET_OBJECT_XY_PERM:
            lhs = remaster_emerald_script_value_or_var(vm, ins->a, &valid);
            if (!valid
                || lhs > 0xffu
                || !remaster_emerald_object_template_set_coords(
                    vm->save,
                    (uint8_t)lhs,
                    ins->x,
                    ins->y))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_OBJECT_MOVEMENT_TYPE:
            lhs = remaster_emerald_script_value_or_var(vm, ins->a, &valid);
            if (!valid
                || lhs > 0xffu
                || !remaster_emerald_object_template_set_movement_type(
                    vm->save,
                    (uint8_t)lhs,
                    (uint8_t)ins->b))
            {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_ADD_MONEY:
            if (ins->b == 0u && !add_money(vm, ins->value_u32)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_REMOVE_MONEY:
            if (ins->b == 0u && !remove_money(vm, ins->value_u32)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_CHECK_MONEY:
            if (ins->b == 0u && !check_money(vm, ins->value_u32)) {
                return script_fail(
                    vm,
                    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
                    instruction_pc,
                    ins->opcode);
            }
            break;

        case REMASTER_EMERALD_SCRIPT_ADD_OBJECT:
        case REMASTER_EMERALD_SCRIPT_REMOVE_OBJECT:
        case REMASTER_EMERALD_SCRIPT_SHOW_OBJECT:
        case REMASTER_EMERALD_SCRIPT_HIDE_OBJECT:
        case REMASTER_EMERALD_SCRIPT_SET_OBJECT_XY:
        case REMASTER_EMERALD_SCRIPT_TURN_OBJECT:
        case REMASTER_EMERALD_SCRIPT_FACE_PLAYER:
        case REMASTER_EMERALD_SCRIPT_APPLY_MOVEMENT:
        case REMASTER_EMERALD_SCRIPT_WAIT_MOVEMENT:
        case REMASTER_EMERALD_SCRIPT_MESSAGE:
        case REMASTER_EMERALD_SCRIPT_CLOSE_MESSAGE:
        case REMASTER_EMERALD_SCRIPT_WAIT_MESSAGE:
        case REMASTER_EMERALD_SCRIPT_CHOICE:
        case REMASTER_EMERALD_SCRIPT_DELAY:
        case REMASTER_EMERALD_SCRIPT_PLAY_SOUND:
        case REMASTER_EMERALD_SCRIPT_WAIT_SOUND:
        case REMASTER_EMERALD_SCRIPT_PLAY_FANFARE:
        case REMASTER_EMERALD_SCRIPT_WAIT_FANFARE:
        case REMASTER_EMERALD_SCRIPT_PLAY_BGM:
        case REMASTER_EMERALD_SCRIPT_FADE:
        case REMASTER_EMERALD_SCRIPT_OPEN_DOOR:
        case REMASTER_EMERALD_SCRIPT_CLOSE_DOOR:
        case REMASTER_EMERALD_SCRIPT_WAIT_DOOR:
        case REMASTER_EMERALD_SCRIPT_WARP:
        case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_ADD:
        case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_REMOVE:
        case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_CHECK:
        case REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_SPACE:
        case REMASTER_EMERALD_SCRIPT_DOMAIN_GIVE_MON:
        case REMASTER_EMERALD_SCRIPT_DOMAIN_HEAL_PARTY:
        case REMASTER_EMERALD_SCRIPT_DOMAIN_PARTY_SIZE:
        case REMASTER_EMERALD_SCRIPT_SPECIAL:
        case REMASTER_EMERALD_SCRIPT_SPECIAL_VAR:
            vm->status = REMASTER_EMERALD_SCRIPT_YIELDED;
            return vm->status;

        default:
            return script_fail(
                vm,
                REMASTER_EMERALD_SCRIPT_ERROR_INVALID_OPCODE,
                instruction_pc,
                ins->opcode);
        }
    }

    vm->status = REMASTER_EMERALD_SCRIPT_STEP_LIMIT;
    return vm->status;
}

