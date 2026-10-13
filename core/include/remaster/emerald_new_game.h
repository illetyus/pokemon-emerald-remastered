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
    REMASTER_EMERALD_NEW_GAME_INVALID_AVATAR
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

#ifdef __cplusplus
}
#endif
#endif
