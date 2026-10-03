#include "remaster/emerald_script.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_script_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldScriptVm vm;
    RemasterEmeraldOverworldState state;
    uint16_t value = 0;
    int flag = 0;

    const RemasterEmeraldScriptInstruction program[] = {
        /*  0 */ { .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR, .a = 0x4020, .b = 5 },
        /*  1 */ { .opcode = REMASTER_EMERALD_SCRIPT_ADD_VAR, .a = 0x4020, .b = 3 },
        /*  2 */ { .opcode = REMASTER_EMERALD_SCRIPT_SET_OR_COPY_VAR, .a = 0x8000, .b = 0x4020 },
        /*  3 */ { .opcode = REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VALUE, .a = 0x4020, .b = 8 },
        /*  4 */ { .opcode = REMASTER_EMERALD_SCRIPT_GOTO_IF, .condition = REMASTER_EMERALD_CONDITION_EQUAL, .target = 7 },
        /*  5 */ { .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR, .a = 0x4021, .b = 999 },
        /*  6 */ { .opcode = REMASTER_EMERALD_SCRIPT_END },
        /*  7 */ { .opcode = REMASTER_EMERALD_SCRIPT_CALL, .target = 13 },
        /*  8 */ { .opcode = REMASTER_EMERALD_SCRIPT_CHECK_FLAG, .a = 0x0100 },
        /*  9 */ { .opcode = REMASTER_EMERALD_SCRIPT_SET_WEATHER, .a = 2 },
        /* 10 */ { .opcode = REMASTER_EMERALD_SCRIPT_SET_MAP_LAYOUT, .a = 77 },
        /* 11 */ { .opcode = REMASTER_EMERALD_SCRIPT_WAIT_STATE },
        /* 12 */ { .opcode = REMASTER_EMERALD_SCRIPT_END },
        /* 13 */ { .opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG, .a = 0x0100 },
        /* 14 */ { .opcode = REMASTER_EMERALD_SCRIPT_SUB_VAR, .a = 0x4020, .b = 2 },
        /* 15 */ { .opcode = REMASTER_EMERALD_SCRIPT_RETURN },
    };

    memset(&save, 0, sizeof(save));
    memset(&state, 0, sizeof(state));

    state.map_layout_id = 1;
    if (!check(remaster_emerald_overworld_set(&save, &state), "state seed failed"))
        return 1;

    remaster_emerald_script_init(
        &vm,
        &save,
        program,
        sizeof(program) / sizeof(program[0]),
        0);

    if (!check(
            remaster_emerald_script_run(&vm, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED,
            "script should yield at waitstate"))
        return 1;

    if (!check(
            remaster_emerald_var_get(&save, 0x4020, &value) && value == 6,
            "set/add/sub var semantics mismatch"))
        return 1;

    if (!check(
            remaster_emerald_var_get(&save, 0x4021, &value) && value == 0,
            "conditional goto did not skip branch"))
        return 1;

    if (!check(
            remaster_emerald_script_var_get(&vm, 0x8000, &value) && value == 8,
            "special var setorcopy semantics mismatch"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&save, 0x0100, &flag) && flag == 1,
            "flag command mismatch"))
        return 1;

    if (!check(vm.comparison_result == 1, "checkflag comparison result mismatch"))
        return 1;

    if (!check(remaster_emerald_overworld_get(&save, &state), "state readback failed"))
        return 1;

    if (!check(
            state.weather == 2 && state.map_layout_id == 77,
            "weather/layout script state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_script_run(&vm, 100)
                == REMASTER_EMERALD_SCRIPT_HALTED,
            "script did not resume after waitstate"))
        return 1;

    if (!check(
            remaster_emerald_script_value_or_var(&vm, 123, 0) == 123,
            "literal VarGet behavior mismatch"))
        return 1;

    if (!check(
            remaster_emerald_script_flag_set(&vm, 0x4004, 1)
            && remaster_emerald_script_flag_get(&vm, 0x4004, &flag)
            && flag == 1,
            "special flag bank mismatch"))
        return 1;


    {
        RemasterEmeraldScriptError error;
        const RemasterEmeraldScriptInstruction program_a[] = {
            {
                .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR,
                .a = 0x4022,
                .b = 1,
            },
            {
                .opcode = REMASTER_EMERALD_SCRIPT_CALL,
                .target = 0,
                .target_program = 1,
                .target_program_valid = 1,
            },
            {
                .opcode = REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VALUE,
                .a = 0x4022,
                .b = 3,
            },
            {
                .opcode = REMASTER_EMERALD_SCRIPT_CALL_IF,
                .condition = REMASTER_EMERALD_CONDITION_EQUAL,
                .target = 2,
                .target_program = 1,
                .target_program_valid = 1,
            },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptInstruction program_b[] = {
            {
                .opcode = REMASTER_EMERALD_SCRIPT_ADD_VAR,
                .a = 0x4022,
                .b = 2,
            },
            { .opcode = REMASTER_EMERALD_SCRIPT_RETURN },
            {
                .opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG,
                .a = 0x0102,
            },
            { .opcode = REMASTER_EMERALD_SCRIPT_RETURN },
        };
        const RemasterEmeraldScriptProgram programs[] = {
            {
                .script_id = "Script_A",
                .instructions = program_a,
                .instruction_count = sizeof(program_a) / sizeof(program_a[0]),
            },
            {
                .script_id = "Script_B",
                .instructions = program_b,
                .instruction_count = sizeof(program_b) / sizeof(program_b[0]),
            },
        };
        const RemasterEmeraldScriptRegistry registry = {
            programs,
            sizeof(programs) / sizeof(programs[0]),
        };

        memset(&save, 0, sizeof(save));
        remaster_emerald_script_init_program(
            &vm,
            &save,
            &registry,
            0,
            0);

        if (!check(
                remaster_emerald_script_run(&vm, 100)
                    == REMASTER_EMERALD_SCRIPT_HALTED,
                "cross-script program did not halt cleanly"))
            return 1;

        if (!check(
                remaster_emerald_var_get(&save, 0x4022, &value)
                    && value == 3,
                "cross-script call/return did not preserve state"))
            return 1;

        if (!check(
                remaster_emerald_flag_get(&save, 0x0102, &flag)
                    && flag == 1,
                "conditional cross-script call did not execute"))
            return 1;

        if (!check(
                vm.program_index == 0 && vm.stack_depth == 0,
                "cross-script return did not restore caller"))
            return 1;

        if (!check(
                !remaster_emerald_script_error_get(&vm, &error),
                "successful script unexpectedly reported an error"))
            return 1;
    }

    {
        RemasterEmeraldScriptError error;
        const RemasterEmeraldScriptInstruction bad_program[] = {
            {
                .opcode = REMASTER_EMERALD_SCRIPT_CALL,
                .target = 0,
                .target_program = 9,
                .target_program_valid = 1,
            },
        };
        const RemasterEmeraldScriptProgram programs[] = {
            {
                .script_id = "Bad_Target",
                .instructions = bad_program,
                .instruction_count = 1,
            },
        };
        const RemasterEmeraldScriptRegistry registry = { programs, 1 };

        memset(&save, 0, sizeof(save));
        remaster_emerald_script_init_program(
            &vm,
            &save,
            &registry,
            0,
            0);

        if (!check(
                remaster_emerald_script_run(&vm, 10)
                    == REMASTER_EMERALD_SCRIPT_ERROR,
                "invalid cross-script target should fail"))
            return 1;

        if (!check(
                remaster_emerald_script_error_get(&vm, &error)
                    && error.code
                        == REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM
                    && error.script_id != 0
                    && strcmp(error.script_id, "Bad_Target") == 0
                    && error.pc == 0
                    && error.opcode == REMASTER_EMERALD_SCRIPT_CALL,
                "structured invalid-target error mismatch"))
            return 1;
    }

    {
        RemasterEmeraldScriptError error;
        const RemasterEmeraldScriptInstruction recursive_program[] = {
            {
                .opcode = REMASTER_EMERALD_SCRIPT_CALL,
                .target = 0,
            },
        };
        const RemasterEmeraldScriptProgram programs[] = {
            {
                .script_id = "Recursive",
                .instructions = recursive_program,
                .instruction_count = 1,
            },
        };
        const RemasterEmeraldScriptRegistry registry = { programs, 1 };

        memset(&save, 0, sizeof(save));
        remaster_emerald_script_init_program(
            &vm,
            &save,
            &registry,
            0,
            0);

        if (!check(
                remaster_emerald_script_run(
                    &vm,
                    REMASTER_EMERALD_SCRIPT_STACK_DEPTH + 2)
                    == REMASTER_EMERALD_SCRIPT_ERROR,
                "recursive call should stop at stack depth"))
            return 1;

        if (!check(
                vm.stack_depth == REMASTER_EMERALD_SCRIPT_STACK_DEPTH,
                "stack overflow occurred at the wrong depth"))
            return 1;

        if (!check(
                remaster_emerald_script_error_get(&vm, &error)
                    && error.code
                        == REMASTER_EMERALD_SCRIPT_ERROR_STACK_OVERFLOW,
                "stack overflow error code mismatch"))
            return 1;
    }

    {
        const RemasterEmeraldScriptInstruction loop_program[] = {
            {
                .opcode = REMASTER_EMERALD_SCRIPT_SET_VAR,
                .a = 0x4023,
                .b = 7,
            },
            {
                .opcode = REMASTER_EMERALD_SCRIPT_GOTO,
                .target = 1,
            },
        };
        const RemasterEmeraldScriptProgram programs[] = {
            {
                .script_id = "Step_Limit",
                .instructions = loop_program,
                .instruction_count = 2,
            },
        };
        const RemasterEmeraldScriptRegistry registry = { programs, 1 };

        memset(&save, 0, sizeof(save));
        remaster_emerald_script_init_program(
            &vm,
            &save,
            &registry,
            0,
            0);

        if (!check(
                remaster_emerald_script_run(&vm, 2)
                    == REMASTER_EMERALD_SCRIPT_STEP_LIMIT,
                "loop should stop at the requested step budget"))
            return 1;

        if (!check(
                remaster_emerald_var_get(&save, 0x4023, &value)
                    && value == 7,
                "completed work before step limit was lost"))
            return 1;

        if (!check(
                remaster_emerald_flag_get(&save, 0x0103, &flag)
                    && flag == 0,
                "step limit corrupted unrelated save state"))
            return 1;
    }

    {
        const RemasterEmeraldScriptInstruction gender_program[] = {
            { .opcode = REMASTER_EMERALD_SCRIPT_CHECK_PLAYER_GENDER },
            { .opcode = REMASTER_EMERALD_SCRIPT_END },
        };
        const RemasterEmeraldScriptProgram gender_programs[] = {
            {
                .script_id = "Check_Player_Gender",
                .instructions = gender_program,
                .instruction_count =
                    sizeof(gender_program) / sizeof(gender_program[0]),
            },
        };
        const RemasterEmeraldScriptRegistry gender_registry = {
            gender_programs,
            1,
        };

        memset(&save, 0, sizeof(save));
        save.save_block2[0x08] = 0;
        remaster_emerald_script_init_program(
            &vm,
            &save,
            &gender_registry,
            0,
            0);

        if (!check(
                remaster_emerald_script_run(&vm, 10)
                    == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_script_var_get(&vm, 0x800D, &value)
                && value == 0,
                "checkplayergender did not report MALE"))
            return 1;

        save.save_block2[0x08] = 1;
        remaster_emerald_script_init_program(
            &vm,
            &save,
            &gender_registry,
            0,
            0);

        if (!check(
                remaster_emerald_script_run(&vm, 10)
                    == REMASTER_EMERALD_SCRIPT_HALTED
                && remaster_emerald_script_var_get(&vm, 0x800D, &value)
                && value == 1,
                "checkplayergender did not report FEMALE"))
            return 1;
    }

    puts("Emerald script VM core compatibility test passed.");
    return 0;
}
