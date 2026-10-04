#include "global.h"
#include "phase9_wild_ecosystem.h"
#include "pokemon.h"
#include "random.h"
#include "constants/items.h"
#include "constants/maps.h"

#define PHASE9_LEVEL_DELTA 15
#define PHASE9_LEVEL_MIN   2
#define PHASE9_LEVEL_MAX   100
#define PHASE9_INVALID_KEY 0xFF
#define PHASE9_MAX_LOCAL_POOL 64

EWRAM_DATA static u16 sPhase9SpeciesBag[PHASE9_MAX_LOCAL_POOL] = {0};
EWRAM_DATA static u16 sPhase9SpeciesBagCount = 0;
EWRAM_DATA static u16 sPhase9SpeciesBagCursor = 0;
EWRAM_DATA static u16 sPhase9SpeciesBagHabitatMask = 0;
EWRAM_DATA static u16 sPhase9LastSpecies = SPECIES_NONE;
EWRAM_DATA static u8 sPhase9SpeciesBagMapGroup = PHASE9_INVALID_KEY;
EWRAM_DATA static u8 sPhase9SpeciesBagMapNum = PHASE9_INVALID_KEY;
EWRAM_DATA static u8 sPhase9SpeciesBagArea = PHASE9_INVALID_KEY;
EWRAM_DATA static u8 sPhase9SpeciesBagRod = PHASE9_INVALID_KEY;

#include "data/phase9_wild_ecology.h"

static void GetPhase9TableSlice(u8 area, u8 rod, u8 *start, u8 *count)
{
    *start = 0;

    switch (area)
    {
    case PHASE9_AREA_LAND:
        *count = LAND_WILD_COUNT;
        break;
    case PHASE9_AREA_WATER:
        *count = WATER_WILD_COUNT;
        break;
    case PHASE9_AREA_ROCKS:
        *count = ROCK_WILD_COUNT;
        break;
    case PHASE9_AREA_FISHING:
        switch (rod)
        {
        case OLD_ROD:
            *start = 0;
            *count = 2;
            break;
        case GOOD_ROD:
            *start = 2;
            *count = 3;
            break;
        case SUPER_ROD:
            *start = 5;
            *count = 5;
            break;
        default:
            *count = FISH_WILD_COUNT;
            break;
        }
        break;
    default:
        *count = 0;
        break;
    }
}

static bool8 Phase9SpeciesWasSeen(const u16 *seenSpecies, u8 seenCount, u16 species)
{
    u8 i;

    for (i = 0; i < seenCount; i++)
    {
        if (seenSpecies[i] == species)
            return TRUE;
    }

    return FALSE;
}

static u16 GetPhase9HabitatMaskFromTable(const struct WildPokemon *wildPokemon, u8 start, u8 count)
{
    u16 seenSpecies[LAND_WILD_COUNT];
    u8 habitatCounts[PHASE9_HABITAT_COUNT];
    u8 seenCount = 0;
    u8 bestHabitat = 0;
    u8 secondHabitat = 0;
    u8 bestCount = 0;
    u8 secondCount = 0;
    u8 i;
    u8 habitat;

    for (i = 0; i < PHASE9_HABITAT_COUNT; i++)
        habitatCounts[i] = 0;

    for (i = 0; i < count; i++)
    {
        u16 species = wildPokemon[start + i].species;
        u16 nationalDexNum;
        u16 habitatMask;

        if (species == SPECIES_NONE || Phase9SpeciesWasSeen(seenSpecies, seenCount, species))
            continue;

        seenSpecies[seenCount++] = species;
        nationalDexNum = SpeciesToNationalPokedexNum(species);
        if (nationalDexNum == 0 || nationalDexNum > NATIONAL_DEX_COUNT)
            continue;

        habitatMask = sPhase9HabitatByNationalDex[nationalDexNum];
        for (habitat = 0; habitat < PHASE9_HABITAT_COUNT; habitat++)
        {
            if (habitatMask & (1 << habitat))
                habitatCounts[habitat]++;
        }
    }

    for (habitat = 0; habitat < PHASE9_HABITAT_COUNT; habitat++)
    {
        if (habitatCounts[habitat] > bestCount)
        {
            secondCount = bestCount;
            secondHabitat = bestHabitat;
            bestCount = habitatCounts[habitat];
            bestHabitat = habitat;
        }
        else if (habitatCounts[habitat] > secondCount)
        {
            secondCount = habitatCounts[habitat];
            secondHabitat = habitat;
        }
    }

    if (bestCount == 0)
        return 0;

    // A route may represent a mixed biome, but a one-off vanilla species should
    // not drag an unrelated entire habitat into the new encounter pool.
    if (secondCount != 0 && secondCount * 2 >= bestCount)
        return (1 << bestHabitat) | (1 << secondHabitat);

    return (1 << bestHabitat);
}

static u16 GetPhase9CurrentMapId(void)
{
    return gSaveBlock1Ptr->location.mapNum | (gSaveBlock1Ptr->location.mapGroup << 8);
}

static u16 ApplyPhase9MapHabitatOverride(u16 habitatMask, u8 area)
{
    u16 mapId;

    // Water and rod fauna keep the ecology inferred from their own vanilla
    // slices. The overrides below refine land/rock micro-biomes only.
    if (area != PHASE9_AREA_LAND && area != PHASE9_AREA_ROCKS)
        return habitatMask;

    mapId = GetPhase9CurrentMapId();

    switch (mapId)
    {
    // Volcanic belt.
    case MAP_MT_CHIMNEY:
    case MAP_JAGGED_PASS:
    case MAP_FIERY_PATH:
    case MAP_MAGMA_HIDEOUT_1F:
    case MAP_MAGMA_HIDEOUT_2F_1R:
    case MAP_MAGMA_HIDEOUT_2F_2R:
    case MAP_MAGMA_HIDEOUT_3F_1R:
    case MAP_MAGMA_HIDEOUT_3F_2R:
    case MAP_MAGMA_HIDEOUT_4F:
    case MAP_MAGMA_HIDEOUT_3F_3R:
    case MAP_MAGMA_HIDEOUT_2F_3R:
        return PHASE9_HABITAT_MOUNTAIN | PHASE9_HABITAT_ROUGH_TERRAIN;

    // Mt. Pyre is a wooded sacred ruin rather than a generic town/urban area.
    case MAP_MT_PYRE_1F:
    case MAP_MT_PYRE_2F:
    case MAP_MT_PYRE_3F:
    case MAP_MT_PYRE_4F:
    case MAP_MT_PYRE_5F:
    case MAP_MT_PYRE_6F:
    case MAP_MT_PYRE_EXTERIOR:
    case MAP_MT_PYRE_SUMMIT:
        return PHASE9_HABITAT_CAVE | PHASE9_HABITAT_FOREST;

    // Shoal Cave land fauna is cave/mountain ecology. Its water and fishing
    // slices are deliberately inferred independently above.
    case MAP_SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM:
    case MAP_SHOAL_CAVE_LOW_TIDE_INNER_ROOM:
    case MAP_SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM:
    case MAP_SHOAL_CAVE_LOW_TIDE_LOWER_ROOM:
    case MAP_SHOAL_CAVE_HIGH_TIDE_ENTRANCE_ROOM:
    case MAP_SHOAL_CAVE_HIGH_TIDE_INNER_ROOM:
    case MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM:
        return PHASE9_HABITAT_CAVE | PHASE9_HABITAT_MOUNTAIN;

    // Human-made industrial ruins.
    case MAP_NEW_MAUVILLE_ENTRANCE:
    case MAP_NEW_MAUVILLE_INSIDE:
        return PHASE9_HABITAT_URBAN | PHASE9_HABITAT_ROUGH_TERRAIN;

    // Desert / arid terrain.
    case MAP_ROUTE111:
    case MAP_DESERT_RUINS:
    case MAP_MIRAGE_TOWER_1F:
    case MAP_MIRAGE_TOWER_2F:
    case MAP_MIRAGE_TOWER_3F:
    case MAP_MIRAGE_TOWER_4F:
    case MAP_DESERT_UNDERPASS:
        return PHASE9_HABITAT_ROUGH_TERRAIN | PHASE9_HABITAT_GRASSLAND;

    // Seafloor Cavern's walkable areas remain cave/rough terrain.
    case MAP_SEAFLOOR_CAVERN_ENTRANCE:
    case MAP_SEAFLOOR_CAVERN_ROOM1:
    case MAP_SEAFLOOR_CAVERN_ROOM2:
    case MAP_SEAFLOOR_CAVERN_ROOM3:
    case MAP_SEAFLOOR_CAVERN_ROOM4:
    case MAP_SEAFLOOR_CAVERN_ROOM5:
    case MAP_SEAFLOOR_CAVERN_ROOM6:
    case MAP_SEAFLOOR_CAVERN_ROOM7:
    case MAP_SEAFLOOR_CAVERN_ROOM8:
    case MAP_SEAFLOOR_CAVERN_ROOM9:
        return PHASE9_HABITAT_CAVE | PHASE9_HABITAT_ROUGH_TERRAIN;
    }

    return habitatMask;
}

static u16 GetPhase9HabitatMask(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 rod)
{
    u8 start;
    u8 count;
    u16 habitatMask;

    GetPhase9TableSlice(area, rod, &start, &count);
    if (wildMonInfo == NULL || wildMonInfo->wildPokemon == NULL || count == 0)
        return 0;

    habitatMask = GetPhase9HabitatMaskFromTable(wildMonInfo->wildPokemon, start, count);
    return ApplyPhase9MapHabitatOverride(habitatMask, area);
}

static u16 ChoosePhase9FallbackSpecies(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 rod)
{
    u16 seenSpecies[LAND_WILD_COUNT];
    u16 choice = SPECIES_NONE;
    u8 seenCount = 0;
    u8 start;
    u8 count;
    u8 i;

    GetPhase9TableSlice(area, rod, &start, &count);
    if (wildMonInfo == NULL || wildMonInfo->wildPokemon == NULL || count == 0)
        return SPECIES_NONE;

    for (i = 0; i < count; i++)
    {
        u16 species = wildMonInfo->wildPokemon[start + i].species;

        if (species == SPECIES_NONE || Phase9SpeciesWasSeen(seenSpecies, seenCount, species))
            continue;

        seenSpecies[seenCount++] = species;
        if (Random() % seenCount == 0)
            choice = species;
    }

    return choice;
}

static bool8 Phase9SpeciesIsInVanillaSlice(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 rod, u16 species)
{
    u8 start;
    u8 count;
    u8 i;

    GetPhase9TableSlice(area, rod, &start, &count);
    if (wildMonInfo == NULL || wildMonInfo->wildPokemon == NULL)
        return FALSE;

    for (i = 0; i < count; i++)
    {
        if (wildMonInfo->wildPokemon[start + i].species == species)
            return TRUE;
    }

    return FALSE;
}

static u16 CountPhase9HabitatCandidates(u16 habitatMask)
{
    u16 count = 0;
    u16 species;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        u16 nationalDexNum = SpeciesToNationalPokedexNum(species);

        if (nationalDexNum == 0 || nationalDexNum > NATIONAL_DEX_COUNT)
            continue;
        if (sPhase9HabitatByNationalDex[nationalDexNum] & habitatMask)
            count++;
    }

    return count;
}

static u8 GetPhase9LocalPoolDivisor(u16 habitatCandidateCount)
{
    if (habitatCandidateCount > 120)
        return 6;
    if (habitatCandidateCount > 80)
        return 4;
    if (habitatCandidateCount > 50)
        return 3;
    if (habitatCandidateCount > 16)
        return 2;

    return 1;
}

static u32 GetPhase9LocalSpeciesHash(u16 nationalDexNum, u8 area, u8 rod)
{
    u32 value;

    value = (u32)nationalDexNum * 0x045D9F3B;
    value ^= (u32)(gSaveBlock1Ptr->location.mapGroup + 1) * 0x27D4EB2D;
    value ^= (u32)(gSaveBlock1Ptr->location.mapNum + 1) * 0x165667B1;
    value ^= (u32)(area + 1) * 0x1B873593;
    value ^= (u32)(rod + 1) * 0x85EBCA6B;
    value ^= value >> 16;
    value *= 0x7FEB352D;
    value ^= value >> 15;

    return value;
}

static bool8 IsPhase9GuaranteedLocalSpecies(u16 species, u8 area)
{
    u16 mapId = GetPhase9CurrentMapId();

    // The deterministic partition reaches 385/386 species by itself.
    // Mr. Mime's canonical Gen III habitat is urban; New Mauville is its
    // explicit physical home so complete National Dex coverage is guaranteed.
    if (species == SPECIES_MR_MIME
        && area == PHASE9_AREA_LAND
        && (mapId == MAP_NEW_MAUVILLE_ENTRANCE || mapId == MAP_NEW_MAUVILLE_INSIDE))
        return TRUE;

    return FALSE;
}

static bool8 IsPhase9LocalPoolSpecies(
    const struct WildPokemonInfo *wildMonInfo,
    u8 area,
    u8 rod,
    u16 habitatMask,
    u8 divisor,
    u16 species)
{
    u16 nationalDexNum;

    // Preserve each map/method's original species as ecological anchors. They
    // get no extra weight: one species still occupies one bag entry.
    if (Phase9SpeciesIsInVanillaSlice(wildMonInfo, area, rod, species))
        return TRUE;

    nationalDexNum = SpeciesToNationalPokedexNum(species);
    if (nationalDexNum == 0 || nationalDexNum > NATIONAL_DEX_COUNT)
        return FALSE;
    if (!(sPhase9HabitatByNationalDex[nationalDexNum] & habitatMask))
        return FALSE;
    if (IsPhase9GuaranteedLocalSpecies(species, area))
        return TRUE;

    // Deterministically shard a broad habitat across individual routes. This
    // prevents every forest/grassland/ocean route from exposing the same huge
    // pool while retaining equal probability inside each local pool.
    return (GetPhase9LocalSpeciesHash(nationalDexNum, area, rod) % divisor) == 0;
}

static u16 ChoosePhase9UniformSpecies(
    const struct WildPokemonInfo *wildMonInfo,
    u8 area,
    u8 rod,
    u16 habitatMask)
{
    u16 choice = SPECIES_NONE;
    u16 eligibleCount = 0;
    u16 habitatCandidateCount = CountPhase9HabitatCandidates(habitatMask);
    u8 divisor = GetPhase9LocalPoolDivisor(habitatCandidateCount);
    u16 species;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        if (!IsPhase9LocalPoolSpecies(wildMonInfo, area, rod, habitatMask, divisor, species))
            continue;

        eligibleCount++;
        if (Random() % eligibleCount == 0)
            choice = species;
    }

    return choice;
}

static bool8 Phase9BagKeyMatches(u8 area, u8 rod, u16 habitatMask)
{
    if (sPhase9SpeciesBagMapGroup != gSaveBlock1Ptr->location.mapGroup)
        return FALSE;
    if (sPhase9SpeciesBagMapNum != gSaveBlock1Ptr->location.mapNum)
        return FALSE;
    if (sPhase9SpeciesBagArea != area)
        return FALSE;
    if (sPhase9SpeciesBagRod != rod)
        return FALSE;
    if (sPhase9SpeciesBagHabitatMask != habitatMask)
        return FALSE;

    return TRUE;
}

static void BuildPhase9SpeciesBag(
    const struct WildPokemonInfo *wildMonInfo,
    u8 area,
    u8 rod,
    u16 habitatMask)
{
    u16 habitatCandidateCount = CountPhase9HabitatCandidates(habitatMask);
    u8 divisor = GetPhase9LocalPoolDivisor(habitatCandidateCount);
    u16 species;
    u16 i;

    sPhase9SpeciesBagCount = 0;
    sPhase9SpeciesBagCursor = 0;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        if (!IsPhase9LocalPoolSpecies(wildMonInfo, area, rod, habitatMask, divisor, species))
            continue;

        if (sPhase9SpeciesBagCount < PHASE9_MAX_LOCAL_POOL)
            sPhase9SpeciesBag[sPhase9SpeciesBagCount++] = species;
    }

    // Fisher-Yates: one copy of every eligible local species gives exact
    // equality inside each bag cycle and eliminates duplicate streaks.
    for (i = sPhase9SpeciesBagCount; i > 1; i--)
    {
        u16 swapIndex = Random() % i;
        u16 temp = sPhase9SpeciesBag[i - 1];

        sPhase9SpeciesBag[i - 1] = sPhase9SpeciesBag[swapIndex];
        sPhase9SpeciesBag[swapIndex] = temp;
    }

    // Do not allow the boundary between two bags to repeat the last species.
    if (sPhase9SpeciesBagCount > 1 && sPhase9SpeciesBag[0] == sPhase9LastSpecies)
    {
        u16 swapIndex = 1 + (Random() % (sPhase9SpeciesBagCount - 1));
        u16 temp = sPhase9SpeciesBag[0];

        sPhase9SpeciesBag[0] = sPhase9SpeciesBag[swapIndex];
        sPhase9SpeciesBag[swapIndex] = temp;
    }

    sPhase9SpeciesBagMapGroup = gSaveBlock1Ptr->location.mapGroup;
    sPhase9SpeciesBagMapNum = gSaveBlock1Ptr->location.mapNum;
    sPhase9SpeciesBagArea = area;
    sPhase9SpeciesBagRod = rod;
    sPhase9SpeciesBagHabitatMask = habitatMask;
}

u16 Phase9ChooseWildSpecies(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 rod, bool8 consumeBag)
{
    u16 habitatMask = GetPhase9HabitatMask(wildMonInfo, area, rod);
    u16 species;

    if (habitatMask == 0)
        return ChoosePhase9FallbackSpecies(wildMonInfo, area, rod);

    if (!consumeBag)
    {
        species = ChoosePhase9UniformSpecies(wildMonInfo, area, rod, habitatMask);
        if (species == SPECIES_NONE)
            species = ChoosePhase9FallbackSpecies(wildMonInfo, area, rod);
        return species;
    }

    if (!Phase9BagKeyMatches(area, rod, habitatMask)
        || sPhase9SpeciesBagCursor >= sPhase9SpeciesBagCount)
    {
        BuildPhase9SpeciesBag(wildMonInfo, area, rod, habitatMask);
    }

    if (sPhase9SpeciesBagCount == 0)
        return ChoosePhase9FallbackSpecies(wildMonInfo, area, rod);

    species = sPhase9SpeciesBag[sPhase9SpeciesBagCursor++];
    sPhase9LastSpecies = species;
    return species;
}

u8 Phase9ChooseWildLevel(void)
{
    u16 totalLevel = 0;
    u16 averageLevel;
    u16 minLevel;
    u16 maxLevel;
    u8 eligiblePartyCount = 0;
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);

        if (species == SPECIES_NONE)
            continue;
        if (GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
            continue;
        if (GetMonData(&gPlayerParty[i], MON_DATA_HP) == 0)
            continue;

        totalLevel += GetMonData(&gPlayerParty[i], MON_DATA_LEVEL);
        eligiblePartyCount++;
    }

    if (eligiblePartyCount == 0)
        return PHASE9_LEVEL_MIN;

    averageLevel = (totalLevel + eligiblePartyCount / 2) / eligiblePartyCount;

    if (averageLevel > PHASE9_LEVEL_DELTA)
        minLevel = averageLevel - PHASE9_LEVEL_DELTA;
    else
        minLevel = PHASE9_LEVEL_MIN;

    if (minLevel < PHASE9_LEVEL_MIN)
        minLevel = PHASE9_LEVEL_MIN;

    maxLevel = averageLevel + PHASE9_LEVEL_DELTA;
    if (maxLevel > PHASE9_LEVEL_MAX)
        maxLevel = PHASE9_LEVEL_MAX;

    return minLevel + (Random() % (maxLevel - minLevel + 1));
}
