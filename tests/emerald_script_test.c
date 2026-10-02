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
        /*  0 */ { REMASTER_EMERALD_SCRIPT_SET_VAR, 0x4020, 5, 0, 0 },
        /*  1 */ { REMASTER_EMERALD_SCRIPT_ADD_VAR, 0x4020, 3, 0, 0 },
        /*  2 */ { REMASTER_EMERALD_SCRIPT_SET_OR_COPY_VAR, 0x8000, 0x4020, 0, 0 },
        /*  3 */ { REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VALUE, 0x4020, 8, 0, 0 },
        /*  4 */ { REMASTER_EMERALD_SCRIPT_GOTO_IF, 0, 0, REMASTER_EMERALD_CONDITION_EQUAL, 7 },
        /*  5 */ { REMASTER_EMERALD_SCRIPT_SET_VAR, 0x4021, 999, 0, 0 },
        /*  6 */ { REMASTER_EMERALD_SCRIPT_END, 0, 0, 0, 0 },
        /*  7 */ { REMASTER_EMERALD_SCRIPT_CALL, 0, 0, 0, 13 },
        /*  8 */ { REMASTER_EMERALD_SCRIPT_CHECK_FLAG, 0x0100, 0, 0, 0 },
        /*  9 */ { REMASTER_EMERALD_SCRIPT_SET_WEATHER, 2, 0, 0, 0 },
        /* 10 */ { REMASTER_EMERALD_SCRIPT_SET_MAP_LAYOUT, 77, 0, 0, 0 },
        /* 11 */ { REMASTER_EMERALD_SCRIPT_WAIT_STATE, 0, 0, 0, 0 },
        /* 12 */ { REMASTER_EMERALD_SCRIPT_END, 0, 0, 0, 0 },
        /* 13 */ { REMASTER_EMERALD_SCRIPT_SET_FLAG, 0x0100, 0, 0, 0 },
        /* 14 */ { REMASTER_EMERALD_SCRIPT_SUB_VAR, 0x4020, 2, 0, 0 },
        /* 15 */ { REMASTER_EMERALD_SCRIPT_RETURN, 0, 0, 0, 0 },
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

    puts("Emerald script VM core compatibility test passed.");
    return 0;
}
