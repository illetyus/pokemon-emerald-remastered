#ifndef REMASTER_EMERALD_SCRIPT_RUNTIME_H
#define REMASTER_EMERALD_SCRIPT_RUNTIME_H

#include "remaster/emerald_script.h"
#include "remaster/emerald_script_host.h"
#include "remaster/emerald_events.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum RemasterEmeraldMapScriptHook {
    REMASTER_EMERALD_MAP_SCRIPT_ON_LOAD = 0,
    REMASTER_EMERALD_MAP_SCRIPT_ON_TRANSITION,
    REMASTER_EMERALD_MAP_SCRIPT_ON_FRAME_TABLE,
    REMASTER_EMERALD_MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE
} RemasterEmeraldMapScriptHook;

typedef struct RemasterEmeraldMapScriptEntry {
    RemasterEmeraldMapScriptHook hook;
    uint16_t lhs;
    uint16_t rhs;
    const char *script_id;
} RemasterEmeraldMapScriptEntry;

typedef enum RemasterEmeraldScriptDispatchResult {
    REMASTER_EMERALD_SCRIPT_DISPATCH_NO_MATCH = 0,
    REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED = 1,
    REMASTER_EMERALD_SCRIPT_DISPATCH_BUSY = 2,
    REMASTER_EMERALD_SCRIPT_DISPATCH_UNKNOWN_SCRIPT = 3,
    REMASTER_EMERALD_SCRIPT_DISPATCH_STATE_ERROR = 4
} RemasterEmeraldScriptDispatchResult;

typedef struct RemasterEmeraldSpecialBinding {
    const char *special_id;
    uint32_t special_index;
    RemasterEmeraldScriptRequestType request_type;
    uint16_t action;
    uint16_t result_var;
} RemasterEmeraldSpecialBinding;

typedef struct RemasterEmeraldSpecialRegistry {
    const RemasterEmeraldSpecialBinding *bindings;
    size_t binding_count;
} RemasterEmeraldSpecialRegistry;

enum {
    REMASTER_EMERALD_SCRIPT_CHECKPOINT_VERSION = 1
};

typedef struct RemasterEmeraldScriptRuntime {
    RemasterEmeraldScriptVm vm;
    const RemasterEmeraldSpecialRegistry *special_registry;

    uint64_t next_request_sequence;
    uint8_t has_pending_request;
    RemasterEmeraldScriptRequest pending_request;
} RemasterEmeraldScriptRuntime;

void remaster_emerald_script_runtime_init(
    RemasterEmeraldScriptRuntime *runtime,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRegistry *registry);

void remaster_emerald_script_runtime_set_special_registry(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldSpecialRegistry *special_registry);

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

size_t remaster_emerald_script_runtime_checkpoint_size(void);

int remaster_emerald_script_runtime_checkpoint_write(
    const RemasterEmeraldScriptRuntime *runtime,
    void *dst,
    size_t dst_size);

int remaster_emerald_script_runtime_checkpoint_read(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldScriptRegistry *registry,
    RemasterEmeraldSave *save,
    const void *src,
    size_t src_size);

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_script_id(
    RemasterEmeraldScriptRuntime *runtime,
    const char *script_id);

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_run_map_hook(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldMapScriptEntry *entries,
    size_t entry_count,
    RemasterEmeraldMapScriptHook hook,
    size_t max_steps,
    RemasterEmeraldScriptStatus *out_status);

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_try_frame_table(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldMapScriptEntry *entries,
    size_t entry_count);

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_object(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldObjectEventDef *event);

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_coord(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldCoordEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation);

RemasterEmeraldScriptDispatchResult
remaster_emerald_script_runtime_dispatch_background(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldBackgroundEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    uint8_t facing_direction);

#ifdef __cplusplus
}
#endif

#endif
