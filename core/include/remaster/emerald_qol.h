#ifndef REMASTER_EMERALD_QOL_H
#define REMASTER_EMERALD_QOL_H

#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_save.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterEmeraldQolBoxSortMode {
    REMASTER_EMERALD_QOL_BOX_SORT_SPECIES = 0,
    REMASTER_EMERALD_QOL_BOX_SORT_LEVEL = 1,
    REMASTER_EMERALD_QOL_BOX_SORT_TYPE = 2
} RemasterEmeraldQolBoxSortMode;

typedef enum RemasterEmeraldQolItemSortMode {
    REMASTER_EMERALD_QOL_ITEM_SORT_NONE = 0,
    REMASTER_EMERALD_QOL_ITEM_SORT_NAME = 1,
    REMASTER_EMERALD_QOL_ITEM_SORT_TYPE = 2,
    REMASTER_EMERALD_QOL_ITEM_SORT_QUANTITY = 3,
    REMASTER_EMERALD_QOL_ITEM_SORT_VALUE = 4,
    REMASTER_EMERALD_QOL_ITEM_SORT_COUNT = 5
} RemasterEmeraldQolItemSortMode;

typedef enum RemasterEmeraldQolTransferResult {
    REMASTER_EMERALD_QOL_TRANSFER_OK = 0,
    REMASTER_EMERALD_QOL_TRANSFER_INVALID = 1,
    REMASTER_EMERALD_QOL_TRANSFER_PARTY_FULL = 2,
    REMASTER_EMERALD_QOL_TRANSFER_BOX_FULL = 3,
    REMASTER_EMERALD_QOL_TRANSFER_LAST_USABLE = 4,
    REMASTER_EMERALD_QOL_TRANSFER_MAIL = 5
} RemasterEmeraldQolTransferResult;

typedef enum RemasterEmeraldQolHeldItemResult {
    REMASTER_EMERALD_QOL_HELD_ITEM_OK = 0,
    REMASTER_EMERALD_QOL_HELD_ITEM_INVALID = 1,
    REMASTER_EMERALD_QOL_HELD_ITEM_EGG = 2,
    REMASTER_EMERALD_QOL_HELD_ITEM_MAIL = 3,
    REMASTER_EMERALD_QOL_HELD_ITEM_UNHOLDABLE = 4,
    REMASTER_EMERALD_QOL_HELD_ITEM_ALREADY_HELD = 5,
    REMASTER_EMERALD_QOL_HELD_ITEM_ITEM_MISSING = 6,
    REMASTER_EMERALD_QOL_HELD_ITEM_BAG_FULL = 7,
    REMASTER_EMERALD_QOL_HELD_ITEM_NOT_HOLDING = 8
} RemasterEmeraldQolHeldItemResult;

typedef enum RemasterEmeraldQolMoveRelearnerStatus {
    REMASTER_EMERALD_QOL_MOVE_RELEARNER_OK = 0,
    REMASTER_EMERALD_QOL_MOVE_RELEARNER_INVALID = 1,
    REMASTER_EMERALD_QOL_MOVE_RELEARNER_EGG = 2,
    REMASTER_EMERALD_QOL_MOVE_RELEARNER_NO_SCALE = 3,
    REMASTER_EMERALD_QOL_MOVE_RELEARNER_NO_MOVES = 4
} RemasterEmeraldQolMoveRelearnerStatus;

typedef enum RemasterEmeraldQolHmFieldUseResult {
    REMASTER_EMERALD_QOL_HM_FIELD_USE_OK = 0,
    REMASTER_EMERALD_QOL_HM_FIELD_USE_INVALID_ITEM = 1,
    REMASTER_EMERALD_QOL_HM_FIELD_USE_NO_ACCESS = 2,
    REMASTER_EMERALD_QOL_HM_FIELD_USE_NO_ACTOR = 3
} RemasterEmeraldQolHmFieldUseResult;

typedef struct RemasterEmeraldQolHmTool {
    uint16_t item_id;
    uint16_t move_id;
    uint16_t badge_flag;
} RemasterEmeraldQolHmTool;

typedef struct RemasterEmeraldQolBikeToggleState {
    uint8_t r_pressed;
    uint8_t cycling_road;
    uint8_t player_moving;
    uint8_t on_mach_bike;
    uint8_t on_acro_bike;
    uint8_t mach_speed_standing;
    uint8_t acro_state_normal;
} RemasterEmeraldQolBikeToggleState;

int remaster_emerald_qol_reusable_evolution_stone(uint16_t item_id);
int remaster_emerald_qol_evolution_item_consumed(uint16_t item_id);
int remaster_emerald_qol_teach_item_consumed(uint16_t item_id);
int remaster_emerald_qol_move_replaceable(uint16_t move_id);

uint16_t remaster_emerald_qol_field_poison_hp_after_step(uint16_t hp);
uint8_t remaster_emerald_qol_flash_level_after_use(void);
uint16_t remaster_emerald_qol_fishing_response_frames(uint8_t rod);
uint8_t remaster_emerald_qol_fishing_required_rounds(
    uint8_t rod,
    uint16_t emerald_random_value);
uint8_t remaster_emerald_qol_fishing_optional_round_chance(
    uint8_t rod,
    uint8_t round_index);
int remaster_emerald_qol_running_requires_shoes(void);
int remaster_emerald_qol_running_environment_allows(
    int underwater,
    int metatile_disallowed);
int remaster_emerald_qol_bike_toggle_allowed(
    const RemasterEmeraldQolBikeToggleState *state);

int remaster_emerald_qol_hm_tool(
    uint16_t item_id,
    RemasterEmeraldQolHmTool *out_tool);
int remaster_emerald_qol_hm_access(
    const RemasterEmeraldSave *save,
    uint16_t item_id);
int remaster_emerald_qol_hm_field_actor(
    const RemasterEmeraldSave *save,
    uint8_t *out_party_slot);
RemasterEmeraldQolHmFieldUseResult
remaster_emerald_qol_hm_field_use(
    const RemasterEmeraldSave *save,
    uint16_t item_id,
    RemasterEmeraldQolHmTool *out_tool,
    uint8_t *out_party_slot);

int remaster_emerald_qol_box_pokemon_is_egg(
    const RemasterEmeraldBoxPokemon *pokemon);
uint16_t remaster_emerald_qol_total_evs(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_qol_hidden_power_type(
    const RemasterEmeraldBoxPokemon *pokemon);
int remaster_emerald_qol_nickname_owned_by_player(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldBoxPokemon *pokemon);

RemasterEmeraldQolMoveRelearnerStatus
remaster_emerald_qol_move_relearner_status(
    const RemasterEmeraldSave *save,
    uint8_t party_slot);
size_t remaster_emerald_qol_move_relearner_candidates(
    const RemasterEmeraldSave *save,
    uint8_t party_slot,
    uint16_t *out_moves,
    size_t capacity);
int remaster_emerald_qol_move_relearner_learn(
    RemasterEmeraldSave *save,
    uint8_t party_slot,
    uint16_t move_id,
    uint8_t replacement_slot);

int remaster_emerald_qol_storage_sort_current_box(
    RemasterEmeraldSave *save,
    RemasterEmeraldQolBoxSortMode mode);
int remaster_emerald_qol_storage_compact_current_box(
    RemasterEmeraldSave *save);

RemasterEmeraldQolTransferResult
remaster_emerald_qol_quick_deposit_current_box(
    RemasterEmeraldSave *save,
    uint8_t party_slot);
RemasterEmeraldQolTransferResult
remaster_emerald_qol_quick_withdraw_current_box(
    RemasterEmeraldSave *save,
    uint8_t box_slot);

int remaster_emerald_qol_swap_held_items(
    RemasterEmeraldBoxPokemon *a,
    RemasterEmeraldBoxPokemon *b);

int remaster_emerald_qol_bag_sort(
    RemasterEmeraldSave *save,
    uint8_t pocket,
    RemasterEmeraldQolItemSortMode mode);
RemasterEmeraldQolItemSortMode remaster_emerald_qol_bag_sort_mode(
    RemasterEmeraldSave *save,
    uint8_t pocket);
int remaster_emerald_qol_bag_set_sort_mode(
    RemasterEmeraldSave *save,
    uint8_t pocket,
    RemasterEmeraldQolItemSortMode mode);
int remaster_emerald_qol_bag_auto_sort_enabled(
    RemasterEmeraldSave *save,
    uint8_t pocket);
int remaster_emerald_qol_bag_set_auto_sort_enabled(
    RemasterEmeraldSave *save,
    uint8_t pocket,
    int enabled);
int remaster_emerald_qol_bag_auto_sort(
    RemasterEmeraldSave *save,
    uint8_t pocket);
int remaster_emerald_qol_pc_items_sort(
    RemasterEmeraldSave *save,
    RemasterEmeraldQolItemSortMode mode);

RemasterEmeraldQolHeldItemResult remaster_emerald_qol_pc_give_held_item(
    RemasterEmeraldSave *save,
    uint8_t box_slot,
    uint16_t item_id);
RemasterEmeraldQolHeldItemResult remaster_emerald_qol_pc_take_held_item(
    RemasterEmeraldSave *save,
    uint8_t box_slot,
    uint16_t *out_item_id);

uint8_t remaster_emerald_qol_quick_item_count(RemasterEmeraldSave *save);
uint16_t remaster_emerald_qol_quick_item_get(
    RemasterEmeraldSave *save,
    uint8_t slot);
int remaster_emerald_qol_quick_item_register(
    RemasterEmeraldSave *save,
    uint16_t item_id);
int remaster_emerald_qol_quick_item_unregister(
    RemasterEmeraldSave *save,
    uint16_t item_id);
void remaster_emerald_qol_quick_items_prune(RemasterEmeraldSave *save);

#ifdef __cplusplus
}
#endif

#endif
