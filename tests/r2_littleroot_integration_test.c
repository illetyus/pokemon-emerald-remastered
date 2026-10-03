#include "remaster/emerald_save.h"
#include "remaster/emerald_script_runtime.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const RemasterEmeraldScriptRegistry gR2LittlerootRegistry;

enum {
    VAR_LITTLEROOT_TOWN_STATE = 0x4050,
    VAR_ROUTE101_STATE = 0x4060,
    VAR_BIRCH_LAB_STATE = 0x4084,
    VAR_LITTLEROOT_RIVAL_STATE = 0x408D,
    VAR_LITTLEROOT_INTRO_STATE = 0x4092,

    FLAG_SET_WALL_CLOCK = 0x0051,
    FLAG_RESCUED_BIRCH = 0x0052,
    FLAG_HIDE_MAY_BEDROOM = 0x02D2,
    FLAG_HIDE_BRENDAN_BEDROOM = 0x02F8,
    FLAG_HIDE_TRUCK_MALE = 0x02F9,
    FLAG_HIDE_TRUCK_FEMALE = 0x02FA,
    FLAG_HIDE_ROUTE101_BOY = 0x03DF,
    FLAG_SYS_POKEMON_GET = 0x0860
};

static const RemasterEmeraldSpecialBinding kSpecialBindings[] = {
    {
        "HealPlayerParty", 0,
        REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN,
        REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_HEAL_PARTY,
        0,
    },
    {
        "TurnOffTVScreen", 65,
        REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL,
        0,
        0,
    },
    {
        "StartWallClock", 157,
        REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL,
        0,
        0,
    },
    {
        "Special_ViewWallClock", 158,
        REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL,
        0,
        0,
    },
    {
        "ChooseStarter", 159,
        REMASTER_EMERALD_SCRIPT_REQUEST_STARTER_SELECTION,
        0,
        0x800D,
    },
    {
        "ChangePokemonNickname", 161,
        REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL,
        0,
        0,
    },
};

static const RemasterEmeraldSpecialRegistry kSpecialRegistry = {
    kSpecialBindings,
    sizeof(kSpecialBindings) / sizeof(kSpecialBindings[0]),
};

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "r2_littleroot_integration_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int expect_var(
    const RemasterEmeraldSave *save,
    uint16_t id,
    uint16_t expected,
    const char *message)
{
    uint16_t value = 0;
    return check(
        remaster_emerald_var_get(save, id, &value) && value == expected,
        message);
}

static int expect_flag(
    const RemasterEmeraldSave *save,
    uint16_t id,
    int expected,
    const char *message)
{
    int value = 0;
    return check(
        remaster_emerald_flag_get(save, id, &value) && value == expected,
        message);
}

static int drive_script(
    RemasterEmeraldScriptRuntime *runtime,
    const char *script_id,
    const uint16_t *choices,
    size_t choice_count)
{
    size_t choice_index = 0;
    size_t guard = 0;

    if (!check(
            remaster_emerald_script_runtime_dispatch_script_id(
                runtime,
                script_id) == REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED,
            "failed to dispatch generated script"))
        return 0;

    while (++guard < 1000u) {
        RemasterEmeraldScriptStatus status =
            remaster_emerald_script_runtime_run(runtime, 100000u);

        if (status == REMASTER_EMERALD_SCRIPT_HALTED)
            return 1;

        if (status != REMASTER_EMERALD_SCRIPT_YIELDED) {
            RemasterEmeraldScriptError error;
            memset(&error, 0, sizeof(error));
            (void)remaster_emerald_script_error_get(&runtime->vm, &error);
            fprintf(
                stderr,
                "script %s failed: status=%d error=%d pc=%u opcode=%d\n",
                script_id,
                (int)status,
                (int)error.code,
                (unsigned)error.pc,
                (int)error.opcode);
            return 0;
        }

        {
            RemasterEmeraldScriptRequest request;
            RemasterEmeraldScriptCompletion completion;

            memset(&request, 0, sizeof(request));
            memset(&completion, 0, sizeof(completion));

            if (!check(
                    remaster_emerald_script_runtime_pending_request(
                        runtime,
                        &request),
                    "yield did not expose host request"))
                return 0;

            completion.type = request.type;
            completion.sequence = request.sequence;
            completion.local_id = request.local_id;
            completion.map_id = request.map_id;
            completion.accepted = 1;

            if (request.type == REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE) {
                if (!check(
                        choice_index < choice_count,
                        "script requested an unexpected choice"))
                    return 0;
                completion.result_u16 = choices[choice_index++];
            } else if (
                request.type
                == REMASTER_EMERALD_SCRIPT_REQUEST_STARTER_SELECTION)
            {
                completion.result_u16 = 0;
            }

            if (!check(
                    remaster_emerald_script_runtime_complete(
                        runtime,
                        &completion),
                    "host completion was rejected"))
                return 0;
        }
    }

    return check(0, "script host loop guard exhausted");
}

static int run_opening_path(uint8_t gender)
{
    RemasterEmeraldSave save;
    RemasterEmeraldSave reloaded;
    RemasterEmeraldScriptRuntime runtime;
    uint8_t *image;
    const char *truck;
    const char *house_1f;
    const char *clock;
    const char *tv;
    const char *rival;
    uint16_t truck_flag;
    uint16_t bedroom_flag;
    const uint16_t lab_choices[] = {0, 1};

    memset(&save, 0, sizeof(save));
    memset(&reloaded, 0, sizeof(reloaded));
    memset(&runtime, 0, sizeof(runtime));

    /* Production SaveBlock2 playerGender offset, already pinned by R1. */
    save.save_block2[0x0008] = gender;

    if (gender == 0) {
        truck = "LittlerootTown_EventScript_StepOffTruckMale";
        house_1f =
            "LittlerootTown_BrendansHouse_1F_EventScript_EnterHouseMovingIn";
        clock =
            "LittlerootTown_BrendansHouse_2F_EventScript_WallClock";
        tv = "PlayersHouse_1F_EventScript_PetalburgGymReportMale";
        rival = "LittlerootTown_MaysHouse_2F_EventScript_MeetMay";
        truck_flag = FLAG_HIDE_TRUCK_MALE;
        bedroom_flag = FLAG_HIDE_MAY_BEDROOM;
    } else {
        truck = "LittlerootTown_EventScript_StepOffTruckFemale";
        house_1f =
            "LittlerootTown_MaysHouse_1F_EventScript_EnterHouseMovingIn";
        clock = "LittlerootTown_MaysHouse_2F_EventScript_WallClock";
        tv = "PlayersHouse_1F_EventScript_PetalburgGymReportFemale";
        rival =
            "LittlerootTown_BrendansHouse_2F_EventScript_MeetBrendan";
        truck_flag = FLAG_HIDE_TRUCK_FEMALE;
        bedroom_flag = FLAG_HIDE_BRENDAN_BEDROOM;
    }

    remaster_emerald_script_runtime_init(
        &runtime,
        &save,
        &gR2LittlerootRegistry);
    remaster_emerald_script_runtime_set_special_registry(
        &runtime,
        &kSpecialRegistry);

    if (!drive_script(&runtime, truck, 0, 0)
        || !expect_var(
            &save,
            VAR_LITTLEROOT_INTRO_STATE,
            3,
            "truck scene did not advance intro state to 3")
        || !expect_flag(
            &save,
            truck_flag,
            1,
            "truck scene did not set gender-specific hide flag"))
        return 0;

    if (!drive_script(&runtime, house_1f, 0, 0)
        || !expect_var(
            &save,
            VAR_LITTLEROOT_INTRO_STATE,
            4,
            "moving-in scene did not advance intro state to 4"))
        return 0;

    if (!drive_script(&runtime, clock, 0, 0)
        || !expect_var(
            &save,
            VAR_LITTLEROOT_INTRO_STATE,
            6,
            "wall clock scene did not advance intro state to 6")
        || !expect_flag(
            &save,
            FLAG_SET_WALL_CLOCK,
            1,
            "wall clock flag was not persisted"))
        return 0;

    if (!drive_script(&runtime, tv, 0, 0)
        || !expect_var(
            &save,
            VAR_LITTLEROOT_INTRO_STATE,
            7,
            "TV scene did not advance intro state to 7"))
        return 0;

    if (!drive_script(&runtime, rival, 0, 0)
        || !expect_var(
            &save,
            VAR_LITTLEROOT_RIVAL_STATE,
            3,
            "rival introduction did not advance rival state")
        || !expect_var(
            &save,
            VAR_LITTLEROOT_TOWN_STATE,
            1,
            "rival introduction did not advance town state"))
        return 0;

    if (!drive_script(
            &runtime,
            "Route101_EventScript_StartBirchRescue",
            0,
            0)
        || !expect_var(
            &save,
            VAR_ROUTE101_STATE,
            2,
            "Birch rescue start did not set Route101 state 2"))
        return 0;

    if (!drive_script(
            &runtime,
            "Route101_EventScript_BirchsBag",
            0,
            0)
        || !expect_flag(
            &save,
            FLAG_SYS_POKEMON_GET,
            1,
            "starter flow did not set pokemon-get flag")
        || !expect_flag(
            &save,
            FLAG_RESCUED_BIRCH,
            1,
            "starter flow did not set rescued-Birch flag")
        || !expect_flag(
            &save,
            bedroom_flag,
            1,
            "starter flow did not set gender-specific rival bedroom flag")
        || !expect_var(
            &save,
            VAR_BIRCH_LAB_STATE,
            2,
            "starter flow did not set Birch lab state 2")
        || !expect_var(
            &save,
            VAR_ROUTE101_STATE,
            3,
            "starter flow did not set Route101 state 3"))
        return 0;

    if (!drive_script(
            &runtime,
            "LittlerootTown_ProfessorBirchsLab_EventScript_GiveStarterEvent",
            lab_choices,
            sizeof(lab_choices) / sizeof(lab_choices[0]))
        || !expect_var(
            &save,
            VAR_BIRCH_LAB_STATE,
            3,
            "Birch lab starter handoff did not advance lab state to 3")
        || !expect_flag(
            &save,
            FLAG_HIDE_ROUTE101_BOY,
            0,
            "Birch lab handoff did not reveal Route101 boy"))
        return 0;

    image = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (!check(image != 0, "save round-trip allocation failed"))
        return 0;

    memset(image, 0xff, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (!check(
            remaster_emerald_save_encode_next(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &save),
            "R2 state failed R1 save encode"))
        return 0;

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &reloaded) == REMASTER_EMERALD_SAVE_OK,
            "R2 state failed R1 save decode"))
        return 0;

    free(image);

    if (!expect_var(
            &reloaded,
            VAR_LITTLEROOT_INTRO_STATE,
            7,
            "round-trip lost intro state")
        || !expect_var(
            &reloaded,
            VAR_LITTLEROOT_RIVAL_STATE,
            3,
            "round-trip lost rival state")
        || !expect_var(
            &reloaded,
            VAR_ROUTE101_STATE,
            3,
            "round-trip lost Route101 state")
        || !expect_var(
            &reloaded,
            VAR_BIRCH_LAB_STATE,
            3,
            "round-trip lost Birch lab state")
        || !expect_flag(
            &reloaded,
            FLAG_SYS_POKEMON_GET,
            1,
            "round-trip lost pokemon-get flag"))
        return 0;

    return 1;
}

int main(void)
{
    if (!run_opening_path(0))
        return 1;
    if (!run_opening_path(1))
        return 1;

    puts("R2 real Vanilla+ Littleroot/Route101 integration passed.");
    return 0;
}
