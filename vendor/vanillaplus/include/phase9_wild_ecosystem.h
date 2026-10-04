#ifndef GUARD_PHASE9_WILD_ECOSYSTEM_H
#define GUARD_PHASE9_WILD_ECOSYSTEM_H

#include "wild_encounter.h"

enum
{
    PHASE9_AREA_LAND,
    PHASE9_AREA_WATER,
    PHASE9_AREA_ROCKS,
    PHASE9_AREA_FISHING,
};

u16 Phase9ChooseWildSpecies(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 rod, bool8 consumeBag);
u8 Phase9ChooseWildLevel(void);

#endif // GUARD_PHASE9_WILD_ECOSYSTEM_H
