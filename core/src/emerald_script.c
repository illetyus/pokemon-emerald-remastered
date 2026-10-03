#include "remaster/emerald_script.h"
#include "remaster/emerald_state.h"

#include <string.h>

enum {
    VARS_START = 0x4000,
    VARS_END = 0x40FF,
    SPECIAL_VARS_START = 0x8000,
    SPECIAL_VARS_END = 0x8015,

    SPECIAL_FLAGS_START = 0x4000,
    SPECIAL_FLAGS_END = 0x407F
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
    vm->program = program;
    vm->program_count = program_count;
    vm->pc = entry_pc;

    if (program == 0
        || program_count == 0u
        || entry_pc >= program_count)
    {
        vm->status = REMASTER_EMERALD_SCRIPT_ERROR;
    }
    else
    {
        vm->status = REMASTER_EMERALD_SCRIPT_RUNNING;
    }
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
        uint16_t lhs;
        uint16_t rhs;
        int value;
        int valid;

        if (vm->program == 0 || vm->pc >= vm->program_count) {
            vm->status = REMASTER_EMERALD_SCRIPT_HALTED;
            return vm->status;
        }

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
            }
            vm->pc = vm->stack[--vm->stack_depth];
            break;

        case REMASTER_EMERALD_SCRIPT_GOTO:
            if (ins->target >= vm->program_count)
                goto error;
            vm->pc = ins->target;
            break;

        case REMASTER_EMERALD_SCRIPT_CALL:
            if (ins->target >= vm->program_count
                || vm->stack_depth >= REMASTER_EMERALD_SCRIPT_STACK_DEPTH - 1u)
            {
                goto error;
            }
            vm->stack[vm->stack_depth++] = vm->pc;
            vm->pc = ins->target;
            break;

        case REMASTER_EMERALD_SCRIPT_GOTO_IF:
            if (condition_matches(ins->condition, vm->comparison_result)) {
                if (ins->target >= vm->program_count)
                    goto error;
                vm->pc = ins->target;
            }
            break;

        case REMASTER_EMERALD_SCRIPT_CALL_IF:
            if (condition_matches(ins->condition, vm->comparison_result)) {
                if (ins->target >= vm->program_count
                    || vm->stack_depth >= REMASTER_EMERALD_SCRIPT_STACK_DEPTH - 1u)
                {
                    goto error;
                }
                vm->stack[vm->stack_depth++] = vm->pc;
                vm->pc = ins->target;
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_VAR:
            if (!remaster_emerald_script_var_set(vm, ins->a, ins->b))
                goto error;
            break;

        case REMASTER_EMERALD_SCRIPT_COPY_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->b, &rhs)
                || !remaster_emerald_script_var_set(vm, ins->a, rhs))
            {
                goto error;
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_OR_COPY_VAR:
            rhs = remaster_emerald_script_value_or_var(vm, ins->b, &valid);
            if (!valid
                || !remaster_emerald_script_var_set(vm, ins->a, rhs))
            {
                goto error;
            }
            break;

        case REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VALUE:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs))
                goto error;
            vm->comparison_result = compare_u16(lhs, ins->b);
            break;

        case REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs)
                || !remaster_emerald_script_var_get(vm, ins->b, &rhs))
            {
                goto error;
            }
            vm->comparison_result = compare_u16(lhs, rhs);
            break;

        case REMASTER_EMERALD_SCRIPT_ADD_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs))
                goto error;
            if (!remaster_emerald_script_var_set(
                    vm,
                    ins->a,
                    (uint16_t)(lhs + ins->b)))
            {
                goto error;
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SUB_VAR:
            if (!remaster_emerald_script_var_get(vm, ins->a, &lhs))
                goto error;

            rhs = remaster_emerald_script_value_or_var(vm, ins->b, &valid);
            if (!valid
                || !remaster_emerald_script_var_set(
                    vm,
                    ins->a,
                    (uint16_t)(lhs - rhs)))
            {
                goto error;
            }
            break;

        case REMASTER_EMERALD_SCRIPT_SET_FLAG:
            if (!remaster_emerald_script_flag_set(vm, ins->a, 1))
                goto error;
            break;

        case REMASTER_EMERALD_SCRIPT_CLEAR_FLAG:
            if (!remaster_emerald_script_flag_set(vm, ins->a, 0))
                goto error;
            break;

        case REMASTER_EMERALD_SCRIPT_CHECK_FLAG:
            if (!remaster_emerald_script_flag_get(vm, ins->a, &value))
                goto error;
            vm->comparison_result = (uint8_t)(value ? 1u : 0u);
            break;

        case REMASTER_EMERALD_SCRIPT_SET_WEATHER:
            if (!set_saved_weather(vm, ins->a))
                goto error;
            break;

        case REMASTER_EMERALD_SCRIPT_SET_MAP_LAYOUT:
            if (!set_map_layout(vm, ins->a))
                goto error;
            break;

        case REMASTER_EMERALD_SCRIPT_WAIT_STATE:
            vm->status = REMASTER_EMERALD_SCRIPT_YIELDED;
            return vm->status;

        default:
            goto error;
        }
    }

    vm->status = REMASTER_EMERALD_SCRIPT_STEP_LIMIT;
    return vm->status;

error:
    vm->status = REMASTER_EMERALD_SCRIPT_ERROR;
    return vm->status;
}
