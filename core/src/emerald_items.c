#include "remaster/emerald_items.h"

#include <string.h>

enum {
    SB2_ENCRYPTION_KEY = 0x00AC,
    SB1_ITEMS = 0x0560,
    SB1_KEY_ITEMS = 0x05D8,
    SB1_POKE_BALLS = 0x0650,
    SB1_TM_HM = 0x0690,
    SB1_BERRIES = 0x0790,
    ITEM_SLOT_BYTES = 4,
    MAX_ITEM_SLOT_QUANTITY = 99,
    MAX_BERRY_SLOT_QUANTITY = 999
};

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8u)
        | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
}

static void write16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

typedef struct BagView {
    size_t offset;
    size_t capacity;
    uint16_t slot_capacity;
    int allow_duplicate_slots;
} BagView;

static int bag_view(uint8_t pocket, BagView *out)
{
    if (out == 0)
        return 0;

    switch ((RemasterEmeraldBagPocket)pocket) {
    case REMASTER_EMERALD_POCKET_ITEMS:
        out->offset = SB1_ITEMS;
        out->capacity = 30;
        out->slot_capacity = MAX_ITEM_SLOT_QUANTITY;
        out->allow_duplicate_slots = 1;
        return 1;

    case REMASTER_EMERALD_POCKET_KEY_ITEMS:
        out->offset = SB1_KEY_ITEMS;
        out->capacity = 30;
        out->slot_capacity = MAX_ITEM_SLOT_QUANTITY;
        out->allow_duplicate_slots = 1;
        return 1;

    case REMASTER_EMERALD_POCKET_POKE_BALLS:
        out->offset = SB1_POKE_BALLS;
        out->capacity = 16;
        out->slot_capacity = MAX_ITEM_SLOT_QUANTITY;
        out->allow_duplicate_slots = 1;
        return 1;

    case REMASTER_EMERALD_POCKET_TM_HM:
        out->offset = SB1_TM_HM;
        out->capacity = 64;
        out->slot_capacity = MAX_ITEM_SLOT_QUANTITY;
        out->allow_duplicate_slots = 0;
        return 1;

    case REMASTER_EMERALD_POCKET_BERRIES:
        out->offset = SB1_BERRIES;
        out->capacity = 46;
        out->slot_capacity = MAX_BERRY_SLOT_QUANTITY;
        out->allow_duplicate_slots = 0;
        return 1;

    case REMASTER_EMERALD_POCKET_NONE:
    default:
        return 0;
    }
}

static uint16_t bag_key(const RemasterEmeraldSave *save)
{
    if (save == 0)
        return 0;
    return (uint16_t)read32(save->save_block2 + SB2_ENCRYPTION_KEY);
}

static uint8_t *slot_ptr(
    RemasterEmeraldSave *save,
    const BagView *view,
    size_t slot)
{
    return save->save_block1
        + view->offset
        + slot * ITEM_SLOT_BYTES;
}

static const uint8_t *slot_ptr_const(
    const RemasterEmeraldSave *save,
    const BagView *view,
    size_t slot)
{
    return save->save_block1
        + view->offset
        + slot * ITEM_SLOT_BYTES;
}

static uint16_t slot_item_id(const uint8_t *slot)
{
    return read16(slot);
}

static uint16_t slot_quantity(
    const RemasterEmeraldSave *save,
    const uint8_t *slot)
{
    if (slot_item_id(slot) == 0)
        return 0;

    return (uint16_t)(read16(slot + 2) ^ bag_key(save));
}

static void slot_set(
    RemasterEmeraldSave *save,
    uint8_t *slot,
    uint16_t item_id,
    uint16_t quantity)
{
    write16(slot, item_id);
    write16(slot + 2, (uint16_t)(quantity ^ bag_key(save)));
}

size_t remaster_emerald_bag_pocket_capacity(uint8_t pocket)
{
    BagView view;
    return bag_view(pocket, &view) ? view.capacity : 0;
}

int remaster_emerald_bag_slot_get(
    const RemasterEmeraldSave *save,
    uint8_t pocket,
    size_t slot,
    RemasterEmeraldItemSlot *out_slot)
{
    BagView view;
    const uint8_t *raw;

    if (save == 0 || out_slot == 0
        || !bag_view(pocket, &view)
        || slot >= view.capacity)
        return 0;

    raw = slot_ptr_const(save, &view, slot);
    out_slot->item_id = slot_item_id(raw);
    out_slot->quantity = slot_quantity(save, raw);
    return 1;
}

uint32_t remaster_emerald_bag_count(
    const RemasterEmeraldSave *save,
    uint16_t item_id)
{
    const RemasterEmeraldItemInfo *info =
        remaster_emerald_item_info(item_id);
    BagView view;
    uint32_t total = 0;
    size_t i;

    if (save == 0 || item_id == 0 || info == 0
        || !bag_view(info->pocket, &view))
        return 0;

    for (i = 0; i < view.capacity; ++i) {
        const uint8_t *raw = slot_ptr_const(save, &view, i);
        if (slot_item_id(raw) == item_id)
            total += slot_quantity(save, raw);
    }

    return total;
}

int remaster_emerald_bag_has_space(
    const RemasterEmeraldSave *save,
    uint16_t item_id,
    uint16_t count)
{
    const RemasterEmeraldItemInfo *info =
        remaster_emerald_item_info(item_id);
    BagView view;
    uint32_t remaining = count;
    size_t i;

    if (save == 0 || item_id == 0 || count == 0 || info == 0
        || !bag_view(info->pocket, &view))
        return 0;

    for (i = 0; i < view.capacity; ++i) {
        const uint8_t *raw = slot_ptr_const(save, &view, i);
        if (slot_item_id(raw) == item_id) {
            const uint16_t owned = slot_quantity(save, raw);
            if (owned < view.slot_capacity) {
                const uint32_t room = view.slot_capacity - owned;
                if (remaining <= room)
                    return 1;
                if (!view.allow_duplicate_slots)
                    return 0;
                remaining -= room;
            } else if (!view.allow_duplicate_slots) {
                return 0;
            }
        }
    }

    for (i = 0; i < view.capacity && remaining > 0; ++i) {
        const uint8_t *raw = slot_ptr_const(save, &view, i);
        if (slot_item_id(raw) == 0) {
            if (remaining <= view.slot_capacity)
                return 1;
            if (!view.allow_duplicate_slots)
                return 0;
            remaining -= view.slot_capacity;
        }
    }

    return remaining == 0;
}

int remaster_emerald_bag_add(
    RemasterEmeraldSave *save,
    uint16_t item_id,
    uint16_t count)
{
    const RemasterEmeraldItemInfo *info =
        remaster_emerald_item_info(item_id);
    BagView view;
    uint32_t remaining = count;
    size_t i;

    if (save == 0 || item_id == 0 || count == 0 || info == 0
        || !bag_view(info->pocket, &view)
        || !remaster_emerald_bag_has_space(save, item_id, count))
        return 0;

    for (i = 0; i < view.capacity && remaining > 0; ++i) {
        uint8_t *raw = slot_ptr(save, &view, i);
        if (slot_item_id(raw) == item_id) {
            const uint16_t owned = slot_quantity(save, raw);
            const uint16_t room = owned < view.slot_capacity
                ? (uint16_t)(view.slot_capacity - owned)
                : 0;
            const uint16_t add = remaining < room
                ? (uint16_t)remaining
                : room;

            if (add > 0) {
                slot_set(save, raw, item_id, (uint16_t)(owned + add));
                remaining -= add;
            }

            if (!view.allow_duplicate_slots && remaining > 0)
                return 0;
        }
    }

    for (i = 0; i < view.capacity && remaining > 0; ++i) {
        uint8_t *raw = slot_ptr(save, &view, i);
        if (slot_item_id(raw) == 0) {
            const uint16_t add = remaining < view.slot_capacity
                ? (uint16_t)remaining
                : view.slot_capacity;
            slot_set(save, raw, item_id, add);
            remaining -= add;

            if (!view.allow_duplicate_slots && remaining > 0)
                return 0;
        }
    }

    return remaining == 0;
}

int remaster_emerald_bag_remove(
    RemasterEmeraldSave *save,
    uint16_t item_id,
    uint16_t count)
{
    const RemasterEmeraldItemInfo *info =
        remaster_emerald_item_info(item_id);
    BagView view;
    uint32_t remaining = count;
    size_t i;

    if (save == 0 || item_id == 0 || count == 0 || info == 0
        || !bag_view(info->pocket, &view)
        || remaster_emerald_bag_count(save, item_id) < count)
        return 0;

    for (i = 0; i < view.capacity && remaining > 0; ++i) {
        uint8_t *raw = slot_ptr(save, &view, i);
        if (slot_item_id(raw) == item_id) {
            const uint16_t owned = slot_quantity(save, raw);
            const uint16_t take = remaining < owned
                ? (uint16_t)remaining
                : owned;
            const uint16_t left = (uint16_t)(owned - take);

            slot_set(save, raw, left == 0 ? 0 : item_id, left);
            remaining -= take;
        }
    }

    return remaining == 0;
}
