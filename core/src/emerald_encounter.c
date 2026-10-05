#include "remaster/emerald_encounter.h"

#include "remaster/emerald_items.h"
#include "remaster/emerald_state.h"

#include <string.h>

enum {
    R12_BEHAVIOR_ENCOUNTER = 1 << 0,
    R12_BEHAVIOR_SURFABLE = 1 << 1,

    R12_SB1_WEATHER = 0x002E,

    R12_SB1_OUTBREAK_SPECIES = 0x2B90,
    R12_SB1_OUTBREAK_MAP_NUM = 0x2B92,
    R12_SB1_OUTBREAK_MAP_GROUP = 0x2B93,
    R12_SB1_OUTBREAK_LEVEL = 0x2B94,
    R12_SB1_OUTBREAK_MOVES = 0x2B98,
    R12_SB1_OUTBREAK_PROBABILITY = 0x2BA1,

    R12_SB1_ROAMER = 0x31DC,

    R12_SB2_PLAYER_GENDER = 0x0008,
    R12_SB2_TRAINER_ID = 0x000A,

    R12_MAIL_NONE = 0xFF
};

typedef struct RemasterEmeraldEncounterHabitatOverride {
    uint8_t map_group;
    uint8_t map_num;
    uint16_t habitat_mask;
} RemasterEmeraldEncounterHabitatOverride;

typedef struct RemasterEmeraldEncounterGuarantee {
    uint8_t map_group;
    uint8_t map_num;
    uint8_t area;
    uint16_t species;
} RemasterEmeraldEncounterGuarantee;

typedef struct RemasterEmeraldEncounterLearnsetEntry {
    uint8_t level;
    uint16_t move;
} RemasterEmeraldEncounterLearnsetEntry;

typedef struct RemasterEmeraldEncounterLearnsetSlice {
    uint16_t start;
    uint16_t count;
} RemasterEmeraldEncounterLearnsetSlice;

#include "emerald_encounter_catalog.inc"

static uint16_t r12_read16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static uint32_t r12_read32(const uint8_t *p)
{
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8u)
        | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
}

static void r12_write16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static int r12_secure_is_egg(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    uint32_t word;

    if (pokemon == 0)
        return 0;

    word = (uint32_t)pokemon->substruct[3][4]
        | ((uint32_t)pokemon->substruct[3][5] << 8u)
        | ((uint32_t)pokemon->substruct[3][6] << 16u)
        | ((uint32_t)pokemon->substruct[3][7] << 24u);
    return (int)((word >> 30u) & 1u);
}

static int r12_sanity_is_egg(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return pokemon != 0 && (pokemon->header_flags & 0x04u) != 0;
}

static int r12_load_party_mon(
    const RemasterEmeraldSave *save,
    size_t slot,
    RemasterEmeraldPartyPokemon *out)
{
    int checksum_valid = 0;

    if (save == 0 || out == 0)
        return 0;

    if (!remaster_emerald_party_get(
            save,
            slot,
            out,
            &checksum_valid))
        return 0;

    (void)checksum_valid;
    return remaster_emerald_box_pokemon_species(&out->box) != 0;
}

static uint8_t r12_lead_ability(
    const RemasterEmeraldSave *save,
    RemasterEmeraldPartyPokemon *out_lead,
    int *out_sanity_egg)
{
    RemasterEmeraldPartyPokemon lead;
    uint16_t species;
    uint8_t ability_num;

    memset(&lead, 0, sizeof(lead));
    if (!r12_load_party_mon(save, 0, &lead)) {
        if (out_lead != 0)
            memset(out_lead, 0, sizeof(*out_lead));
        if (out_sanity_egg != 0)
            *out_sanity_egg = 0;
        return 0;
    }

    if (out_lead != 0)
        *out_lead = lead;

    if (out_sanity_egg != 0)
        *out_sanity_egg = r12_sanity_is_egg(&lead.box);

    if (r12_sanity_is_egg(&lead.box))
        return 0;

    species = remaster_emerald_box_pokemon_species(&lead.box);
    ability_num = remaster_emerald_box_pokemon_ability_num(&lead.box);
    return remaster_emerald_species_ability(species, ability_num);
}

static uint8_t r12_gender_for_personality(
    uint16_t species,
    uint32_t personality)
{
    const RemasterEmeraldSpeciesInfo *info =
        remaster_emerald_species_info(species);

    if (info == 0)
        return REMASTER_EMERALD_GENDER_GENDERLESS;

    if (info->gender_ratio == 0x00u)
        return REMASTER_EMERALD_GENDER_MALE;
    if (info->gender_ratio == 0xFEu)
        return REMASTER_EMERALD_GENDER_FEMALE;
    if (info->gender_ratio == 0xFFu)
        return REMASTER_EMERALD_GENDER_GENDERLESS;

    return (uint8_t)(personality & 0xFFu) < info->gender_ratio
        ? REMASTER_EMERALD_GENDER_FEMALE
        : REMASTER_EMERALD_GENDER_MALE;
}

static int r12_species_supports_cute_charm(uint16_t species)
{
    const RemasterEmeraldSpeciesInfo *info =
        remaster_emerald_species_info(species);

    if (info == 0)
        return 0;

    return info->gender_ratio != 0x00u
        && info->gender_ratio != 0xFEu
        && info->gender_ratio != 0xFFu;
}

void remaster_emerald_encounter_rng_seed(
    RemasterEmeraldEncounterRng *rng,
    uint32_t seed)
{
    if (rng == 0)
        return;

    rng->state = seed;
    rng->calls = 0;
}

uint16_t remaster_emerald_encounter_random(
    RemasterEmeraldEncounterRng *rng)
{
    if (rng == 0)
        return 0;

    rng->state = UINT32_C(1103515245) * rng->state + UINT32_C(24691);
    rng->calls++;
    return (uint16_t)(rng->state >> 16u);
}

uint32_t remaster_emerald_encounter_random32(
    RemasterEmeraldEncounterRng *rng)
{
    const uint32_t low = remaster_emerald_encounter_random(rng);
    const uint32_t high = remaster_emerald_encounter_random(rng);

    return low | (high << 16u);
}

void remaster_emerald_encounter_runtime_init(
    RemasterEmeraldEncounterRuntime *runtime,
    uint32_t seed)
{
    if (runtime == 0)
        return;

    memset(runtime, 0, sizeof(*runtime));
    remaster_emerald_encounter_rng_seed(&runtime->rng, seed);
}

void remaster_emerald_encounter_restart_immunity(
    RemasterEmeraldEncounterRuntime *runtime)
{
    if (runtime == 0)
        return;

    runtime->wild_immunity_steps = 0;
    runtime->previous_behavior_valid = 0;
}

size_t remaster_emerald_encounter_map_count(void)
{
    return sizeof(kRemasterEncounterMaps)
        / sizeof(kRemasterEncounterMaps[0]);
}

const RemasterEmeraldEncounterMapInfo *remaster_emerald_encounter_map_at(
    size_t index)
{
    if (index >= remaster_emerald_encounter_map_count())
        return 0;

    return &kRemasterEncounterMaps[index];
}

const RemasterEmeraldEncounterMapInfo *remaster_emerald_encounter_map_find(
    uint8_t map_group,
    uint8_t map_num)
{
    size_t i;

    for (i = 0; i < remaster_emerald_encounter_map_count(); ++i) {
        if (kRemasterEncounterMaps[i].map_group == map_group
            && kRemasterEncounterMaps[i].map_num == map_num)
            return &kRemasterEncounterMaps[i];
    }

    return 0;
}

uint16_t remaster_emerald_encounter_species_habitat(uint16_t species)
{
    if ((size_t)species
        >= sizeof(kRemasterEncounterSpeciesHabitat)
            / sizeof(kRemasterEncounterSpeciesHabitat[0]))
        return 0;

    return kRemasterEncounterSpeciesHabitat[species];
}

uint8_t remaster_emerald_encounter_area_from_behavior(
    uint8_t behavior,
    uint8_t surfing)
{
    uint8_t flags;

    if ((size_t)behavior
        >= sizeof(kRemasterEncounterBehaviorFlags)
            / sizeof(kRemasterEncounterBehaviorFlags[0]))
        return REMASTER_EMERALD_ENCOUNTER_AREA_NONE;

    flags = kRemasterEncounterBehaviorFlags[behavior];

    if ((flags & R12_BEHAVIOR_ENCOUNTER) != 0) {
        return (flags & R12_BEHAVIOR_SURFABLE) != 0
            ? REMASTER_EMERALD_ENCOUNTER_AREA_WATER
            : REMASTER_EMERALD_ENCOUNTER_AREA_LAND;
    }

    if (surfing
        && kRemasterEncounterBridgeFlags[behavior] != 0)
        return REMASTER_EMERALD_ENCOUNTER_AREA_WATER;

    return REMASTER_EMERALD_ENCOUNTER_AREA_NONE;
}

static const RemasterEmeraldEncounterAreaInfo *r12_area_info(
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area)
{
    if (map == 0)
        return 0;

    switch ((RemasterEmeraldEncounterArea)area) {
    case REMASTER_EMERALD_ENCOUNTER_AREA_LAND:
        return &map->land;
    case REMASTER_EMERALD_ENCOUNTER_AREA_WATER:
        return &map->water;
    case REMASTER_EMERALD_ENCOUNTER_AREA_ROCKS:
        return &map->rocks;
    case REMASTER_EMERALD_ENCOUNTER_AREA_FISHING:
        return &map->fishing;
    default:
        return 0;
    }
}

static void r12_anchor_slice(
    const RemasterEmeraldEncounterAreaInfo *info,
    uint8_t area,
    uint8_t rod,
    size_t *out_start,
    size_t *out_count)
{
    size_t start = 0;
    size_t count = info != 0 ? info->anchor_count : 0;

    if (area == REMASTER_EMERALD_ENCOUNTER_AREA_FISHING) {
        switch ((RemasterEmeraldEncounterRod)rod) {
        case REMASTER_EMERALD_ROD_OLD:
            start = 0;
            count = count >= 2u ? 2u : count;
            break;
        case REMASTER_EMERALD_ROD_GOOD:
            start = count > 2u ? 2u : count;
            count = count >= 5u ? 3u : (count > start ? count - start : 0u);
            break;
        case REMASTER_EMERALD_ROD_SUPER:
            start = count > 5u ? 5u : count;
            count = count > start ? count - start : 0u;
            if (count > 5u)
                count = 5u;
            break;
        default:
            break;
        }
    }

    *out_start = start;
    *out_count = count;
}

static int r12_seen_species(
    const uint16_t *seen,
    size_t seen_count,
    uint16_t species)
{
    size_t i;

    for (i = 0; i < seen_count; ++i) {
        if (seen[i] == species)
            return 1;
    }
    return 0;
}

static uint16_t r12_inferred_habitat(
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod)
{
    const RemasterEmeraldEncounterAreaInfo *info =
        r12_area_info(map, area);
    uint16_t seen[REMASTER_EMERALD_ENCOUNTER_MAX_ANCHORS];
    uint8_t counts[8] = {0};
    size_t start = 0;
    size_t count = 0;
    size_t seen_count = 0;
    size_t i;
    uint8_t best_habitat = 0;
    uint8_t second_habitat = 0;
    uint8_t best_count = 0;
    uint8_t second_count = 0;
    uint16_t mask;

    if (info == 0 || info->anchor_count == 0)
        return 0;

    r12_anchor_slice(info, area, rod, &start, &count);

    for (i = 0; i < count; ++i) {
        const uint16_t species = info->anchors[start + i];
        const uint16_t species_mask =
            remaster_emerald_encounter_species_habitat(species);
        uint8_t habitat;

        if (species == 0
            || r12_seen_species(seen, seen_count, species))
            continue;

        if (seen_count < REMASTER_EMERALD_ENCOUNTER_MAX_ANCHORS)
            seen[seen_count++] = species;

        for (habitat = 0; habitat < 8u; ++habitat) {
            if ((species_mask & ((uint16_t)1u << habitat)) != 0)
                counts[habitat]++;
        }
    }

    for (i = 0; i < 8u; ++i) {
        if (counts[i] > best_count) {
            second_count = best_count;
            second_habitat = best_habitat;
            best_count = counts[i];
            best_habitat = (uint8_t)i;
        } else if (counts[i] > second_count) {
            second_count = counts[i];
            second_habitat = (uint8_t)i;
        }
    }

    if (best_count == 0)
        return 0;

    mask = (uint16_t)1u << best_habitat;
    if (second_count != 0
        && (uint16_t)second_count * 2u >= best_count)
        mask |= (uint16_t)1u << second_habitat;

    if (area == REMASTER_EMERALD_ENCOUNTER_AREA_LAND
        || area == REMASTER_EMERALD_ENCOUNTER_AREA_ROCKS) {
        for (i = 0;
             i < sizeof(kRemasterEncounterHabitatOverrides)
                / sizeof(kRemasterEncounterHabitatOverrides[0]);
             ++i) {
            if (kRemasterEncounterHabitatOverrides[i].map_group
                    == map->map_group
                && kRemasterEncounterHabitatOverrides[i].map_num
                    == map->map_num)
                return kRemasterEncounterHabitatOverrides[i].habitat_mask;
        }
    }

    return mask;
}

static int r12_anchor_contains(
    const RemasterEmeraldEncounterAreaInfo *info,
    uint8_t area,
    uint8_t rod,
    uint16_t species)
{
    size_t start = 0;
    size_t count = 0;
    size_t i;

    if (info == 0)
        return 0;

    r12_anchor_slice(info, area, rod, &start, &count);
    for (i = 0; i < count; ++i) {
        if (info->anchors[start + i] == species)
            return 1;
    }

    return 0;
}

static uint16_t r12_habitat_candidate_count(uint16_t habitat_mask)
{
    uint16_t species;
    uint16_t count = 0;

    for (species = 1;
         (size_t)species
            < sizeof(kRemasterEncounterSpeciesHabitat)
                / sizeof(kRemasterEncounterSpeciesHabitat[0]);
         ++species) {
        if ((remaster_emerald_encounter_species_habitat(species)
                & habitat_mask) != 0)
            count++;
    }

    return count;
}

static uint8_t r12_pool_divisor(uint16_t candidate_count)
{
    if (candidate_count > 120u)
        return 6;
    if (candidate_count > 80u)
        return 4;
    if (candidate_count > 50u)
        return 3;
    if (candidate_count > 16u)
        return 2;
    return 1;
}

static uint32_t r12_species_hash(
    uint16_t species,
    uint8_t map_group,
    uint8_t map_num,
    uint8_t area,
    uint8_t rod)
{
    uint32_t value = (uint32_t)species * UINT32_C(0x045D9F3B);

    value ^= (uint32_t)(map_group + 1u) * UINT32_C(0x27D4EB2D);
    value ^= (uint32_t)(map_num + 1u) * UINT32_C(0x165667B1);
    value ^= (uint32_t)(area + 1u) * UINT32_C(0x1B873593);
    value ^= (uint32_t)(rod + 1u) * UINT32_C(0x85EBCA6B);
    value ^= value >> 16u;
    value *= UINT32_C(0x7FEB352D);
    value ^= value >> 15u;

    return value;
}

static int r12_guaranteed_species(
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint16_t species)
{
    size_t i;

    for (i = 0;
         i < sizeof(kRemasterEncounterGuarantees)
            / sizeof(kRemasterEncounterGuarantees[0]);
         ++i) {
        const RemasterEmeraldEncounterGuarantee *entry =
            &kRemasterEncounterGuarantees[i];

        if (entry->map_group == map->map_group
            && entry->map_num == map->map_num
            && entry->area == area
            && entry->species == species)
            return 1;
    }

    return 0;
}

static int r12_local_species(
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod,
    uint16_t habitat_mask,
    uint8_t divisor,
    uint16_t species)
{
    const RemasterEmeraldEncounterAreaInfo *info =
        r12_area_info(map, area);
    uint16_t species_mask;

    if (r12_anchor_contains(info, area, rod, species))
        return 1;

    species_mask =
        remaster_emerald_encounter_species_habitat(species);
    if (species_mask == 0
        || (species_mask & habitat_mask) == 0)
        return 0;

    if (r12_guaranteed_species(map, area, species))
        return 1;

    return (r12_species_hash(
        species,
        map->map_group,
        map->map_num,
        area,
        rod) % divisor) == 0u;
}

static int r12_bag_key_matches(
    const RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod,
    uint16_t habitat_mask)
{
    return runtime->species_bag_valid
        && runtime->species_bag_map_group == map->map_group
        && runtime->species_bag_map_num == map->map_num
        && runtime->species_bag_area == area
        && runtime->species_bag_rod == rod
        && runtime->species_bag_habitat_mask == habitat_mask;
}

int remaster_emerald_encounter_build_local_pool(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod)
{
    const uint16_t habitat_mask =
        r12_inferred_habitat(map, area, rod);
    const uint16_t candidate_count =
        r12_habitat_candidate_count(habitat_mask);
    const uint8_t divisor = r12_pool_divisor(candidate_count);
    uint16_t species;
    uint16_t i;

    if (runtime == 0 || map == 0 || habitat_mask == 0)
        return 0;

    runtime->species_bag_count = 0;
    runtime->species_bag_cursor = 0;

    for (species = 1;
         (size_t)species
            < sizeof(kRemasterEncounterSpeciesHabitat)
                / sizeof(kRemasterEncounterSpeciesHabitat[0]);
         ++species) {
        if (!r12_local_species(
                map,
                area,
                rod,
                habitat_mask,
                divisor,
                species))
            continue;

        if (runtime->species_bag_count
            < REMASTER_EMERALD_ENCOUNTER_MAX_LOCAL_POOL) {
            runtime->species_bag[runtime->species_bag_count++] =
                species;
        }
    }

    for (i = runtime->species_bag_count; i > 1u; --i) {
        const uint16_t swap_index =
            (uint16_t)(
                remaster_emerald_encounter_random(&runtime->rng)
                % i);
        const uint16_t temp = runtime->species_bag[i - 1u];

        runtime->species_bag[i - 1u] =
            runtime->species_bag[swap_index];
        runtime->species_bag[swap_index] = temp;
    }

    if (runtime->species_bag_count > 1u
        && runtime->species_bag[0] == runtime->last_species) {
        const uint16_t swap_index = (uint16_t)(
            1u
            + remaster_emerald_encounter_random(&runtime->rng)
                % (runtime->species_bag_count - 1u));
        const uint16_t temp = runtime->species_bag[0];

        runtime->species_bag[0] =
            runtime->species_bag[swap_index];
        runtime->species_bag[swap_index] = temp;
    }

    runtime->species_bag_map_group = map->map_group;
    runtime->species_bag_map_num = map->map_num;
    runtime->species_bag_area = area;
    runtime->species_bag_rod = rod;
    runtime->species_bag_habitat_mask = habitat_mask;
    runtime->species_bag_valid = 1;

    return runtime->species_bag_count != 0;
}

static uint16_t r12_fallback_species(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod)
{
    const RemasterEmeraldEncounterAreaInfo *info =
        r12_area_info(map, area);
    uint16_t seen[REMASTER_EMERALD_ENCOUNTER_MAX_ANCHORS];
    size_t seen_count = 0;
    size_t start = 0;
    size_t count = 0;
    size_t i;
    uint16_t choice = 0;

    if (runtime == 0 || info == 0)
        return 0;

    r12_anchor_slice(info, area, rod, &start, &count);

    for (i = 0; i < count; ++i) {
        const uint16_t species = info->anchors[start + i];

        if (species == 0
            || r12_seen_species(seen, seen_count, species))
            continue;

        if (seen_count < REMASTER_EMERALD_ENCOUNTER_MAX_ANCHORS)
            seen[seen_count++] = species;

        if (remaster_emerald_encounter_random(&runtime->rng)
                % seen_count == 0u)
            choice = species;
    }

    return choice;
}

uint16_t remaster_emerald_encounter_choose_species(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod,
    int consume_bag)
{
    const uint16_t habitat_mask =
        r12_inferred_habitat(map, area, rod);

    if (runtime == 0 || map == 0)
        return 0;

    if (habitat_mask == 0)
        return r12_fallback_species(
            runtime, map, area, rod);

    if (!consume_bag) {
        const uint16_t candidate_count =
            r12_habitat_candidate_count(habitat_mask);
        const uint8_t divisor =
            r12_pool_divisor(candidate_count);
        uint16_t eligible_count = 0;
        uint16_t choice = 0;
        uint16_t species;

        for (species = 1;
             (size_t)species
                < sizeof(kRemasterEncounterSpeciesHabitat)
                    / sizeof(kRemasterEncounterSpeciesHabitat[0]);
             ++species) {
            if (!r12_local_species(
                    map,
                    area,
                    rod,
                    habitat_mask,
                    divisor,
                    species))
                continue;

            eligible_count++;
            if (remaster_emerald_encounter_random(&runtime->rng)
                    % eligible_count == 0u)
                choice = species;
        }

        return choice != 0
            ? choice
            : r12_fallback_species(runtime, map, area, rod);
    }

    if (!r12_bag_key_matches(
            runtime, map, area, rod, habitat_mask)
        || runtime->species_bag_cursor
            >= runtime->species_bag_count) {
        if (!remaster_emerald_encounter_build_local_pool(
                runtime, map, area, rod))
            return r12_fallback_species(
                runtime, map, area, rod);
    }

    if (runtime->species_bag_count == 0)
        return r12_fallback_species(
            runtime, map, area, rod);

    {
        const uint16_t species =
            runtime->species_bag[runtime->species_bag_cursor++];
        runtime->last_species = species;
        return species;
    }
}

uint8_t remaster_emerald_encounter_choose_level(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save)
{
    uint16_t total_level = 0;
    uint16_t average;
    uint16_t min_level;
    uint16_t max_level;
    uint8_t eligible = 0;
    size_t i;

    if (runtime == 0 || save == 0)
        return REMASTER_EMERALD_ENCOUNTER_MIN_LEVEL;

    for (i = 0; i < REMASTER_EMERALD_PARTY_SIZE; ++i) {
        RemasterEmeraldPartyPokemon mon;

        memset(&mon, 0, sizeof(mon));
        if (!r12_load_party_mon(save, i, &mon))
            continue;
        if (r12_secure_is_egg(&mon.box))
            continue;
        if (mon.hp == 0)
            continue;

        total_level += mon.level;
        eligible++;
    }

    if (eligible == 0)
        return REMASTER_EMERALD_ENCOUNTER_MIN_LEVEL;

    average = (uint16_t)(
        (total_level + (uint16_t)(eligible / 2u))
        / eligible);

    if (average > REMASTER_EMERALD_ENCOUNTER_LEVEL_DELTA)
        min_level =
            average - REMASTER_EMERALD_ENCOUNTER_LEVEL_DELTA;
    else
        min_level = REMASTER_EMERALD_ENCOUNTER_MIN_LEVEL;

    if (min_level < REMASTER_EMERALD_ENCOUNTER_MIN_LEVEL)
        min_level = REMASTER_EMERALD_ENCOUNTER_MIN_LEVEL;

    max_level =
        average + REMASTER_EMERALD_ENCOUNTER_LEVEL_DELTA;
    if (max_level > REMASTER_EMERALD_ENCOUNTER_MAX_LEVEL)
        max_level = REMASTER_EMERALD_ENCOUNTER_MAX_LEVEL;

    return (uint8_t)(
        min_level
        + remaster_emerald_encounter_random(&runtime->rng)
            % (max_level - min_level + 1u));
}

uint16_t remaster_emerald_encounter_modified_rate(
    uint8_t base_rate,
    const RemasterEmeraldEncounterRateContext *context)
{
    uint32_t rate = (uint32_t)base_rate * 16u;

    if (context == 0) {
        if (rate > (uint32_t)REMASTER_EMERALD_ENCOUNTER_MAX_RATE)
            rate = (uint32_t)REMASTER_EMERALD_ENCOUNTER_MAX_RATE;
        return (uint16_t)rate;
    }

    if (context->biking)
        rate = rate * 80u / 100u;

    if (context->flute_up)
        rate += rate / 2u;
    else if (context->flute_down)
        rate /= 2u;

    if (context->cleanse_tag)
        rate = rate * 2u / 3u;

    if (!context->ignore_ability
        && !context->lead_is_egg) {
        switch (context->lead_ability) {
        case R12_ABILITY_STENCH:
            rate = context->battle_pyramid
                ? rate * 3u / 4u
                : rate / 2u;
            break;
        case R12_ABILITY_ILLUMINATE:
            rate *= 2u;
            break;
        case R12_ABILITY_WHITE_SMOKE:
            rate /= 2u;
            break;
        case R12_ABILITY_ARENA_TRAP:
            rate *= 2u;
            break;
        case R12_ABILITY_SAND_VEIL:
            if (context->weather == R12_WEATHER_SANDSTORM)
                rate /= 2u;
            break;
        default:
            break;
        }
    }

    if (rate > REMASTER_EMERALD_ENCOUNTER_MAX_RATE)
        rate = REMASTER_EMERALD_ENCOUNTER_MAX_RATE;

    return (uint16_t)rate;
}

int remaster_emerald_encounter_repel_allows(
    const RemasterEmeraldSave *save,
    uint8_t wild_level)
{
    uint16_t steps = 0;
    size_t i;

    if (save == 0)
        return 0;

    if (!remaster_emerald_var_get(
            save,
            R12_VAR_REPEL_STEP_COUNT,
            &steps))
        return 0;

    if (steps == 0)
        return 1;

    for (i = 0; i < REMASTER_EMERALD_PARTY_SIZE; ++i) {
        RemasterEmeraldPartyPokemon mon;

        memset(&mon, 0, sizeof(mon));
        if (!r12_load_party_mon(save, i, &mon))
            continue;
        if (mon.hp == 0 || r12_secure_is_egg(&mon.box))
            continue;

        return wild_level >= mon.level;
    }

    return 0;
}

int remaster_emerald_encounter_update_repel(
    RemasterEmeraldSave *save,
    int in_battle_pike,
    int in_battle_pyramid,
    int in_union_room,
    int *out_wore_off)
{
    uint16_t steps = 0;

    if (out_wore_off != 0)
        *out_wore_off = 0;

    if (save == 0)
        return 0;

    if (in_battle_pike || in_battle_pyramid || in_union_room)
        return 1;

    if (!remaster_emerald_var_get(
            save,
            R12_VAR_REPEL_STEP_COUNT,
            &steps))
        return 0;

    if (steps == 0)
        return 1;

    steps--;
    if (!remaster_emerald_var_set(
            save,
            R12_VAR_REPEL_STEP_COUNT,
            steps))
        return 0;

    if (steps == 0 && out_wore_off != 0)
        *out_wore_off = 1;

    return 1;
}

static uint8_t r12_pick_nature(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save)
{
    RemasterEmeraldPartyPokemon lead;
    int sanity_egg = 0;
    const uint8_t ability =
        r12_lead_ability(save, &lead, &sanity_egg);

    if (!sanity_egg
        && ability == R12_ABILITY_SYNCHRONIZE
        && remaster_emerald_encounter_random(&runtime->rng) % 2u == 0u)
        return remaster_emerald_box_pokemon_nature(&lead.box);

    return (uint8_t)(
        remaster_emerald_encounter_random(&runtime->rng) % 25u);
}

static uint32_t r12_choose_personality(
    RemasterEmeraldEncounterRuntime *runtime,
    uint16_t species,
    uint8_t nature,
    int force_gender,
    uint8_t desired_gender)
{
    uint32_t personality;
    uint32_t guard = 0;

    do {
        personality =
            remaster_emerald_encounter_random32(&runtime->rng);
        guard++;
        if (guard > UINT32_C(1000000))
            return personality;
    } while (
        (uint8_t)(personality % 25u) != nature
        || (force_gender
            && r12_gender_for_personality(species, personality)
                != desired_gender));

    return personality;
}

static int r12_apply_initial_moves(
    RemasterEmeraldBoxPokemon *pokemon,
    uint16_t species,
    uint8_t level)
{
    const RemasterEmeraldEncounterLearnsetSlice *slice;
    uint16_t current_moves[4] = {0};
    uint8_t current_pp[4] = {0};
    uint16_t i;

    if (pokemon == 0
        || (size_t)species
            >= sizeof(kRemasterEncounterLearnsetSlices)
                / sizeof(kRemasterEncounterLearnsetSlices[0]))
        return 0;

    slice = &kRemasterEncounterLearnsetSlices[species];

    for (i = 0; i < slice->count; ++i) {
        const RemasterEmeraldEncounterLearnsetEntry *entry =
            &kRemasterEncounterLearnsetEntries[slice->start + i];
        const RemasterEmeraldMoveInfo *move_info;
        size_t slot;
        int known = 0;

        if (entry->level > level)
            break;

        for (slot = 0; slot < 4u; ++slot) {
            if (current_moves[slot] == entry->move) {
                known = 1;
                break;
            }
        }
        if (known)
            continue;

        move_info = remaster_emerald_move_info(entry->move);
        if (move_info == 0)
            return 0;

        for (slot = 0; slot < 4u; ++slot) {
            if (current_moves[slot] == 0)
                break;
        }

        if (slot < 4u) {
            current_moves[slot] = entry->move;
            current_pp[slot] = move_info->pp;
        } else {
            current_moves[0] = current_moves[1];
            current_moves[1] = current_moves[2];
            current_moves[2] = current_moves[3];
            current_moves[3] = entry->move;
            current_pp[0] = current_pp[1];
            current_pp[1] = current_pp[2];
            current_pp[2] = current_pp[3];
            current_pp[3] = move_info->pp;
        }
    }

    for (i = 0; i < 4u; ++i) {
        if (!remaster_emerald_box_pokemon_set_move(
                pokemon,
                i,
                current_moves[i],
                current_pp[i]))
            return 0;
    }

    return 1;
}

static int r12_build_fixed_mon(
    const RemasterEmeraldSave *save,
    uint16_t species,
    uint8_t level,
    uint16_t region_map_section,
    uint32_t personality,
    const uint8_t ivs[6],
    RemasterEmeraldPartyPokemon *out_pokemon)
{
    const RemasterEmeraldSpeciesInfo *info;
    RemasterEmeraldCalculatedStats stats;
    uint8_t zero_evs[6] = {0};
    uint16_t origin;
    uint32_t trainer_id;
    uint8_t ability_num;
    size_t i;

    if (save == 0 || out_pokemon == 0 || ivs == 0)
        return 0;

    info = remaster_emerald_species_info(species);
    if (info == 0 || species == 0 || level == 0 || level > 100)
        return 0;

    memset(out_pokemon, 0, sizeof(*out_pokemon));

    trainer_id = r12_read32(save->save_block2 + R12_SB2_TRAINER_ID);
    out_pokemon->box.personality = personality;
    out_pokemon->box.ot_id = trainer_id;
    memcpy(
        out_pokemon->box.ot_name,
        save->save_block2,
        sizeof(out_pokemon->box.ot_name));
    out_pokemon->box.language = R12_LANGUAGE_ENGLISH;
    out_pokemon->box.header_flags = 0x02u;

    if (!remaster_emerald_box_pokemon_set_species(
            &out_pokemon->box, species)
        || !remaster_emerald_box_pokemon_set_experience(
            &out_pokemon->box,
            remaster_emerald_experience_for_level(
                info->growth_rate,
                level))
        || !remaster_emerald_box_pokemon_set_friendship(
            &out_pokemon->box,
            info->friendship))
        return 0;

    out_pokemon->box.substruct[3][1] =
        (uint8_t)region_map_section;
    origin = (uint16_t)(level & 0x7Fu);
    origin |= (uint16_t)(R12_VERSION_EMERALD & 0x0Fu) << 7u;
    origin |= (uint16_t)(R12_ITEM_POKE_BALL & 0x0Fu) << 11u;
    origin |= (uint16_t)(save->save_block2[R12_SB2_PLAYER_GENDER] & 1u)
        << 15u;
    r12_write16(&out_pokemon->box.substruct[3][2], origin);

    for (i = 0; i < 6u; ++i) {
        if (!remaster_emerald_box_pokemon_set_iv(
                &out_pokemon->box,
                i,
                ivs[i]))
            return 0;
    }

    ability_num =
        info->abilities[1] != 0
        ? (uint8_t)(personality & 1u)
        : 0u;
    if (!remaster_emerald_box_pokemon_set_ability_num(
            &out_pokemon->box,
            ability_num))
        return 0;

    if (!r12_apply_initial_moves(
            &out_pokemon->box,
            species,
            level))
        return 0;

    if (!remaster_emerald_calculate_stats(
            species,
            level,
            (uint8_t)(personality % 25u),
            ivs,
            zero_evs,
            &stats))
        return 0;

    out_pokemon->status = 0;
    out_pokemon->level = level;
    out_pokemon->mail = R12_MAIL_NONE;
    out_pokemon->hp = stats.hp;
    out_pokemon->max_hp = stats.hp;
    out_pokemon->attack = stats.attack;
    out_pokemon->defense = stats.defense;
    out_pokemon->speed = stats.speed;
    out_pokemon->sp_attack = stats.sp_attack;
    out_pokemon->sp_defense = stats.sp_defense;
    out_pokemon->box.checksum =
        remaster_emerald_box_pokemon_checksum(&out_pokemon->box);

    return 1;
}

int remaster_emerald_encounter_create_wild(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save,
    uint16_t species,
    uint8_t level,
    uint16_t region_map_section,
    RemasterEmeraldPartyPokemon *out_pokemon)
{
    RemasterEmeraldPartyPokemon lead;
    int lead_sanity_egg = 0;
    const uint8_t lead_ability =
        r12_lead_ability(save, &lead, &lead_sanity_egg);
    int force_gender = 0;
    uint8_t desired_gender = REMASTER_EMERALD_GENDER_GENDERLESS;
    uint8_t nature;
    uint32_t personality;
    uint16_t value;
    uint8_t ivs[6];

    if (runtime == 0 || save == 0 || out_pokemon == 0)
        return 0;

    if (r12_species_supports_cute_charm(species)
        && !lead_sanity_egg
        && lead_ability == R12_ABILITY_CUTE_CHARM
        && remaster_emerald_encounter_random(&runtime->rng) % 3u != 0u) {
        const uint16_t lead_species =
            remaster_emerald_box_pokemon_species(&lead.box);
        const uint8_t lead_gender =
            r12_gender_for_personality(
                lead_species,
                lead.box.personality);

        if (lead_gender == REMASTER_EMERALD_GENDER_FEMALE) {
            desired_gender = REMASTER_EMERALD_GENDER_MALE;
            force_gender = 1;
        } else if (lead_gender == REMASTER_EMERALD_GENDER_MALE) {
            desired_gender = REMASTER_EMERALD_GENDER_FEMALE;
            force_gender = 1;
        }
    }

    nature = r12_pick_nature(runtime, save);
    personality = r12_choose_personality(
        runtime,
        species,
        nature,
        force_gender,
        desired_gender);

    value = remaster_emerald_encounter_random(&runtime->rng);
    ivs[0] = (uint8_t)(value & 31u);
    ivs[1] = (uint8_t)((value >> 5u) & 31u);
    ivs[2] = (uint8_t)((value >> 10u) & 31u);

    value = remaster_emerald_encounter_random(&runtime->rng);
    ivs[3] = (uint8_t)(value & 31u);
    ivs[4] = (uint8_t)((value >> 5u) & 31u);
    ivs[5] = (uint8_t)((value >> 10u) & 31u);

    return r12_build_fixed_mon(
        save,
        species,
        level,
        region_map_section,
        personality,
        ivs,
        out_pokemon);
}

static void r12_result_reset(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldEncounterResult *out_result,
    uint8_t area,
    uint8_t rod)
{
    if (out_result == 0)
        return;

    memset(out_result, 0, sizeof(*out_result));
    out_result->area = area;
    out_result->rod = rod;
    if (runtime != 0)
        out_result->rng_calls_before = runtime->rng.calls;
}

static void r12_result_finalize(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldEncounterResult *out_result)
{
    if (runtime != 0 && out_result != 0)
        out_result->rng_calls_after = runtime->rng.calls;
}

static void r12_fill_result_mon(
    RemasterEmeraldEncounterResult *out_result)
{
    if (out_result == 0 || !out_result->occurred)
        return;

    out_result->species =
        remaster_emerald_box_pokemon_species(
            &out_result->pokemon.box);
    out_result->level = out_result->pokemon.level;
    out_result->nature =
        remaster_emerald_box_pokemon_nature(
            &out_result->pokemon.box);
    out_result->gender =
        r12_gender_for_personality(
            out_result->species,
            out_result->pokemon.box.personality);
    out_result->ability_num =
        remaster_emerald_box_pokemon_ability_num(
            &out_result->pokemon.box);
}

static int r12_keen_eye_allows(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save,
    uint8_t wild_level)
{
    RemasterEmeraldPartyPokemon lead;
    int sanity_egg = 0;
    const uint8_t ability =
        r12_lead_ability(save, &lead, &sanity_egg);

    if (sanity_egg)
        return 1;

    if ((ability == R12_ABILITY_KEEN_EYE
            || ability == R12_ABILITY_INTIMIDATE)
        && lead.level > 5u
        && wild_level <= (uint8_t)(lead.level - 5u)
        && remaster_emerald_encounter_random(&runtime->rng) % 2u == 0u)
        return 0;

    return 1;
}

static int r12_roamer_active(
    const RemasterEmeraldSave *save)
{
    return save != 0
        && save->save_block1[R12_SB1_ROAMER + 0x13] != 0;
}

static int r12_create_roamer(
    const RemasterEmeraldSave *save,
    RemasterEmeraldEncounterResult *out_result,
    uint16_t region_map_section)
{
    const uint8_t *raw;
    uint16_t species;
    uint8_t level;
    uint32_t packed_ivs;
    uint8_t ivs[6];
    uint32_t personality;
    size_t i;

    if (save == 0 || out_result == 0)
        return 0;

    raw = save->save_block1 + R12_SB1_ROAMER;
    packed_ivs = r12_read32(raw + 0x00);
    personality = r12_read32(raw + 0x04);
    species = r12_read16(raw + 0x08);
    level = raw[0x0C];

    for (i = 0; i < 6u; ++i)
        ivs[i] = (uint8_t)((packed_ivs >> (i * 5u)) & 31u);

    if (!r12_build_fixed_mon(
            save,
            species,
            level,
            region_map_section,
            personality,
            ivs,
            &out_result->pokemon))
        return 0;

    out_result->pokemon.hp = r12_read16(raw + 0x0A);
    out_result->pokemon.status = raw[0x0D];
    return 1;
}

static int r12_try_roamer(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    uint16_t region_map_section,
    RemasterEmeraldEncounterResult *out_result)
{
    const uint8_t *raw;
    uint8_t level;

    if (!r12_roamer_active(save)
        || !runtime->roamer_location_valid
        || runtime->roamer_map_group != map_group
        || runtime->roamer_map_num != map_num)
        return 0;

    if (remaster_emerald_encounter_random(&runtime->rng) % 4u != 0u)
        return 0;

    raw = save->save_block1 + R12_SB1_ROAMER;
    level = raw[0x0C];

    if (!remaster_emerald_encounter_repel_allows(save, level))
        return -1;

    if (!r12_create_roamer(
            save,
            out_result,
            region_map_section))
        return -1;

    out_result->occurred = 1;
    out_result->kind = REMASTER_EMERALD_ENCOUNTER_ROAMER;
    r12_fill_result_mon(out_result);
    return 1;
}

static int r12_try_outbreak(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    uint16_t region_map_section,
    RemasterEmeraldEncounterResult *out_result)
{
    uint8_t *raw = save->save_block1;
    uint16_t species;
    uint8_t level;
    uint8_t probability;
    size_t i;

    species = r12_read16(raw + R12_SB1_OUTBREAK_SPECIES);
    if (species == 0
        || raw[R12_SB1_OUTBREAK_MAP_NUM] != map_num
        || raw[R12_SB1_OUTBREAK_MAP_GROUP] != map_group)
        return 0;

    probability = raw[R12_SB1_OUTBREAK_PROBABILITY];
    if (remaster_emerald_encounter_random(&runtime->rng) % 100u
        >= probability)
        return 0;

    level = raw[R12_SB1_OUTBREAK_LEVEL];
    if (!remaster_emerald_encounter_repel_allows(save, level))
        return -1;

    if (!remaster_emerald_encounter_create_wild(
            runtime,
            save,
            species,
            level,
            region_map_section,
            &out_result->pokemon))
        return -1;

    for (i = 0; i < 4u; ++i) {
        const uint16_t move = r12_read16(
            raw + R12_SB1_OUTBREAK_MOVES + i * 2u);
        const RemasterEmeraldMoveInfo *move_info =
            remaster_emerald_move_info(move);
        const uint8_t pp =
            move_info != 0 ? move_info->pp : 0;

        if (!remaster_emerald_box_pokemon_set_move(
                &out_result->pokemon.box,
                i,
                move,
                pp))
            return -1;
    }

    out_result->pokemon.box.checksum =
        remaster_emerald_box_pokemon_checksum(
            &out_result->pokemon.box);
    out_result->occurred = 1;
    out_result->kind = REMASTER_EMERALD_ENCOUNTER_OUTBREAK;
    r12_fill_result_mon(out_result);
    return 1;
}

int remaster_emerald_encounter_generate_after_rate(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    uint8_t area,
    uint8_t rod,
    RemasterEmeraldEncounterResult *out_result)
{
    const RemasterEmeraldEncounterMapInfo *map;
    uint16_t species;
    uint8_t level;
    int special;

    if (runtime == 0 || save == 0 || out_result == 0)
        return 0;

    r12_result_reset(runtime, out_result, area, rod);

    map = remaster_emerald_encounter_map_find(
        map_group,
        map_num);
    if (map == 0) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    if (area == REMASTER_EMERALD_ENCOUNTER_AREA_LAND
        || area == REMASTER_EMERALD_ENCOUNTER_AREA_WATER) {
        special = r12_try_roamer(
            runtime,
            save,
            map_group,
            map_num,
            map->region_map_section,
            out_result);
        if (special != 0) {
            r12_result_finalize(runtime, out_result);
            return special > 0;
        }
    }

    if (area == REMASTER_EMERALD_ENCOUNTER_AREA_LAND) {
        special = r12_try_outbreak(
            runtime,
            save,
            map_group,
            map_num,
            map->region_map_section,
            out_result);
        if (special != 0) {
            r12_result_finalize(runtime, out_result);
            return special > 0;
        }
    }

    species = remaster_emerald_encounter_choose_species(
        runtime,
        map,
        area,
        rod,
        1);
    level = remaster_emerald_encounter_choose_level(
        runtime,
        save);

    if (species == 0) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    if (area != REMASTER_EMERALD_ENCOUNTER_AREA_FISHING) {
        if (!remaster_emerald_encounter_repel_allows(
                save,
                level)) {
            r12_result_finalize(runtime, out_result);
            return 0;
        }

        if (!r12_keen_eye_allows(
                runtime,
                save,
                level)) {
            r12_result_finalize(runtime, out_result);
            return 0;
        }
    }

    if (!remaster_emerald_encounter_create_wild(
            runtime,
            save,
            species,
            level,
            map->region_map_section,
            &out_result->pokemon)) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    out_result->occurred = 1;
    if (area == REMASTER_EMERALD_ENCOUNTER_AREA_ROCKS)
        out_result->kind = REMASTER_EMERALD_ENCOUNTER_ROCK_SMASH;
    else if (area == REMASTER_EMERALD_ENCOUNTER_AREA_FISHING)
        out_result->kind = REMASTER_EMERALD_ENCOUNTER_FISHING;
    else
        out_result->kind = REMASTER_EMERALD_ENCOUNTER_REGULAR;

    r12_fill_result_mon(out_result);
    r12_result_finalize(runtime, out_result);
    return 1;
}

static void r12_rate_context_from_save(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldEncounterStepContext *step,
    int ignore_ability,
    RemasterEmeraldEncounterRateContext *out)
{
    RemasterEmeraldPartyPokemon lead;
    int sanity_egg = 0;
    int flag = 0;

    memset(out, 0, sizeof(*out));
    out->biking = step != 0 ? step->biking : 0;
    out->ignore_ability = ignore_ability ? 1u : 0u;
    out->battle_pyramid =
        step != 0 ? step->battle_pyramid : 0;
    out->weather =
        save != 0 ? save->save_block1[R12_SB1_WEATHER] : 0;
    out->lead_ability =
        r12_lead_ability(save, &lead, &sanity_egg);
    out->lead_is_egg = sanity_egg ? 1u : 0u;

    if (remaster_emerald_flag_get(
            save,
            R12_FLAG_ENC_UP,
            &flag)
        && flag)
        out->flute_up = 1;

    flag = 0;
    if (remaster_emerald_flag_get(
            save,
            R12_FLAG_ENC_DOWN,
            &flag)
        && flag)
        out->flute_down = 1;

    if (remaster_emerald_box_pokemon_held_item(
            &lead.box) == R12_ITEM_CLEANSE_TAG)
        out->cleanse_tag = 1;
}

static int r12_rate_roll(
    RemasterEmeraldEncounterRuntime *runtime,
    uint16_t rate)
{
    return remaster_emerald_encounter_random(&runtime->rng)
        % REMASTER_EMERALD_ENCOUNTER_MAX_RATE
        < rate;
}

int remaster_emerald_encounter_step(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    const RemasterEmeraldEncounterStepContext *context,
    RemasterEmeraldEncounterResult *out_result)
{
    const RemasterEmeraldEncounterMapInfo *map;
    const RemasterEmeraldEncounterAreaInfo *area_info;
    RemasterEmeraldEncounterRateContext rate_context;
    uint8_t area;
    uint16_t rate;
    uint64_t calls_before;
    int sootopolis_flag = 0;
    int occurred;

    if (runtime == 0 || save == 0
        || context == 0 || out_result == 0)
        return 0;

    int wore_off = 0;
    uint8_t previous_behavior;

    area = remaster_emerald_encounter_area_from_behavior(
        context->current_behavior,
        context->surfing);
    r12_result_reset(
        runtime,
        out_result,
        area,
        REMASTER_EMERALD_ROD_NONE);
    calls_before = runtime->rng.calls;

    /*
     * ProcessPlayerFieldInput calls UpdateRepelCounter before
     * CheckStandardWildEncounter. If repel expires, its script consumes this
     * step and the wild check does not run.
     */
    if (!remaster_emerald_encounter_update_repel(
            save,
            context->battle_pike,
            context->battle_pyramid,
            context->union_room,
            &wore_off)) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }
    if (wore_off) {
        runtime->previous_behavior = context->current_behavior;
        runtime->previous_behavior_valid = 1;
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    /*
     * Vanilla CheckStandardWildEncounter grants four encounter-immune steps
     * after the counter is restarted.
     */
    if (runtime->wild_immunity_steps < 4u) {
        runtime->wild_immunity_steps++;
        runtime->previous_behavior = context->current_behavior;
        runtime->previous_behavior_valid = 1;
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    previous_behavior = runtime->previous_behavior_valid
        ? runtime->previous_behavior
        : context->previous_behavior;

    if (context->encounters_disabled
        || area == REMASTER_EMERALD_ENCOUNTER_AREA_NONE) {
        runtime->previous_behavior = context->current_behavior;
        runtime->previous_behavior_valid = 1;
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    map = remaster_emerald_encounter_map_find(
        context->map_group,
        context->map_num);
    if (map == 0) {
        /*
         * Battle Pike/Pyramid have separate source tables. R12 preserves
         * their explicit hook result without pretending they are normal
         * Hoenn map encounters; battle-specific generation remains R13.
         */
        if (context->battle_pike) {
            out_result->kind =
                REMASTER_EMERALD_ENCOUNTER_BATTLE_PIKE;
        } else if (context->battle_pyramid) {
            out_result->kind =
                REMASTER_EMERALD_ENCOUNTER_BATTLE_PYRAMID;
        }
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    area_info = r12_area_info(map, area);
    if (area_info == 0 || area_info->anchor_count == 0) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    if (area == REMASTER_EMERALD_ENCOUNTER_AREA_WATER
        && context->map_group == R12_MAP_SOOTOPOLIS_GROUP
        && context->map_num == R12_MAP_SOOTOPOLIS_NUM
        && remaster_emerald_flag_get(
            save,
            R12_FLAG_SOOTOPOLIS_LEGENDARIES,
            &sootopolis_flag)
        && sootopolis_flag) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    if (previous_behavior != context->current_behavior
        && remaster_emerald_encounter_random(&runtime->rng)
            % 100u >= 60u) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    r12_rate_context_from_save(
        save,
        context,
        0,
        &rate_context);
    rate = remaster_emerald_encounter_modified_rate(
        area_info->encounter_rate,
        &rate_context);
    out_result->modified_rate = rate;

    if (!r12_rate_roll(runtime, rate)) {
        runtime->previous_behavior = context->current_behavior;
        runtime->previous_behavior_valid = 1;
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    occurred = remaster_emerald_encounter_generate_after_rate(
        runtime,
        save,
        context->map_group,
        context->map_num,
        area,
        REMASTER_EMERALD_ROD_NONE,
        out_result);

    out_result->modified_rate = rate;
    out_result->rng_calls_before = calls_before;
    runtime->previous_behavior = context->current_behavior;
    runtime->previous_behavior_valid = 1;
    if (occurred)
        runtime->wild_immunity_steps = 0;
    r12_result_finalize(runtime, out_result);
    return occurred;
}

int remaster_emerald_encounter_rock_smash(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    RemasterEmeraldEncounterResult *out_result)
{
    const RemasterEmeraldEncounterMapInfo *map;
    RemasterEmeraldEncounterRateContext rate_context;
    RemasterEmeraldEncounterStepContext step;
    uint16_t rate;
    uint64_t calls_before;
    int occurred;

    if (runtime == 0 || save == 0 || out_result == 0)
        return 0;

    r12_result_reset(
        runtime,
        out_result,
        REMASTER_EMERALD_ENCOUNTER_AREA_ROCKS,
        REMASTER_EMERALD_ROD_NONE);
    calls_before = runtime->rng.calls;

    map = remaster_emerald_encounter_map_find(
        map_group,
        map_num);
    if (map == 0 || map->rocks.anchor_count == 0) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    memset(&step, 0, sizeof(step));
    r12_rate_context_from_save(
        save,
        &step,
        1,
        &rate_context);
    rate = remaster_emerald_encounter_modified_rate(
        map->rocks.encounter_rate,
        &rate_context);
    out_result->modified_rate = rate;

    if (!r12_rate_roll(runtime, rate)) {
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    occurred = remaster_emerald_encounter_generate_after_rate(
        runtime,
        save,
        map_group,
        map_num,
        REMASTER_EMERALD_ENCOUNTER_AREA_ROCKS,
        REMASTER_EMERALD_ROD_NONE,
        out_result);
    out_result->modified_rate = rate;
    out_result->rng_calls_before = calls_before;
    r12_result_finalize(runtime, out_result);
    return occurred;
}

int remaster_emerald_encounter_fishing(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    uint8_t rod,
    RemasterEmeraldEncounterResult *out_result)
{
    const RemasterEmeraldEncounterMapInfo *map;

    if (runtime == 0 || save == 0 || out_result == 0)
        return 0;

    map = remaster_emerald_encounter_map_find(
        map_group,
        map_num);
    if (map == 0 || map->fishing.anchor_count == 0) {
        r12_result_reset(
            runtime,
            out_result,
            REMASTER_EMERALD_ENCOUNTER_AREA_FISHING,
            rod);
        r12_result_finalize(runtime, out_result);
        return 0;
    }

    return remaster_emerald_encounter_generate_after_rate(
        runtime,
        save,
        map_group,
        map_num,
        REMASTER_EMERALD_ENCOUNTER_AREA_FISHING,
        rod,
        out_result);
}

void remaster_emerald_encounter_roamer_set_location(
    RemasterEmeraldEncounterRuntime *runtime,
    uint8_t map_group,
    uint8_t map_num)
{
    if (runtime == 0)
        return;

    runtime->roamer_map_group = map_group;
    runtime->roamer_map_num = map_num;
    runtime->roamer_location_valid = 1;
}

int remaster_emerald_encounter_roamer_move(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save)
{
    const uint8_t current_group =
        save != 0 ? save->save_block1[0x04] : 0;
    const uint8_t current_num =
        save != 0 ? save->save_block1[0x05] : 0;
    const int active = r12_roamer_active(save);
    size_t row_count =
        sizeof(kRemasterRoamerLocations)
        / sizeof(kRemasterRoamerLocations[0]);
    size_t row;

    if (runtime == 0 || save == 0 || row_count == 0)
        return 0;

    runtime->roamer_location_history[2][0] =
        runtime->roamer_location_history[1][0];
    runtime->roamer_location_history[2][1] =
        runtime->roamer_location_history[1][1];
    runtime->roamer_location_history[1][0] =
        runtime->roamer_location_history[0][0];
    runtime->roamer_location_history[1][1] =
        runtime->roamer_location_history[0][1];
    runtime->roamer_location_history[0][0] = current_group;
    runtime->roamer_location_history[0][1] = current_num;

    if (!runtime->roamer_location_valid && active) {
        row = remaster_emerald_encounter_random(&runtime->rng)
            % row_count;
        remaster_emerald_encounter_roamer_set_location(
            runtime,
            0,
            kRemasterRoamerLocations[row][0]);
        return 1;
    }

    if (remaster_emerald_encounter_random(&runtime->rng) % 16u == 0u) {
        if (!active)
            return 1;

        for (;;) {
            row = remaster_emerald_encounter_random(&runtime->rng)
                % row_count;
            if (kRemasterRoamerLocations[row][0]
                != runtime->roamer_map_num) {
                runtime->roamer_map_group = 0;
                runtime->roamer_map_num =
                    kRemasterRoamerLocations[row][0];
                return 1;
            }
        }
    }

    if (!active)
        return 1;

    for (row = 0; row < row_count; ++row) {
        if (kRemasterRoamerLocations[row][0]
            == runtime->roamer_map_num) {
            for (;;) {
                const size_t slot =
                    1u
                    + remaster_emerald_encounter_random(&runtime->rng)
                        % 5u;
                const uint8_t map_num =
                    kRemasterRoamerLocations[row][slot];

                if (map_num == 0xFFu)
                    continue;
                if (runtime->roamer_location_history[2][0] == 0u
                    && runtime->roamer_location_history[2][1]
                        == map_num)
                    continue;

                runtime->roamer_map_group = 0;
                runtime->roamer_map_num = map_num;
                return 1;
            }
        }
    }

    return 1;
}
