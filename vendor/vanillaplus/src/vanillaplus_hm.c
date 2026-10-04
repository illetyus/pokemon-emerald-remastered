#include "global.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "vanillaplus_hm.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"

bool8 IsVanillaPlusHmItem(u16 itemId)
{
    return itemId >= ITEM_HM01_CUT && itemId <= ITEM_HM08_DIVE;
}

u16 GetVanillaPlusHmMove(u16 itemId)
{
    switch (itemId)
    {
    case ITEM_HM01_CUT:
        return MOVE_CUT;
    case ITEM_HM02_FLY:
        return MOVE_FLY;
    case ITEM_HM03_SURF:
        return MOVE_SURF;
    case ITEM_HM04_STRENGTH:
        return MOVE_STRENGTH;
    case ITEM_HM05_FLASH:
        return MOVE_FLASH;
    case ITEM_HM06_ROCK_SMASH:
        return MOVE_ROCK_SMASH;
    case ITEM_HM07_WATERFALL:
        return MOVE_WATERFALL;
    case ITEM_HM08_DIVE:
        return MOVE_DIVE;
    default:
        return MOVE_NONE;
    }
}

u16 GetVanillaPlusHmBadgeFlag(u16 itemId)
{
    switch (itemId)
    {
    case ITEM_HM01_CUT:
        return FLAG_BADGE01_GET;
    case ITEM_HM05_FLASH:
        return FLAG_BADGE02_GET;
    case ITEM_HM06_ROCK_SMASH:
        return FLAG_BADGE03_GET;
    case ITEM_HM04_STRENGTH:
        return FLAG_BADGE04_GET;
    case ITEM_HM03_SURF:
        return FLAG_BADGE05_GET;
    case ITEM_HM02_FLY:
        return FLAG_BADGE06_GET;
    case ITEM_HM08_DIVE:
        return FLAG_BADGE07_GET;
    case ITEM_HM07_WATERFALL:
        return FLAG_BADGE08_GET;
    default:
        return 0;
    }
}

bool8 HasVanillaPlusHmAccess(u16 itemId)
{
    u16 badgeFlag = GetVanillaPlusHmBadgeFlag(itemId);

    if (badgeFlag == 0)
        return FALSE;

    return CheckBagHasItem(itemId, 1) && FlagGet(badgeFlag);
}

u8 GetVanillaPlusHmFieldActor(void)
{
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE
         && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
            return i;
    }

    return PARTY_SIZE;
}
