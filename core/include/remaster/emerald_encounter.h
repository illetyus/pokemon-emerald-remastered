#ifndef REMASTER_EMERALD_ENCOUNTER_H
#define REMASTER_EMERALD_ENCOUNTER_H

#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_ENCOUNTER_MAX_LOCAL_POOL = 64,
    REMASTER_EMERALD_ENCOUNTER_MAX_ANCHORS = 12,
    REMASTER_EMERALD_ENCOUNTER_MAX_LEVEL = 100,
    REMASTER_EMERALD_ENCOUNTER_MIN_LEVEL = 2,
    REMASTER_EMERALD_ENCOUNTER_LEVEL_DELTA = 15,
    REMASTER_EMERALD_ENCOUNTER_MAX_RATE = 2880
};

enum {
    REMASTER_EMERALD_HABITAT_CAVE = 1 << 0,
    REMASTER_EMERALD_HABITAT_FOREST = 1 << 1,
    REMASTER_EMERALD_HABITAT_GRASSLAND = 1 << 2,
    REMASTER_EMERALD_HABITAT_MOUNTAIN = 1 << 3,
    REMASTER_EMERALD_HABITAT_ROUGH_TERRAIN = 1 << 4,
    REMASTER_EMERALD_HABITAT_SEA = 1 << 5,
    REMASTER_EMERALD_HABITAT_URBAN = 1 << 6,
    REMASTER_EMERALD_HABITAT_WATERS_EDGE = 1 << 7
};

typedef enum RemasterEmeraldEncounterArea {
    REMASTER_EMERALD_ENCOUNTER_AREA_NONE = 0,
    REMASTER_EMERALD_ENCOUNTER_AREA_LAND = 1,
    REMASTER_EMERALD_ENCOUNTER_AREA_WATER = 2,
    REMASTER_EMERALD_ENCOUNTER_AREA_ROCKS = 3,
    REMASTER_EMERALD_ENCOUNTER_AREA_FISHING = 4
} RemasterEmeraldEncounterArea;

typedef enum RemasterEmeraldEncounterRod {
    REMASTER_EMERALD_ROD_NONE = 0,
    REMASTER_EMERALD_ROD_OLD = 1,
    REMASTER_EMERALD_ROD_GOOD = 2,
    REMASTER_EMERALD_ROD_SUPER = 3
} RemasterEmeraldEncounterRod;

typedef enum RemasterEmeraldEncounterKind {
    REMASTER_EMERALD_ENCOUNTER_NONE = 0,
    REMASTER_EMERALD_ENCOUNTER_REGULAR = 1,
    REMASTER_EMERALD_ENCOUNTER_ROAMER = 2,
    REMASTER_EMERALD_ENCOUNTER_OUTBREAK = 3,
    REMASTER_EMERALD_ENCOUNTER_ROCK_SMASH = 4,
    REMASTER_EMERALD_ENCOUNTER_FISHING = 5,
    REMASTER_EMERALD_ENCOUNTER_BATTLE_PIKE = 6,
    REMASTER_EMERALD_ENCOUNTER_BATTLE_PYRAMID = 7
} RemasterEmeraldEncounterKind;

typedef enum RemasterEmeraldGender {
    REMASTER_EMERALD_GENDER_MALE = 0,
    REMASTER_EMERALD_GENDER_FEMALE = 1,
    REMASTER_EMERALD_GENDER_GENDERLESS = 2
} RemasterEmeraldGender;

typedef struct RemasterEmeraldEncounterRng {
    uint32_t state;
    uint64_t calls;
} RemasterEmeraldEncounterRng;

typedef struct RemasterEmeraldEncounterAreaInfo {
    uint8_t encounter_rate;
    uint8_t anchor_count;
    uint16_t anchors[REMASTER_EMERALD_ENCOUNTER_MAX_ANCHORS];
} RemasterEmeraldEncounterAreaInfo;

typedef struct RemasterEmeraldEncounterMapInfo {
    uint8_t map_group;
    uint8_t map_num;
    uint16_t region_map_section;
    RemasterEmeraldEncounterAreaInfo land;
    RemasterEmeraldEncounterAreaInfo water;
    RemasterEmeraldEncounterAreaInfo rocks;
    RemasterEmeraldEncounterAreaInfo fishing;
} RemasterEmeraldEncounterMapInfo;

typedef struct RemasterEmeraldEncounterRateContext {
    uint8_t biking;
    uint8_t flute_up;
    uint8_t flute_down;
    uint8_t cleanse_tag;
    uint8_t ignore_ability;
    uint8_t lead_is_egg;
    uint8_t lead_ability;
    uint8_t weather;
    uint8_t battle_pyramid;
} RemasterEmeraldEncounterRateContext;

typedef struct RemasterEmeraldEncounterStepContext {
    uint8_t map_group;
    uint8_t map_num;
    uint8_t current_behavior;
    uint8_t previous_behavior;
    uint8_t surfing;
    uint8_t biking;
    uint8_t encounters_disabled;
    uint8_t battle_pike;
    uint8_t battle_pyramid;
    uint8_t union_room;
} RemasterEmeraldEncounterStepContext;

typedef struct RemasterEmeraldEncounterRuntime {
    RemasterEmeraldEncounterRng rng;

    uint16_t species_bag[REMASTER_EMERALD_ENCOUNTER_MAX_LOCAL_POOL];
    uint16_t species_bag_count;
    uint16_t species_bag_cursor;
    uint16_t species_bag_habitat_mask;
    uint16_t last_species;
    uint8_t species_bag_map_group;
    uint8_t species_bag_map_num;
    uint8_t species_bag_area;
    uint8_t species_bag_rod;
    uint8_t species_bag_valid;

    uint8_t roamer_map_group;
    uint8_t roamer_map_num;
    uint8_t roamer_location_valid;
    uint8_t roamer_location_history[3][2];

    uint8_t wild_immunity_steps;
    uint8_t previous_behavior;
    uint8_t previous_behavior_valid;
} RemasterEmeraldEncounterRuntime;

typedef struct RemasterEmeraldEncounterResult {
    uint8_t occurred;
    uint8_t repel_wore_off;
    uint8_t kind;
    uint8_t area;
    uint8_t rod;
    uint8_t level;
    uint8_t nature;
    uint8_t gender;
    uint8_t ability_num;
    uint16_t species;
    uint16_t modified_rate;
    uint64_t rng_calls_before;
    uint64_t rng_calls_after;
    RemasterEmeraldPartyPokemon pokemon;
} RemasterEmeraldEncounterResult;

void remaster_emerald_encounter_rng_seed(
    RemasterEmeraldEncounterRng *rng,
    uint32_t seed);
uint16_t remaster_emerald_encounter_random(
    RemasterEmeraldEncounterRng *rng);
uint32_t remaster_emerald_encounter_random32(
    RemasterEmeraldEncounterRng *rng);

void remaster_emerald_encounter_runtime_init(
    RemasterEmeraldEncounterRuntime *runtime,
    uint32_t seed);

void remaster_emerald_encounter_restart_immunity(
    RemasterEmeraldEncounterRuntime *runtime);

size_t remaster_emerald_encounter_map_count(void);
const RemasterEmeraldEncounterMapInfo *remaster_emerald_encounter_map_at(
    size_t index);
const RemasterEmeraldEncounterMapInfo *remaster_emerald_encounter_map_find(
    uint8_t map_group,
    uint8_t map_num);

uint16_t remaster_emerald_encounter_species_habitat(uint16_t species);
uint8_t remaster_emerald_encounter_area_from_behavior(
    uint8_t behavior,
    uint8_t surfing);

int remaster_emerald_encounter_build_local_pool(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod);

uint16_t remaster_emerald_encounter_choose_species(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldEncounterMapInfo *map,
    uint8_t area,
    uint8_t rod,
    int consume_bag);

uint8_t remaster_emerald_encounter_choose_level(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save);

uint16_t remaster_emerald_encounter_modified_rate(
    uint8_t base_rate,
    const RemasterEmeraldEncounterRateContext *context);

int remaster_emerald_encounter_repel_allows(
    const RemasterEmeraldSave *save,
    uint8_t wild_level);

int remaster_emerald_encounter_update_repel(
    RemasterEmeraldSave *save,
    int in_battle_pike,
    int in_battle_pyramid,
    int in_union_room,
    int *out_wore_off);

int remaster_emerald_encounter_create_wild(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save,
    uint16_t species,
    uint8_t level,
    uint16_t region_map_section,
    RemasterEmeraldPartyPokemon *out_pokemon);

int remaster_emerald_encounter_generate_after_rate(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    uint8_t area,
    uint8_t rod,
    RemasterEmeraldEncounterResult *out_result);

int remaster_emerald_encounter_step(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    const RemasterEmeraldEncounterStepContext *context,
    RemasterEmeraldEncounterResult *out_result);

int remaster_emerald_encounter_rock_smash(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    RemasterEmeraldEncounterResult *out_result);

int remaster_emerald_encounter_fishing(
    RemasterEmeraldEncounterRuntime *runtime,
    RemasterEmeraldSave *save,
    uint8_t map_group,
    uint8_t map_num,
    uint8_t rod,
    RemasterEmeraldEncounterResult *out_result);

void remaster_emerald_encounter_roamer_set_location(
    RemasterEmeraldEncounterRuntime *runtime,
    uint8_t map_group,
    uint8_t map_num);

int remaster_emerald_encounter_roamer_move(
    RemasterEmeraldEncounterRuntime *runtime,
    const RemasterEmeraldSave *save);

#ifdef __cplusplus
}
#endif

#endif
