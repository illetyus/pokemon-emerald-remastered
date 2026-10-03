#ifndef REMASTER_EMERALD_SCRIPT_RUNTIME_H
#define REMASTER_EMERALD_SCRIPT_RUNTIME_H

#include "remaster/emerald_script.h"
#include "remaster/emerald_script_host.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RemasterEmeraldScriptRuntime {
    RemasterEmeraldScriptVm vm;

    uint64_t next_request_sequence;
    uint8_t has_pending_request;
    RemasterEmeraldScriptRequest pending_request;
} RemasterEmeraldScriptRuntime;

void remaster_emerald_script_runtime_start(
    RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRegistry *registry,
    uint32_t program_index,
    uint32_t entry_pc);

RemasterEmeraldScriptStatus remaster_emerald_script_runtime_run(
    RemasterEmeraldScriptRuntime *runtime,
    size_t max_steps);

int remaster_emerald_script_runtime_pending_request(
    const RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldScriptRequest *out_request);

int remaster_emerald_script_runtime_complete(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldScriptCompletion *completion);

#ifdef __cplusplus
}
#endif

#endif
