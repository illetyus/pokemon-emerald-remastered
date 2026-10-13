#ifndef REMASTER_EMERALD_NEW_GAME_H
#define REMASTER_EMERALD_NEW_GAME_H
#include "remaster/emerald_save.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterEmeraldNewGameResult {
    REMASTER_EMERALD_NEW_GAME_PREPARED = 0,
    REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT,
    REMASTER_EMERALD_NEW_GAME_UNSUPPORTED_FORMAT,
    REMASTER_EMERALD_NEW_GAME_INVALID_NAME,
    REMASTER_EMERALD_NEW_GAME_INVALID_AVATAR,
    REMASTER_EMERALD_NEW_GAME_INITIALIZED,
    REMASTER_EMERALD_NEW_GAME_NOT_EMPTY,
    REMASTER_EMERALD_NEW_GAME_INVALID_POLICY,
    REMASTER_EMERALD_NEW_GAME_INVALID_CLOCK
} RemasterEmeraldNewGameResult;
typedef enum RemasterEmeraldNewGameStage {
    REMASTER_EMERALD_NEW_GAME_DRAFT = 0
} RemasterEmeraldNewGameStage;
typedef struct RemasterEmeraldNewGameRequest {
    RemasterEmeraldSaveFormat format;
    uint32_t rng_state; /* explicit Random state at NewGameInitData entry */
    uint16_t trainer_id_lower; /* captured source timer word; zero is valid */
    uint8_t avatar; /* source MALE=0 / FEMALE=1 */
    uint8_t player_name[8]; /* source keyboard bytes, 1..7 glyphs and EOS */
} RemasterEmeraldNewGameRequest;

/* Preparation is deliberately not RemasterEmeraldSave: it cannot be passed to
 * a codec/store or exposed as a usable session. The remaining NewGameInitData
 * domains and RTC/intro host must be completed before any save is published. */
typedef struct RemasterEmeraldNewGameDraft {
    RemasterEmeraldNewGameStage stage;
    uint8_t save_block2_prefix[0xB0];
    uint32_t rng_after_trainer_id;
    uint16_t pending_source_calls;
    uint16_t map_layout_id;
    int16_t player_x, player_y;
    int16_t warp_x, warp_y;
    int8_t map_group, map_num, warp_id;
} RemasterEmeraldNewGameDraft;

/* Bounded UTF8 -> exact pinned source naming keyboard; no control sequences,
 * truncation, alternate language/font assumptions, I/O or output on failure. */
RemasterEmeraldNewGameResult remaster_emerald_new_game_encode_name(
    const char *utf8, size_t length, uint8_t out_name[8]);
RemasterEmeraldNewGameResult remaster_emerald_new_game_prepare(
    const RemasterEmeraldNewGameRequest *request, RemasterEmeraldNewGameDraft *out_draft);

typedef enum RemasterEmeraldNewGameRankingPolicy {
    REMASTER_EMERALD_NEW_GAME_RANKING_POLICY_UNSPECIFIED = 0,
    /* Pinned source's explicit UBFIX branch; never dereference GBA address zero. */
    REMASTER_EMERALD_NEW_GAME_SOURCE_UBFIX_ZERO_ID = 1
} RemasterEmeraldNewGameRankingPolicy;
typedef struct RemasterEmeraldNewGameInitRequest {
    RemasterEmeraldNewGameRequest identity;
    RemasterEmeraldSaveStatus observed_save_status;
    RemasterEmeraldNewGameRankingPolicy ranking_id_policy;
    /* Explicit source clock observation/replay input after the host's RTC reset.
     * Hardware reset does not authorize changing the Windows system clock. */
    RemasterEmeraldTime rtc_after_reset;
} RemasterEmeraldNewGameInitRequest;
enum {
    REMASTER_EMERALD_NEW_GAME_HOST_RTC_RESET = 1u,
    REMASTER_EMERALD_NEW_GAME_HOST_GLOBAL_RESET = 2u,
    REMASTER_EMERALD_NEW_GAME_HOST_TRUCK_INTRO = 4u
};
typedef struct RemasterEmeraldNewGameRuntime {
    uint32_t rng_state, rng_calls, match_call_minutes, host_actions;
} RemasterEmeraldNewGameRuntime;

/* Complete persistent initial state, never direct filesystem I/O or Playing.
 * Workspace is caller-owned scratch and must not overlap inputs/outputs.
 * Validation failures preserve the entire previous save/runtime. The session
 * host must execute host_actions and validate map readiness before Playing.
 * The initial EMPTY checkpoint is stored only after independent MISSING proof. */
RemasterEmeraldNewGameResult remaster_emerald_new_game_initialize(
    const RemasterEmeraldNewGameInitRequest *request,
    RemasterEmeraldSave *workspace, RemasterEmeraldSave *out_save,
    RemasterEmeraldNewGameRuntime *out_runtime);

#ifdef __cplusplus
}
#endif
#endif
