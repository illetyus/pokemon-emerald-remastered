#include "remaster/emerald_object_state.h"
#include "remaster/emerald_script_host.h"
#include "remaster/emerald_script_runtime.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_script_object_test: %s\n", message);
        return 0;
    }
    return 1;
}

static int complete_pending(
    RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldScriptRequest *out_request)
{
    RemasterEmeraldScriptCompletion completion;

    if (!remaster_emerald_script_runtime_pending_request(
            runtime,
            out_request))
        return 0;

    memset(&completion, 0, sizeof(completion));
    completion.type = out_request->type;
    completion.sequence = out_request->sequence;
    completion.accepted = 1;
    return remaster_emerald_script_runtime_complete(runtime, &completion);
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldScriptRuntime runtime;
    RemasterEmeraldObjectTemplate templates[2];
    RemasterEmeraldObjectTemplate observed;
    RemasterEmeraldScriptRequest request;
    RemasterEmeraldScriptCompletion wrong;

    const RemasterEmeraldScriptInstruction instructions[] = {
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SET_OBJECT_XY_PERM,
            .a = 2,
            .x = 19,
            .y = 7,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_SET_OBJECT_MOVEMENT_TYPE,
            .a = 2,
            .b = 12,
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_APPLY_MOVEMENT,
            .a = 2,
            .map_id = 0x0018,
            .resource_id = "Movement_WalkLeft",
        },
        {
            .opcode = REMASTER_EMERALD_SCRIPT_WAIT_MOVEMENT,
            .a = 2,
            .map_id = 0x0018,
        },
        { .opcode = REMASTER_EMERALD_SCRIPT_END },
    };
    const RemasterEmeraldScriptProgram programs[] = {
        {
            .script_id = "Object_Movement_Test",
            .instructions = instructions,
            .instruction_count = sizeof(instructions) / sizeof(instructions[0]),
        },
    };
    const RemasterEmeraldScriptRegistry registry = { programs, 1 };

    memset(&save, 0, sizeof(save));
    memset(templates, 0, sizeof(templates));

    templates[0].local_id = 1;
    templates[0].x = 4;
    templates[0].y = 5;
    templates[0].movement_type = 2;

    templates[1].local_id = 2;
    templates[1].x = 10;
    templates[1].y = 11;
    templates[1].movement_type = 3;

    if (!check(
            remaster_emerald_object_templates_replace(
                &save,
                templates,
                2),
            "failed to seed object templates"))
        return 1;

    remaster_emerald_script_runtime_start(
        &runtime,
        &save,
        &registry,
        0,
        0);

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED,
            "applymovement should yield a movement-start request"))
        return 1;

    if (!check(
            remaster_emerald_object_template_get(&save, 1, &observed)
                && observed.local_id == 2
                && observed.x == 19
                && observed.y == 7
                && observed.movement_type == 12,
            "permanent object mutation mismatch"))
        return 1;

    if (!check(
            remaster_emerald_object_template_get(&save, 0, &observed)
                && observed.local_id == 1
                && observed.x == 4
                && observed.y == 5
                && observed.movement_type == 2,
            "script mutated the wrong object template"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_pending_request(
                &runtime,
                &request)
                && request.type == REMASTER_EMERALD_SCRIPT_REQUEST_MOVEMENT
                && request.action
                    == REMASTER_EMERALD_SCRIPT_MOVEMENT_START
                && request.local_id == 2
                && request.map_id == 0x0018
                && request.resource_id != 0
                && strcmp(
                    request.resource_id,
                    "Movement_WalkLeft") == 0,
            "applymovement request payload mismatch"))
        return 1;

    memset(&wrong, 0, sizeof(wrong));
    wrong.type = request.type;
    wrong.sequence = request.sequence;
    wrong.accepted = 1;
    wrong.local_id = 3;
    if (!check(
            !remaster_emerald_script_runtime_complete(
                &runtime,
                &wrong),
            "movement completion for wrong local id should fail"))
        return 1;

    if (!check(
            complete_pending(&runtime, &request),
            "movement-start completion failed"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_YIELDED,
            "waitmovement should yield until movement finishes"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_pending_request(
                &runtime,
                &request)
                && request.type == REMASTER_EMERALD_SCRIPT_REQUEST_MOVEMENT
                && request.action
                    == REMASTER_EMERALD_SCRIPT_MOVEMENT_WAIT
                && request.local_id == 2
                && request.map_id == 0x0018,
            "waitmovement request payload mismatch"))
        return 1;

    if (!check(
            complete_pending(&runtime, &request),
            "movement-wait completion failed"))
        return 1;

    if (!check(
            remaster_emerald_script_runtime_run(&runtime, 100)
                == REMASTER_EMERALD_SCRIPT_HALTED,
            "object/movement script did not halt"))
        return 1;

    puts("Emerald scripted object/movement test passed.");
    return 0;
}
