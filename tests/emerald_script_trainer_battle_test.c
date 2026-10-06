#include "remaster/emerald_script_runtime.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

static int complete_request(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldScriptRequest *request)
{
    RemasterEmeraldScriptCompletion completion;

    memset(&completion, 0, sizeof(completion));
    completion.type = request->type;
    completion.sequence = request->sequence;
    completion.accepted = 1;
    return remaster_emerald_script_runtime_complete(runtime, &completion);
}

int main(void)
{
    static const RemasterEmeraldScriptInstruction instructions[] = {
        {
            .opcode = REMASTER_EMERALD_SCRIPT_DOMAIN_TRAINER_BATTLE_CONFIG,
            .a = 1,
            .b = 0,
            .target = 7,
            .resource_id = "TrainerIntro"
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_DOMAIN_TRAINER_BATTLE_START
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_END
        }
    };
    static const RemasterEmeraldScriptProgram programs[] = {
        {
            "TrainerScript",
            instructions,
            sizeof(instructions) / sizeof(instructions[0])
        }
    };
    static const RemasterEmeraldScriptRegistry registry = {
        programs,
        sizeof(programs) / sizeof(programs[0])
    };

    RemasterEmeraldSave save;
    RemasterEmeraldScriptRuntime runtime;
    RemasterEmeraldScriptRequest request;
    RemasterEmeraldScriptStatus status;

    memset(&save, 0, sizeof(save));
    remaster_emerald_script_runtime_start(
        &runtime,
        &save,
        &registry,
        0,
        0);

    status = remaster_emerald_script_runtime_run(&runtime, 16);
    if (!check(
            status == REMASTER_EMERALD_SCRIPT_YIELDED
                && remaster_emerald_script_runtime_pending_request(
                    &runtime,
                    &request),
            "trainer config should yield a host request"))
        return 1;
    if (!check(
            request.type == REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN
                && request.action
                    == REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_TRAINER_BATTLE_CONFIG
                && request.value_u16 == 1
                && request.quantity == 0
                && request.local_id == 7
                && request.resource_id != 0
                && strcmp(request.resource_id, "TrainerIntro") == 0,
            "trainer config request must preserve trainer, mode and local id"))
        return 1;
    if (!check(
            complete_request(&runtime, &request),
            "trainer config completion should resume script"))
        return 1;

    status = remaster_emerald_script_runtime_run(&runtime, 16);
    if (!check(
            status == REMASTER_EMERALD_SCRIPT_YIELDED
                && remaster_emerald_script_runtime_pending_request(
                    &runtime,
                    &request),
            "trainer start should yield a host request"))
        return 1;
    if (!check(
            request.type == REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN
                && request.action
                    == REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_TRAINER_BATTLE_START,
            "trainer start request must use the trainer battle domain action"))
        return 1;
    if (!check(
            complete_request(&runtime, &request),
            "trainer start completion should resume script"))
        return 1;

    status = remaster_emerald_script_runtime_run(&runtime, 16);
    if (!check(
            status == REMASTER_EMERALD_SCRIPT_HALTED,
            "trainer script should halt after completed battle request"))
        return 1;

    puts("r13 trainer script bridge test passed");
    return 0;
}
