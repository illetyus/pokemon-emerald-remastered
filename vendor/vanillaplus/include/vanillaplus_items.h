#ifndef GUARD_VANILLAPLUS_ITEMS_H
#define GUARD_VANILLAPLUS_ITEMS_H

#include "event_object_lock.h"

#define VANILLAPLUS_QUICK_ITEM_MAX 4

enum VanillaPlusItemSortMode
{
    VP_ITEM_SORT_NONE,
    VP_ITEM_SORT_NAME,
    VP_ITEM_SORT_TYPE,
    VP_ITEM_SORT_QUANTITY,
    VP_ITEM_SORT_VALUE,
    VP_ITEM_SORT_COUNT,
};

bool8 VanillaPlusSortBagPocket(u8 pocketId, enum VanillaPlusItemSortMode mode);
void VanillaPlusSortPCItems(enum VanillaPlusItemSortMode mode);
void VanillaPlusMaybeAutoSortBagPocket(u8 pocketId);

enum VanillaPlusItemSortMode VanillaPlusGetBagSortMode(u8 pocketId);
void VanillaPlusSetBagSortMode(u8 pocketId, enum VanillaPlusItemSortMode mode);
bool8 VanillaPlusIsAutoSortEnabled(u8 pocketId);
void VanillaPlusSetAutoSortEnabled(u8 pocketId, bool8 enabled);

bool8 VanillaPlusRegisterQuickItem(u16 itemId);
bool8 VanillaPlusUnregisterQuickItem(u16 itemId);
u16 VanillaPlusGetQuickItem(u8 slot);
u8 VanillaPlusGetQuickItemCount(void);
void VanillaPlusPruneQuickItems(void);
bool8 VanillaPlusIsQuickItemRegistered(u16 itemId);
void VanillaPlusSyncPrimaryRegisteredItem(void);
void VanillaPlusReplaceQuickItem(u16 oldItem, u16 newItem);

#endif // GUARD_VANILLAPLUS_ITEMS_H
