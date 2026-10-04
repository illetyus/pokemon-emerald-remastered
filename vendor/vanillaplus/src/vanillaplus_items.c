#include "global.h"
#include "item.h"
#include "vanillaplus_items.h"
#include "constants/items.h"

#define VANILLAPLUS_ITEM_META_MAGIC   0x35504956 // "VIP5"
#define VANILLAPLUS_ITEM_META_VERSION 1

// Emerald's Western charmap is not alphabetically ordered for Turkish letters.
// Keep the mapping local so item-name ordering is deterministic and Turkish-aware.
#define CHAR_C_CEDILLA       0x04
#define CHAR_G_BREVE         0x01
#define CHAR_I_DOTTED        0x09
#define CHAR_O_DIAERESIS     0xF2
#define CHAR_S_CEDILLA       0x05
#define CHAR_U_DIAERESIS     0xF3
#define CHAR_c_CEDILLA       0x19
#define CHAR_g_BREVE         0x16
#define CHAR_i_DOTLESS       0x1E
#define CHAR_o_DIAERESIS     0xF5
#define CHAR_s_CEDILLA       0x1A
#define CHAR_u_DIAERESIS     0xF6
#define CHAR_A_UPPER         0xBB
#define CHAR_Z_UPPER         0xD4
#define CHAR_A_LOWER         0xD5
#define CHAR_Z_LOWER         0xEE
#define CHAR_EOS             0xFF

struct VanillaPlusItemMetadata
{
    u32 magic;
    u8 version;
    u8 autoSortMask;
    u8 bagSortMode[POCKETS_COUNT];
    u8 quickItemCount;
    u16 quickItems[VANILLAPLUS_QUICK_ITEM_MAX];
    u8 lastQuickItem;
} __attribute__((packed));

static struct VanillaPlusItemMetadata *GetItemMetadata(void);
static void ResetItemMetadata(struct VanillaPlusItemMetadata *meta);
static bool8 IsMetadataValid(const struct VanillaPlusItemMetadata *meta);
static s16 CompareItemSlots(const struct ItemSlot *a, const struct ItemSlot *b, enum VanillaPlusItemSortMode mode, bool8 encryptedQuantity);
static s16 CompareItemNames(u16 itemA, u16 itemB);
static u16 GetTurkishSortWeight(u8 ch);
static u16 GetSlotQuantity(const struct ItemSlot *slot, bool8 encryptedQuantity);
static void SortItemSlots(struct ItemSlot *slots, u16 count, enum VanillaPlusItemSortMode mode, bool8 encryptedQuantity);

static struct VanillaPlusItemMetadata *GetItemMetadata(void)
{
    struct VanillaPlusItemMetadata *meta = (struct VanillaPlusItemMetadata *)gSaveBlock1Ptr->unused_3598;

    if (!IsMetadataValid(meta))
        ResetItemMetadata(meta);
    return meta;
}

static bool8 IsMetadataValid(const struct VanillaPlusItemMetadata *meta)
{
    u8 i;

    if (meta->magic != VANILLAPLUS_ITEM_META_MAGIC || meta->version != VANILLAPLUS_ITEM_META_VERSION)
        return FALSE;
    if (meta->quickItemCount > VANILLAPLUS_QUICK_ITEM_MAX)
        return FALSE;
    for (i = 0; i < POCKETS_COUNT; i++)
    {
        if (meta->bagSortMode[i] >= VP_ITEM_SORT_COUNT)
            return FALSE;
    }
    return TRUE;
}

static void ResetItemMetadata(struct VanillaPlusItemMetadata *meta)
{
    memset(meta, 0, sizeof(*meta));
    meta->magic = VANILLAPLUS_ITEM_META_MAGIC;
    meta->version = VANILLAPLUS_ITEM_META_VERSION;

    // Migrate the single vanilla registered item lazily without changing its field.
    if (gSaveBlock1Ptr->registeredItem != ITEM_NONE)
    {
        meta->quickItems[0] = gSaveBlock1Ptr->registeredItem;
        meta->quickItemCount = 1;
    }
}

static u16 GetSlotQuantity(const struct ItemSlot *slot, bool8 encryptedQuantity)
{
    if (encryptedQuantity)
        return gSaveBlock2Ptr->encryptionKey ^ slot->quantity;
    return slot->quantity;
}

static u16 GetTurkishSortWeight(u8 ch)
{
    // Turkish alphabet plus Q/W/X for foreign item names. Upper/lowercase share weights.
    if (ch >= CHAR_A_UPPER && ch <= CHAR_Z_UPPER)
    {
        static const u8 sLatinWeights[26] =
        {
            1, 2, 3, 5, 6, 7, 8, 10, 11, 13, 14, 15, 16,
            17, 18, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32
        };
        return sLatinWeights[ch - CHAR_A_UPPER];
    }
    if (ch >= CHAR_A_LOWER && ch <= CHAR_Z_LOWER)
    {
        static const u8 sLatinWeights[26] =
        {
            1, 2, 3, 5, 6, 7, 8, 10, 12, 13, 14, 15, 16,
            17, 18, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32
        };
        return sLatinWeights[ch - CHAR_A_LOWER];
    }

    switch (ch)
    {
    case CHAR_C_CEDILLA:
    case CHAR_c_CEDILLA:
        return 4;
    case CHAR_G_BREVE:
    case CHAR_g_BREVE:
        return 9;
    case CHAR_i_DOTLESS:
        return 11;
    case CHAR_I_DOTTED:
        return 12;
    case CHAR_O_DIAERESIS:
    case CHAR_o_DIAERESIS:
        return 19;
    case CHAR_S_CEDILLA:
    case CHAR_s_CEDILLA:
        return 24;
    case CHAR_U_DIAERESIS:
    case CHAR_u_DIAERESIS:
        return 27;
    case CHAR_EOS:
        return 0;
    default:
        return 0x100 + ch;
    }
}

static s16 CompareItemNames(u16 itemA, u16 itemB)
{
    const u8 *a = ItemId_GetName(itemA);
    const u8 *b = ItemId_GetName(itemB);
    u16 wa;
    u16 wb;

    while (*a != CHAR_EOS || *b != CHAR_EOS)
    {
        if (*a == CHAR_EOS)
            return -1;
        if (*b == CHAR_EOS)
            return 1;
        wa = GetTurkishSortWeight(*a);
        wb = GetTurkishSortWeight(*b);
        if (wa < wb)
            return -1;
        if (wa > wb)
            return 1;
        a++;
        b++;
    }
    if (itemA < itemB)
        return -1;
    if (itemA > itemB)
        return 1;
    return 0;
}

static s16 CompareItemSlots(const struct ItemSlot *a, const struct ItemSlot *b, enum VanillaPlusItemSortMode mode, bool8 encryptedQuantity)
{
    u16 itemA = a->itemId;
    u16 itemB = b->itemId;
    u16 qa;
    u16 qb;
    u16 keyA;
    u16 keyB;

    if (itemA == ITEM_NONE && itemB == ITEM_NONE)
        return 0;
    if (itemA == ITEM_NONE)
        return 1;
    if (itemB == ITEM_NONE)
        return -1;

    switch (mode)
    {
    case VP_ITEM_SORT_NAME:
        return CompareItemNames(itemA, itemB);
    case VP_ITEM_SORT_TYPE:
        keyA = (ItemId_GetPocket(itemA) << 8) | ItemId_GetType(itemA);
        keyB = (ItemId_GetPocket(itemB) << 8) | ItemId_GetType(itemB);
        if (keyA < keyB)
            return -1;
        if (keyA > keyB)
            return 1;
        break;
    case VP_ITEM_SORT_QUANTITY:
        qa = GetSlotQuantity(a, encryptedQuantity);
        qb = GetSlotQuantity(b, encryptedQuantity);
        if (qa > qb)
            return -1;
        if (qa < qb)
            return 1;
        break;
    case VP_ITEM_SORT_VALUE:
        keyA = ItemId_GetPrice(itemA);
        keyB = ItemId_GetPrice(itemB);
        if (keyA > keyB)
            return -1;
        if (keyA < keyB)
            return 1;
        break;
    case VP_ITEM_SORT_NONE:
    default:
        return 0;
    }

    if (itemA < itemB)
        return -1;
    if (itemA > itemB)
        return 1;
    return 0;
}

static void SortItemSlots(struct ItemSlot *slots, u16 count, enum VanillaPlusItemSortMode mode, bool8 encryptedQuantity)
{
    u16 i;
    u16 j;
    struct ItemSlot key;

    if (mode <= VP_ITEM_SORT_NONE || mode >= VP_ITEM_SORT_COUNT)
        return;

    // Stable insertion sort: pocket capacities are small and this avoids scratch allocations.
    for (i = 1; i < count; i++)
    {
        key = slots[i];
        j = i;
        while (j > 0 && CompareItemSlots(&key, &slots[j - 1], mode, encryptedQuantity) < 0)
        {
            slots[j] = slots[j - 1];
            j--;
        }
        slots[j] = key;
    }
}

bool8 VanillaPlusSortBagPocket(u8 pocketId, enum VanillaPlusItemSortMode mode)
{
    struct BagPocket *pocket;
    u16 count = 0;

    if (pocketId >= POCKETS_COUNT || mode <= VP_ITEM_SORT_NONE || mode >= VP_ITEM_SORT_COUNT)
        return FALSE;

    // Preserve Emerald's numbered canonical order for these two pockets.
    if (pocketId == TMHM_POCKET || pocketId == BERRIES_POCKET)
        return FALSE;

    pocket = &gBagPockets[pocketId];
    CompactItemsInBagPocket(pocket);
    while (count < pocket->capacity && pocket->itemSlots[count].itemId != ITEM_NONE)
        count++;
    SortItemSlots(pocket->itemSlots, count, mode, TRUE);
    VanillaPlusSetBagSortMode(pocketId, mode);
    return TRUE;
}

void VanillaPlusSortPCItems(enum VanillaPlusItemSortMode mode)
{
    u8 count;

    if (mode <= VP_ITEM_SORT_NONE || mode >= VP_ITEM_SORT_COUNT)
        return;
    CompactPCItems();
    count = CountUsedPCItemSlots();
    SortItemSlots(gSaveBlock1Ptr->pcItems, count, mode, FALSE);
}

void VanillaPlusMaybeAutoSortBagPocket(u8 pocketId)
{
    enum VanillaPlusItemSortMode mode;

    if (!VanillaPlusIsAutoSortEnabled(pocketId))
        return;
    mode = VanillaPlusGetBagSortMode(pocketId);
    if (mode != VP_ITEM_SORT_NONE)
        VanillaPlusSortBagPocket(pocketId, mode);
}

enum VanillaPlusItemSortMode VanillaPlusGetBagSortMode(u8 pocketId)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();

    if (pocketId >= POCKETS_COUNT)
        return VP_ITEM_SORT_NONE;
    return meta->bagSortMode[pocketId];
}

void VanillaPlusSetBagSortMode(u8 pocketId, enum VanillaPlusItemSortMode mode)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();

    if (pocketId >= POCKETS_COUNT || mode >= VP_ITEM_SORT_COUNT)
        return;
    meta->bagSortMode[pocketId] = mode;
}

bool8 VanillaPlusIsAutoSortEnabled(u8 pocketId)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();

    if (pocketId >= POCKETS_COUNT)
        return FALSE;
    return (meta->autoSortMask & (1 << pocketId)) != 0;
}

void VanillaPlusSetAutoSortEnabled(u8 pocketId, bool8 enabled)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();

    if (pocketId >= POCKETS_COUNT || pocketId == TMHM_POCKET || pocketId == BERRIES_POCKET)
        return;
    if (enabled)
        meta->autoSortMask |= (1 << pocketId);
    else
        meta->autoSortMask &= ~(1 << pocketId);
}

void VanillaPlusSyncPrimaryRegisteredItem(void)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();

    if (meta->quickItemCount == 0)
        gSaveBlock1Ptr->registeredItem = ITEM_NONE;
    else
        gSaveBlock1Ptr->registeredItem = meta->quickItems[0];
}

bool8 VanillaPlusIsQuickItemRegistered(u16 itemId)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();
    u8 i;

    for (i = 0; i < meta->quickItemCount; i++)
    {
        if (meta->quickItems[i] == itemId)
            return TRUE;
    }
    return FALSE;
}

bool8 VanillaPlusRegisterQuickItem(u16 itemId)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();

    if (itemId == ITEM_NONE)
        return FALSE;
    if (VanillaPlusIsQuickItemRegistered(itemId))
        return TRUE;
    if (meta->quickItemCount >= VANILLAPLUS_QUICK_ITEM_MAX)
        return FALSE;

    meta->quickItems[meta->quickItemCount++] = itemId;
    VanillaPlusSyncPrimaryRegisteredItem();
    return TRUE;
}

bool8 VanillaPlusUnregisterQuickItem(u16 itemId)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();
    u8 i;
    u8 j;

    for (i = 0; i < meta->quickItemCount; i++)
    {
        if (meta->quickItems[i] == itemId)
        {
            for (j = i; j + 1 < meta->quickItemCount; j++)
                meta->quickItems[j] = meta->quickItems[j + 1];
            meta->quickItemCount--;
            meta->quickItems[meta->quickItemCount] = ITEM_NONE;
            if (meta->lastQuickItem >= meta->quickItemCount)
                meta->lastQuickItem = 0;
            VanillaPlusSyncPrimaryRegisteredItem();
            return TRUE;
        }
    }
    return FALSE;
}

u16 VanillaPlusGetQuickItem(u8 slot)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();

    if (slot >= meta->quickItemCount)
        return ITEM_NONE;
    return meta->quickItems[slot];
}

u8 VanillaPlusGetQuickItemCount(void)
{
    return GetItemMetadata()->quickItemCount;
}

void VanillaPlusPruneQuickItems(void)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();
    u16 kept[VANILLAPLUS_QUICK_ITEM_MAX] = {ITEM_NONE};
    u8 keptCount = 0;
    u8 i;

    for (i = 0; i < meta->quickItemCount; i++)
    {
        if (meta->quickItems[i] != ITEM_NONE && CheckBagHasItem(meta->quickItems[i], 1))
            kept[keptCount++] = meta->quickItems[i];
    }
    for (i = 0; i < VANILLAPLUS_QUICK_ITEM_MAX; i++)
        meta->quickItems[i] = kept[i];
    meta->quickItemCount = keptCount;
    if (meta->lastQuickItem >= keptCount)
        meta->lastQuickItem = 0;
    VanillaPlusSyncPrimaryRegisteredItem();
}

void VanillaPlusReplaceQuickItem(u16 oldItem, u16 newItem)
{
    struct VanillaPlusItemMetadata *meta = GetItemMetadata();
    u8 i;

    for (i = 0; i < meta->quickItemCount; i++)
    {
        if (meta->quickItems[i] == oldItem)
            meta->quickItems[i] = newItem;
    }
    VanillaPlusSyncPrimaryRegisteredItem();
}
