#include "remaster/emerald_quest.h"

#include "remaster/emerald_state.h"

#include <stddef.h>

#include "emerald_quest_catalog.inc"

static int quest_condition_met(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldQuestCondition *condition)
{
    int flag_value = 0;
    uint16_t var_value = 0;

    if (save == 0 || condition == 0)
        return 0;

    switch ((RemasterEmeraldQuestConditionType)condition->type) {
    case REMASTER_EMERALD_QUEST_CONDITION_FLAG_SET:
        return remaster_emerald_flag_get(
                   save,
                   condition->id,
                   &flag_value)
            && flag_value != 0;

    case REMASTER_EMERALD_QUEST_CONDITION_FLAG_CLEAR:
        return remaster_emerald_flag_get(
                   save,
                   condition->id,
                   &flag_value)
            && flag_value == 0;

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_EQ:
        return remaster_emerald_var_get(
                   save,
                   condition->id,
                   &var_value)
            && var_value == condition->value;

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_NE:
        return remaster_emerald_var_get(
                   save,
                   condition->id,
                   &var_value)
            && var_value != condition->value;

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_GE:
        return remaster_emerald_var_get(
                   save,
                   condition->id,
                   &var_value)
            && var_value >= condition->value;

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_LT:
        return remaster_emerald_var_get(
                   save,
                   condition->id,
                   &var_value)
            && var_value < condition->value;
    }

    return 0;
}

static int quest_conditions_met(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldQuestCondition *conditions,
    uint8_t count)
{
    uint8_t i;

    if (count > REMASTER_EMERALD_QUEST_MAX_CONDITIONS)
        return 0;

    for (i = 0; i < count; ++i) {
        if (!quest_condition_met(save, &conditions[i]))
            return 0;
    }

    return 1;
}

size_t remaster_emerald_quest_objective_count(void)
{
    return sizeof(kRemasterEmeraldQuestObjectives)
        / sizeof(kRemasterEmeraldQuestObjectives[0]);
}

const RemasterEmeraldQuestObjective *remaster_emerald_quest_objective_at(
    size_t index)
{
    if (index >= remaster_emerald_quest_objective_count())
        return 0;

    return &kRemasterEmeraldQuestObjectives[index];
}

const RemasterEmeraldQuestObjective *remaster_emerald_quest_objective_by_id(
    uint16_t id)
{
    size_t i;

    for (i = 0; i < remaster_emerald_quest_objective_count(); ++i) {
        if (kRemasterEmeraldQuestObjectives[i].id == id)
            return &kRemasterEmeraldQuestObjectives[i];
    }

    return 0;
}

RemasterEmeraldQuestProgress remaster_emerald_quest_state(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldQuestObjective *objective)
{
    if (save == 0 || objective == 0)
        return REMASTER_EMERALD_QUEST_PROGRESS_LOCKED;

    if (quest_conditions_met(
            save,
            objective->completion,
            objective->completion_count))
        return REMASTER_EMERALD_QUEST_PROGRESS_COMPLETED;

    if (quest_conditions_met(
            save,
            objective->activation,
            objective->activation_count))
        return REMASTER_EMERALD_QUEST_PROGRESS_ACTIVE;

    return REMASTER_EMERALD_QUEST_PROGRESS_LOCKED;
}

const RemasterEmeraldQuestObjective *remaster_emerald_quest_active(
    const RemasterEmeraldSave *save)
{
    size_t i;

    if (save == 0)
        return 0;

    for (i = 0; i < remaster_emerald_quest_objective_count(); ++i) {
        const RemasterEmeraldQuestObjective *objective =
            &kRemasterEmeraldQuestObjectives[i];

        if (remaster_emerald_quest_state(save, objective)
            == REMASTER_EMERALD_QUEST_PROGRESS_ACTIVE)
            return objective;
    }

    return 0;
}

int remaster_emerald_quest_target_matches_map(
    const RemasterEmeraldQuestObjective *objective,
    uint8_t map_group,
    uint8_t map_num)
{
    if (objective == 0)
        return 0;

    if (objective->target_type != REMASTER_EMERALD_QUEST_TARGET_MAP
        && objective->target_type
            != REMASTER_EMERALD_QUEST_TARGET_OBJECT_EVENT
        && objective->target_type
            != REMASTER_EMERALD_QUEST_TARGET_COORDINATE)
        return 0;

    return objective->map_group == map_group
        && objective->map_num == map_num;
}

int remaster_emerald_quest_active_region_marker(
    const RemasterEmeraldSave *save,
    RemasterEmeraldQuestRegionMarker *out_marker)
{
    const RemasterEmeraldQuestObjective *objective;

    if (save == 0 || out_marker == 0)
        return 0;

    objective = remaster_emerald_quest_active(save);
    if (objective == 0)
        return 0;

    *out_marker = objective->region_marker;
    return 1;
}
