#include "remaster/emerald_script_runtime.h"

#include <stdio.h>
#include <string.h>

extern const RemasterEmeraldScriptRegistry gR2LittlerootRegistry;

int main(void)
{
    const char *scripts[2] = {"LittlerootTown_EventScript_StepOffTruckMale",
                            "LittlerootTown_EventScript_StepOffTruckFemale"};
    size_t i;
    for (i = 0; i < 2; ++i) {
        RemasterEmeraldSave save, copy;
        RemasterEmeraldScriptRuntime a, b;
        RemasterEmeraldScriptRequest before, after;
        RemasterEmeraldScriptCompletion completion;
        uint8_t checkpoint[298], reopened[298];
        memset(&save, 0, sizeof(save));
        save.save_block2[8] = (uint8_t)i;
        remaster_emerald_script_runtime_init(&a, &save, &gR2LittlerootRegistry);
        if (remaster_emerald_script_runtime_dispatch_script_id(&a, scripts[i]) != REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED
            || remaster_emerald_script_runtime_run(&a, 4096) != REMASTER_EMERALD_SCRIPT_YIELDED
            || !remaster_emerald_script_runtime_pending_request(&a, &before)
            || before.type != REMASTER_EMERALD_SCRIPT_REQUEST_WORLD) {
            fputs("source-generated truck must yield actual WORLD request\n", stderr);
            return 1;
        }
        if (remaster_emerald_script_runtime_checkpoint_size() != sizeof(checkpoint)
            || !remaster_emerald_script_runtime_checkpoint_write(&a, checkpoint, sizeof(checkpoint))) {
            fprintf(stderr, "WORLD checkpoint write failed: gender=%zu type=%u\n", i, (unsigned)before.type);
            return 1;
        }
        copy = save;
        memset(&b, 0, sizeof(b));
        if (!remaster_emerald_script_runtime_checkpoint_read(&b, &gR2LittlerootRegistry, &copy, checkpoint, sizeof(checkpoint))
            || !remaster_emerald_script_runtime_pending_request(&b, &after)
            || before.type != after.type || before.action != after.action
            || before.sequence != after.sequence || before.program_index != after.program_index
            || before.pc != after.pc || before.local_id != after.local_id || before.map_id != after.map_id
            || before.value_u16 != after.value_u16 || before.quantity != after.quantity
            || before.value_u32 != after.value_u32 || before.x != after.x || before.y != after.y
            || before.resource_id != after.resource_id
            || !remaster_emerald_script_runtime_checkpoint_write(&b, reopened, sizeof(reopened))
            || memcmp(checkpoint, reopened, sizeof(checkpoint)) != 0) {
            fputs("WORLD checkpoint re-open changed pending authoritative request\n", stderr);
            return 1;
        }
        memset(&completion, 0, sizeof(completion));
        completion.type = after.type;
        completion.sequence = after.sequence + 1;
        completion.accepted = 1;
        if (remaster_emerald_script_runtime_complete(&b, &completion)) {
            fputs("reopened request accepted stale completion\n", stderr);
            return 1;
        }
        /* Checkpoint transport only: actual WORLD host execution is not acknowledged. */
        /* Version-1 pending request is the final 40 serialized bytes. */
        checkpoint[sizeof(checkpoint) - 40] = 0xff;
        checkpoint[sizeof(checkpoint) - 39] = 0xff;
        if (remaster_emerald_script_runtime_checkpoint_read(&b, &gR2LittlerootRegistry, &copy, checkpoint, sizeof(checkpoint))) {
            fputs("checkpoint accepted unsupported pending request type\n", stderr);
            return 1;
        }
    }
    puts("R19 emitted WORLD checkpoint write/read/stale-completion regression: both genders PASS");
    return 0;
}
