#ifndef REMASTER_EMERALD_ITEMS_H
#define REMASTER_EMERALD_ITEMS_H

#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterEmeraldBagPocket {
    REMASTER_EMERALD_POCKET_NONE = 0,
    REMASTER_EMERALD_POCKET_ITEMS = 1,
    REMASTER_EMERALD_POCKET_POKE_BALLS = 2,
    REMASTER_EMERALD_POCKET_TM_HM = 3,
    REMASTER_EMERALD_POCKET_BERRIES = 4,
    REMASTER_EMERALD_POCKET_KEY_ITEMS = 5
} RemasterEmeraldBagPocket;

typedef struct RemasterEmeraldItemInfo {
    uint16_t item_id;
    uint16_t price;
    uint16_t name_sort_rank;
    uint8_t pocket;
    uint8_t hold_effect;
    uint8_t hold_effect_param;
    uint8_t importance;
    uint8_t type;
    uint8_t battle_usage;
    uint8_t secondary_id;
} RemasterEmeraldItemInfo;

typedef struct RemasterEmeraldItemSlot {
    uint16_t item_id;
    uint16_t quantity;
} RemasterEmeraldItemSlot;

size_t remaster_emerald_item_info_count(void);
const RemasterEmeraldItemInfo *remaster_emerald_item_info(uint16_t item_id);

size_t remaster_emerald_bag_pocket_capacity(uint8_t pocket);

int remaster_emerald_bag_slot_get(
    const RemasterEmeraldSave *save,
    uint8_t pocket,
    size_t slot,
    RemasterEmeraldItemSlot *out_slot);

uint32_t remaster_emerald_bag_count(
    const RemasterEmeraldSave *save,
    uint16_t item_id);

int remaster_emerald_bag_has_space(
    const RemasterEmeraldSave *save,
    uint16_t item_id,
    uint16_t count);

int remaster_emerald_bag_add(
    RemasterEmeraldSave *save,
    uint16_t item_id,
    uint16_t count);

int remaster_emerald_bag_remove(
    RemasterEmeraldSave *save,
    uint16_t item_id,
    uint16_t count);

#ifdef __cplusplus
}
#endif

#endif
