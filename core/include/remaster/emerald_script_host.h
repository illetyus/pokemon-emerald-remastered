#ifndef REMASTER_EMERALD_SCRIPT_HOST_H
#define REMASTER_EMERALD_SCRIPT_HOST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterEmeraldScriptRequestType {
    REMASTER_EMERALD_SCRIPT_REQUEST_NONE = 0,
    REMASTER_EMERALD_SCRIPT_REQUEST_WAIT_STATE,
    REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE,
    REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE,
    REMASTER_EMERALD_SCRIPT_REQUEST_DELAY,
    REMASTER_EMERALD_SCRIPT_REQUEST_SOUND,
    REMASTER_EMERALD_SCRIPT_REQUEST_FANFARE,
    REMASTER_EMERALD_SCRIPT_REQUEST_BGM,
    REMASTER_EMERALD_SCRIPT_REQUEST_FADE,
    REMASTER_EMERALD_SCRIPT_REQUEST_DOOR,
    REMASTER_EMERALD_SCRIPT_REQUEST_MOVEMENT,
    REMASTER_EMERALD_SCRIPT_REQUEST_WARP,
    REMASTER_EMERALD_SCRIPT_REQUEST_STARTER_SELECTION,
    REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN,
    REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL
} RemasterEmeraldScriptRequestType;

typedef struct RemasterEmeraldScriptRequest {
    RemasterEmeraldScriptRequestType type;
    uint64_t sequence;
    uint32_t program_index;
    uint32_t pc;
} RemasterEmeraldScriptRequest;

typedef struct RemasterEmeraldScriptCompletion {
    RemasterEmeraldScriptRequestType type;
    uint64_t sequence;
    uint16_t result_u16;
    uint8_t accepted;
} RemasterEmeraldScriptCompletion;

#ifdef __cplusplus
}
#endif

#endif
