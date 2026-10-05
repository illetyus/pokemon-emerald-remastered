#ifndef REMASTER_EMERALD_QUEST_H
#define REMASTER_EMERALD_QUEST_H

#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_QUEST_MAX_CONDITIONS = 4
};

typedef enum RemasterEmeraldQuestId {
    REMASTER_EMERALD_QUEST_MEET_RIVAL_ROUTE103 = 1,
    REMASTER_EMERALD_QUEST_RETURN_TO_BIRCH,
    REMASTER_EMERALD_QUEST_VISIT_PETALBURG_GYM,
    REMASTER_EMERALD_QUEST_CHALLENGE_ROXANNE,
    REMASTER_EMERALD_QUEST_FIND_DEVON_THIEF,
    REMASTER_EMERALD_QUEST_RECOVER_DEVON_GOODS,
    REMASTER_EMERALD_QUEST_RETURN_DEVON_GOODS,
    REMASTER_EMERALD_QUEST_DELIVER_STEVEN_LETTER,
    REMASTER_EMERALD_QUEST_DELIVER_DEVON_PACKAGE,
    REMASTER_EMERALD_QUEST_CHALLENGE_BRAWLY,
    REMASTER_EMERALD_QUEST_CHALLENGE_WATTSON,
    REMASTER_EMERALD_QUEST_GO_METEOR_FALLS,
    REMASTER_EMERALD_QUEST_STOP_MT_CHIMNEY,
    REMASTER_EMERALD_QUEST_CHALLENGE_FLANNERY,
    REMASTER_EMERALD_QUEST_CHALLENGE_NORMAN,
    REMASTER_EMERALD_QUEST_CLEAR_WEATHER_INSTITUTE,
    REMASTER_EMERALD_QUEST_GET_DEVON_SCOPE,
    REMASTER_EMERALD_QUEST_CHALLENGE_WINONA,
    REMASTER_EMERALD_QUEST_CLIMB_MT_PYRE,
    REMASTER_EMERALD_QUEST_ENTER_MAGMA_HIDEOUT,
    REMASTER_EMERALD_QUEST_GO_SLATEPORT_HARBOR,
    REMASTER_EMERALD_QUEST_CLEAR_AQUA_HIDEOUT,
    REMASTER_EMERALD_QUEST_CHALLENGE_TATE_LIZA,
    REMASTER_EMERALD_QUEST_DEFEND_SPACE_CENTER,
    REMASTER_EMERALD_QUEST_GET_DIVE_FROM_STEVEN,
    REMASTER_EMERALD_QUEST_ENTER_SEAFLOOR_CAVERN,
    REMASTER_EMERALD_QUEST_GO_SOOTOPOLIS,
    REMASTER_EMERALD_QUEST_WAKE_RAYQUAZA,
    REMASTER_EMERALD_QUEST_RETURN_SOOTOPOLIS,
    REMASTER_EMERALD_QUEST_CHALLENGE_JUAN,
    REMASTER_EMERALD_QUEST_CROSS_VICTORY_ROAD,
    REMASTER_EMERALD_QUEST_CHALLENGE_POKEMON_LEAGUE
} RemasterEmeraldQuestId;

typedef enum RemasterEmeraldQuestTargetType {
    REMASTER_EMERALD_QUEST_TARGET_NONE = 0,
    REMASTER_EMERALD_QUEST_TARGET_REGION,
    REMASTER_EMERALD_QUEST_TARGET_MAP,
    REMASTER_EMERALD_QUEST_TARGET_OBJECT_EVENT,
    REMASTER_EMERALD_QUEST_TARGET_COORDINATE
} RemasterEmeraldQuestTargetType;

typedef enum RemasterEmeraldQuestConditionType {
    REMASTER_EMERALD_QUEST_CONDITION_FLAG_SET = 0,
    REMASTER_EMERALD_QUEST_CONDITION_FLAG_CLEAR,
    REMASTER_EMERALD_QUEST_CONDITION_VAR_EQ,
    REMASTER_EMERALD_QUEST_CONDITION_VAR_NE,
    REMASTER_EMERALD_QUEST_CONDITION_VAR_GE,
    REMASTER_EMERALD_QUEST_CONDITION_VAR_LT
} RemasterEmeraldQuestConditionType;

typedef enum RemasterEmeraldQuestProgress {
    REMASTER_EMERALD_QUEST_PROGRESS_LOCKED = 0,
    REMASTER_EMERALD_QUEST_PROGRESS_ACTIVE,
    REMASTER_EMERALD_QUEST_PROGRESS_COMPLETED
} RemasterEmeraldQuestProgress;

typedef struct RemasterEmeraldQuestCondition {
    uint8_t type;
    uint16_t id;
    uint16_t value;
} RemasterEmeraldQuestCondition;

typedef struct RemasterEmeraldQuestRegionMarker {
    uint16_t map_section_id;
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
} RemasterEmeraldQuestRegionMarker;

typedef struct RemasterEmeraldQuestObjective {
    uint16_t id;
    const char *key;
    const char *title;
    const char *description;

    uint16_t map_section_id;
    uint8_t target_type;
    uint8_t map_group;
    uint8_t map_num;
    uint8_t local_id;
    int16_t x;
    int16_t y;

    RemasterEmeraldQuestRegionMarker region_marker;

    uint8_t activation_count;
    uint8_t completion_count;
    RemasterEmeraldQuestCondition
        activation[REMASTER_EMERALD_QUEST_MAX_CONDITIONS];
    RemasterEmeraldQuestCondition
        completion[REMASTER_EMERALD_QUEST_MAX_CONDITIONS];
} RemasterEmeraldQuestObjective;

size_t remaster_emerald_quest_objective_count(void);

const RemasterEmeraldQuestObjective *remaster_emerald_quest_objective_at(
    size_t index);

const RemasterEmeraldQuestObjective *remaster_emerald_quest_objective_by_id(
    uint16_t id);

RemasterEmeraldQuestProgress remaster_emerald_quest_state(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldQuestObjective *objective);

const RemasterEmeraldQuestObjective *remaster_emerald_quest_active(
    const RemasterEmeraldSave *save);

int remaster_emerald_quest_target_matches_map(
    const RemasterEmeraldQuestObjective *objective,
    uint8_t map_group,
    uint8_t map_num);

int remaster_emerald_quest_active_region_marker(
    const RemasterEmeraldSave *save,
    RemasterEmeraldQuestRegionMarker *out_marker);

#ifdef __cplusplus
}
#endif

#endif
