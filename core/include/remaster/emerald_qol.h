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

typedef enum RemasterEmeraldQolTransferResult {
    REMASTER_EMERALD_QOL_TRANSFER_OK = 0,
    REMASTER_EMERALD_QOL_TRANSFER_INVALID = 1,
    REMASTER_EMERALD_QOL_TRANSFER_PARTY_FULL = 2,
    REMASTER_EMERALD_QOL_TRANSFER_BOX_FULL = 3,
    REMASTER_EMERALD_QOL_TRANSFER_LAST_USABLE = 4,
    REMASTER_EMERALD_QOL_TRANSFER_MAIL = 5
} RemasterEmeraldQolTransferResult;

typedef struct RemasterEmeraldQolHmTool {
    uint16_t item_id;
    uint16_t move_id;
    uint16_t badge_flag;
} RemasterEmeraldQolHmTool;

int remaster_emerald_qol_reusable_evolution_stone(uint16_t item_id);
int remaster_emerald_qol_evolution_item_consumed(uint16_t item_id);
int remaster_emerald_qol_teach_item_consumed(uint16_t item_id);
int remaster_emerald_qol_move_replaceable(uint16_t move_id);

uint16_t remaster_emerald_qol_field_poison_hp_after_step(uint16_t hp);
uint8_t remaster_emerald_qol_flash_level_after_use(void);
uint16_t remaster_emerald_qol_fishing_response_frames(uint8_t rod);
uint8_t remaster_emerald_qol_fishing_reaction_rounds(uint8_t rod);
int remaster_emerald_qol_bike_toggle_allowed(int stationary);
int remaster_emerald_qol_running_allowed(void);

int remaster_emerald_qol_hm_tool(
    uint16_t item_id,
    RemasterEmeraldQolHmTool *out_tool);
int remaster_emerald_qol_hm_access(
    const RemasterEmeraldSave *save,
    uint16_t item_id);

int remaster_emerald_qol_box_pokemon_is_egg(
    const RemasterEmeraldBoxPokemon *pokemon);
uint16_t remaster_emerald_qol_total_evs(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_qol_hidden_power_type(
    const RemasterEmeraldBoxPokemon *pokemon);
int remaster_emerald_qol_nickname_owned_by_player(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldBoxPokemon *pokemon);

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
