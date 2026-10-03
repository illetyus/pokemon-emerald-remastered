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
    REMASTER_EMERALD_SCRIPT_REQUEST_OBJECT,
    REMASTER_EMERALD_SCRIPT_REQUEST_MOVEMENT,
    REMASTER_EMERALD_SCRIPT_REQUEST_WARP,
    REMASTER_EMERALD_SCRIPT_REQUEST_STARTER_SELECTION,
    REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN,
    REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL
} RemasterEmeraldScriptRequestType;

typedef enum RemasterEmeraldScriptObjectAction {
    REMASTER_EMERALD_SCRIPT_OBJECT_ADD = 1,
    REMASTER_EMERALD_SCRIPT_OBJECT_REMOVE,
    REMASTER_EMERALD_SCRIPT_OBJECT_SHOW,
    REMASTER_EMERALD_SCRIPT_OBJECT_HIDE,
    REMASTER_EMERALD_SCRIPT_OBJECT_SET_XY,
    REMASTER_EMERALD_SCRIPT_OBJECT_TURN,
    REMASTER_EMERALD_SCRIPT_OBJECT_FACE_PLAYER
} RemasterEmeraldScriptObjectAction;

typedef enum RemasterEmeraldScriptMovementAction {
    REMASTER_EMERALD_SCRIPT_MOVEMENT_START = 1,
    REMASTER_EMERALD_SCRIPT_MOVEMENT_WAIT
} RemasterEmeraldScriptMovementAction;

typedef struct RemasterEmeraldScriptRequest {
    RemasterEmeraldScriptRequestType type;
    uint64_t sequence;
    uint32_t program_index;
    uint32_t pc;

    uint16_t action;
    uint16_t local_id;
    uint32_t map_id;
    const char *resource_id;
    int16_t x;
    int16_t y;
    uint16_t value_u16;
} RemasterEmeraldScriptRequest;

typedef struct RemasterEmeraldScriptCompletion {
    RemasterEmeraldScriptRequestType type;
    uint64_t sequence;
    uint16_t result_u16;
    uint16_t local_id;
    uint32_t map_id;
    uint8_t accepted;
} RemasterEmeraldScriptCompletion;

#ifdef __cplusplus
}
#endif

#endif
