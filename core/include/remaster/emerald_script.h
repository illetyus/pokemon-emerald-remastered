#ifndef REMASTER_EMERALD_SCRIPT_H
#define REMASTER_EMERALD_SCRIPT_H

#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_SCRIPT_STACK_DEPTH = 20,
    REMASTER_EMERALD_SPECIAL_VAR_COUNT = 0x16,
    REMASTER_EMERALD_SPECIAL_FLAG_BYTES = 0x10
};

typedef enum RemasterEmeraldScriptCompare {
    REMASTER_EMERALD_COMPARE_LESS = 0,
    REMASTER_EMERALD_COMPARE_EQUAL = 1,
    REMASTER_EMERALD_COMPARE_GREATER = 2
} RemasterEmeraldScriptCompare;

typedef enum RemasterEmeraldScriptCondition {
    REMASTER_EMERALD_CONDITION_LESS = 0,
    REMASTER_EMERALD_CONDITION_EQUAL = 1,
    REMASTER_EMERALD_CONDITION_GREATER = 2,
    REMASTER_EMERALD_CONDITION_LESS_EQUAL = 3,
    REMASTER_EMERALD_CONDITION_GREATER_EQUAL = 4,
    REMASTER_EMERALD_CONDITION_NOT_EQUAL = 5
} RemasterEmeraldScriptCondition;

typedef enum RemasterEmeraldScriptOpcode {
    REMASTER_EMERALD_SCRIPT_NOP = 0,
    REMASTER_EMERALD_SCRIPT_END,
    REMASTER_EMERALD_SCRIPT_RETURN,
    REMASTER_EMERALD_SCRIPT_GOTO,
    REMASTER_EMERALD_SCRIPT_CALL,
    REMASTER_EMERALD_SCRIPT_GOTO_IF,
    REMASTER_EMERALD_SCRIPT_CALL_IF,
    REMASTER_EMERALD_SCRIPT_SET_VAR,
    REMASTER_EMERALD_SCRIPT_COPY_VAR,
    REMASTER_EMERALD_SCRIPT_SET_OR_COPY_VAR,
    REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VALUE,
    REMASTER_EMERALD_SCRIPT_COMPARE_VAR_VAR,
    REMASTER_EMERALD_SCRIPT_ADD_VAR,
    REMASTER_EMERALD_SCRIPT_SUB_VAR,
    REMASTER_EMERALD_SCRIPT_SET_FLAG,
    REMASTER_EMERALD_SCRIPT_CLEAR_FLAG,
    REMASTER_EMERALD_SCRIPT_CHECK_FLAG,
    REMASTER_EMERALD_SCRIPT_SET_WEATHER,
    REMASTER_EMERALD_SCRIPT_SET_MAP_LAYOUT,
    REMASTER_EMERALD_SCRIPT_WAIT_STATE,

    REMASTER_EMERALD_SCRIPT_ADD_OBJECT,
    REMASTER_EMERALD_SCRIPT_REMOVE_OBJECT,
    REMASTER_EMERALD_SCRIPT_SHOW_OBJECT,
    REMASTER_EMERALD_SCRIPT_HIDE_OBJECT,
    REMASTER_EMERALD_SCRIPT_SET_OBJECT_XY,
    REMASTER_EMERALD_SCRIPT_SET_OBJECT_XY_PERM,
    REMASTER_EMERALD_SCRIPT_SET_OBJECT_MOVEMENT_TYPE,
    REMASTER_EMERALD_SCRIPT_TURN_OBJECT,
    REMASTER_EMERALD_SCRIPT_FACE_PLAYER,
    REMASTER_EMERALD_SCRIPT_APPLY_MOVEMENT,
    REMASTER_EMERALD_SCRIPT_WAIT_MOVEMENT,

    REMASTER_EMERALD_SCRIPT_MESSAGE,
    REMASTER_EMERALD_SCRIPT_CLOSE_MESSAGE,
    REMASTER_EMERALD_SCRIPT_WAIT_MESSAGE,
    REMASTER_EMERALD_SCRIPT_CHOICE,
    REMASTER_EMERALD_SCRIPT_DELAY,
    REMASTER_EMERALD_SCRIPT_PLAY_SOUND,
    REMASTER_EMERALD_SCRIPT_WAIT_SOUND,
    REMASTER_EMERALD_SCRIPT_PLAY_FANFARE,
    REMASTER_EMERALD_SCRIPT_WAIT_FANFARE,
    REMASTER_EMERALD_SCRIPT_PLAY_BGM,
    REMASTER_EMERALD_SCRIPT_FADE,
    REMASTER_EMERALD_SCRIPT_OPEN_DOOR,
    REMASTER_EMERALD_SCRIPT_CLOSE_DOOR,
    REMASTER_EMERALD_SCRIPT_WAIT_DOOR,
    REMASTER_EMERALD_SCRIPT_WARP,

    REMASTER_EMERALD_SCRIPT_ADD_MONEY,
    REMASTER_EMERALD_SCRIPT_REMOVE_MONEY,
    REMASTER_EMERALD_SCRIPT_CHECK_MONEY,

    REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_ADD,
    REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_REMOVE,
    REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_CHECK,
    REMASTER_EMERALD_SCRIPT_DOMAIN_ITEM_SPACE,
    REMASTER_EMERALD_SCRIPT_DOMAIN_GIVE_MON,
    REMASTER_EMERALD_SCRIPT_DOMAIN_HEAL_PARTY,
    REMASTER_EMERALD_SCRIPT_DOMAIN_PARTY_SIZE,

    REMASTER_EMERALD_SCRIPT_SPECIAL,
    REMASTER_EMERALD_SCRIPT_SPECIAL_VAR,

    /*
     * Appended for R2 acceptance without renumbering existing checkpoint-visible
     * opcode values.
     */
    REMASTER_EMERALD_SCRIPT_CHECK_PLAYER_GENDER,
    REMASTER_EMERALD_SCRIPT_LOCK,
    REMASTER_EMERALD_SCRIPT_LOCK_ALL,
    REMASTER_EMERALD_SCRIPT_RELEASE,
    REMASTER_EMERALD_SCRIPT_RELEASE_ALL,
    REMASTER_EMERALD_SCRIPT_SET_METATILE,
    REMASTER_EMERALD_SCRIPT_SAVE_BGM,
    REMASTER_EMERALD_SCRIPT_FADE_DEFAULT_BGM,
    REMASTER_EMERALD_SCRIPT_INCREMENT_GAME_STAT,
    REMASTER_EMERALD_SCRIPT_BUFFER_LEAD_MON_SPECIES_NAME,
    REMASTER_EMERALD_SCRIPT_SET_FOLLOWER,
    REMASTER_EMERALD_SCRIPT_FOLLOWER_INTO_POKEBALL,
    REMASTER_EMERALD_SCRIPT_UPDATE_FOLLOWER_POKEMON_GRAPHIC,

    /* R13 trainer-battle domain bridge; appended to preserve opcode ABI. */
    REMASTER_EMERALD_SCRIPT_DOMAIN_TRAINER_BATTLE_CONFIG,
    REMASTER_EMERALD_SCRIPT_DOMAIN_TRAINER_BATTLE_START
} RemasterEmeraldScriptOpcode;

typedef struct RemasterEmeraldScriptInstruction {
    RemasterEmeraldScriptOpcode opcode;

    /*
     * a / b retain the original 16-bit command operands.
     * For *_IF instructions condition selects Vanilla's 6x3 condition table.
     */
    uint16_t a;
    uint16_t b;
    uint8_t condition;

    /* Instruction index for goto/call targets. */
    uint32_t target;

    /*
     * Zero-initialized legacy instructions keep target_program_valid == 0,
     * which means "current program". Cross-script IR sets both fields.
     */
    uint32_t target_program;
    uint8_t target_program_valid;

    /*
     * Typed R2 resource payload. These are semantic IR values, never GBA
     * pointers. resource_id is a generated stable identity.
     */
    uint32_t map_id;
    const char *resource_id;
    int16_t x;
    int16_t y;
    uint32_t value_u32;
} RemasterEmeraldScriptInstruction;

typedef struct RemasterEmeraldScriptProgram {
    const char *script_id;
    const RemasterEmeraldScriptInstruction *instructions;
    size_t instruction_count;
} RemasterEmeraldScriptProgram;

typedef struct RemasterEmeraldScriptRegistry {
    const RemasterEmeraldScriptProgram *programs;
    size_t program_count;
} RemasterEmeraldScriptRegistry;

typedef struct RemasterEmeraldScriptCallFrame {
    uint32_t program_index;
    uint32_t return_pc;
} RemasterEmeraldScriptCallFrame;

typedef enum RemasterEmeraldScriptErrorCode {
    REMASTER_EMERALD_SCRIPT_ERROR_NONE = 0,
    REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PROGRAM,
    REMASTER_EMERALD_SCRIPT_ERROR_INVALID_PC,
    REMASTER_EMERALD_SCRIPT_ERROR_STACK_OVERFLOW,
    REMASTER_EMERALD_SCRIPT_ERROR_STATE_ACCESS,
    REMASTER_EMERALD_SCRIPT_ERROR_INVALID_OPCODE,
    REMASTER_EMERALD_SCRIPT_ERROR_UNKNOWN_SPECIAL
} RemasterEmeraldScriptErrorCode;

typedef struct RemasterEmeraldScriptError {
    RemasterEmeraldScriptErrorCode code;
    const char *script_id;
    uint32_t program_index;
    uint32_t pc;
    RemasterEmeraldScriptOpcode opcode;
} RemasterEmeraldScriptError;

typedef enum RemasterEmeraldScriptStatus {
    REMASTER_EMERALD_SCRIPT_HALTED = 0,
    REMASTER_EMERALD_SCRIPT_RUNNING = 1,
    REMASTER_EMERALD_SCRIPT_YIELDED = 2,
    REMASTER_EMERALD_SCRIPT_ERROR = 3,
    REMASTER_EMERALD_SCRIPT_STEP_LIMIT = 4
} RemasterEmeraldScriptStatus;

typedef struct RemasterEmeraldScriptVm {
    RemasterEmeraldSave *save;

    const RemasterEmeraldScriptRegistry *registry;
    uint32_t program_index;

    const RemasterEmeraldScriptInstruction *program;
    size_t program_count;
    uint32_t pc;

    RemasterEmeraldScriptCallFrame stack[REMASTER_EMERALD_SCRIPT_STACK_DEPTH];
    uint8_t stack_depth;

    uint8_t comparison_result;

    uint16_t special_vars[REMASTER_EMERALD_SPECIAL_VAR_COUNT];
    uint8_t special_flags[REMASTER_EMERALD_SPECIAL_FLAG_BYTES];

    RemasterEmeraldScriptStatus status;
    RemasterEmeraldScriptError error;
} RemasterEmeraldScriptVm;

void remaster_emerald_script_init(
    RemasterEmeraldScriptVm *vm,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptInstruction *program,
    size_t program_count,
    uint32_t entry_pc);

void remaster_emerald_script_init_program(
    RemasterEmeraldScriptVm *vm,
    RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRegistry *registry,
    uint32_t program_index,
    uint32_t entry_pc);

int remaster_emerald_script_error_get(
    const RemasterEmeraldScriptVm *vm,
    RemasterEmeraldScriptError *out_error);

RemasterEmeraldScriptStatus remaster_emerald_script_run(
    RemasterEmeraldScriptVm *vm,
    size_t max_steps);

int remaster_emerald_script_var_get(
    const RemasterEmeraldScriptVm *vm,
    uint16_t id,
    uint16_t *out_value);

int remaster_emerald_script_var_set(
    RemasterEmeraldScriptVm *vm,
    uint16_t id,
    uint16_t value);

uint16_t remaster_emerald_script_value_or_var(
    const RemasterEmeraldScriptVm *vm,
    uint16_t operand,
    int *out_valid);

int remaster_emerald_script_flag_get(
    const RemasterEmeraldScriptVm *vm,
    uint16_t id,
    int *out_value);

int remaster_emerald_script_flag_set(
    RemasterEmeraldScriptVm *vm,
    uint16_t id,
    int value);

#ifdef __cplusplus
}
#endif

#endif
