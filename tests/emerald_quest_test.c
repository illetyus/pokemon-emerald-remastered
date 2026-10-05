#include "remaster/emerald_quest.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_quest_test: %s\n", message);
        return 0;
    }

    return 1;
}

static int set_condition_truth(
    RemasterEmeraldSave *save,
    const RemasterEmeraldQuestCondition *condition,
    int truth)
{
    uint16_t value;

    switch (condition->type) {
    case REMASTER_EMERALD_QUEST_CONDITION_FLAG_SET:
        return remaster_emerald_flag_set(save, condition->id, truth ? 1 : 0);

    case REMASTER_EMERALD_QUEST_CONDITION_FLAG_CLEAR:
        return remaster_emerald_flag_set(save, condition->id, truth ? 0 : 1);

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_EQ:
        value = truth
            ? condition->value
            : (uint16_t)(condition->value + 1u);
        return remaster_emerald_var_set(save, condition->id, value);

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_NE:
        value = truth
            ? (uint16_t)(condition->value + 1u)
            : condition->value;
        return remaster_emerald_var_set(save, condition->id, value);

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_GE:
        if (truth)
            value = condition->value;
        else if (condition->value > 0)
            value = (uint16_t)(condition->value - 1u);
        else
            return 0;
        return remaster_emerald_var_set(save, condition->id, value);

    case REMASTER_EMERALD_QUEST_CONDITION_VAR_LT:
        if (truth) {
            if (condition->value == 0)
                return 0;
            value = (uint16_t)(condition->value - 1u);
        } else {
            value = condition->value;
        }
        return remaster_emerald_var_set(save, condition->id, value);
    }

    return 0;
}

static int set_conditions_truth(
    RemasterEmeraldSave *save,
    const RemasterEmeraldQuestCondition *conditions,
    uint8_t count,
    int truth)
{
    uint8_t i;

    for (i = 0; i < count; ++i) {
        if (!set_condition_truth(save, &conditions[i], truth))
            return 0;
    }

    return 1;
}

static int build_state_for_objective(
    RemasterEmeraldSave *save,
    size_t objective_index)
{
    size_t i;
    const RemasterEmeraldQuestObjective *objective;

    memset(save, 0, sizeof(*save));

    for (i = 0; i < objective_index; ++i) {
        objective = remaster_emerald_quest_objective_at(i);
        if (objective == 0)
            return 0;

        if (!set_conditions_truth(
                save,
                objective->completion,
                objective->completion_count,
                1))
            return 0;
    }

    objective = remaster_emerald_quest_objective_at(objective_index);
    if (objective == 0)
        return 0;

    if (!set_conditions_truth(
            save,
            objective->activation,
            objective->activation_count,
            1))
        return 0;

    if (!set_conditions_truth(
            save,
            objective->completion,
            objective->completion_count,
            0))
        return 0;

    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    const RemasterEmeraldQuestObjective *objective;
    const RemasterEmeraldQuestObjective *active;
    RemasterEmeraldQuestRegionMarker marker;
    size_t i;

    memset(&save, 0, sizeof(save));
    memset(&marker, 0, sizeof(marker));

    if (!check(
            remaster_emerald_quest_objective_count() == 32u,
            "R10 main-story catalog must contain exactly 32 objectives"))
        return 1;

    objective = remaster_emerald_quest_objective_at(0);
    if (!check(
            objective != 0
            && objective->id == REMASTER_EMERALD_QUEST_MEET_RIVAL_ROUTE103
            && objective->target_type == REMASTER_EMERALD_QUEST_TARGET_REGION
            && objective->map_section_id == 0x12,
            "first objective identity/target mismatch"))
        return 1;

    if (!check(
            objective->region_marker.x == 4
            && objective->region_marker.y == 8
            && objective->region_marker.width == 4
            && objective->region_marker.height == 1,
            "Route 103 canonical region marker mismatch"))
        return 1;

    if (!check(
            remaster_emerald_quest_active(&save) == 0,
            "blank pre-starter state should have no active R10 objective"))
        return 1;

    /*
     * Every catalog entry must be independently reachable from a synthetic
     * story state assembled only from its declared completion/activation
     * conditions. This catches overlapping or unreachable objective rules.
     */
    for (i = 0; i < remaster_emerald_quest_objective_count(); ++i) {
        if (!check(
                build_state_for_objective(&save, i),
                "failed to build synthetic story state"))
            return 1;

        objective = remaster_emerald_quest_objective_at(i);
        active = remaster_emerald_quest_active(&save);

        if (!check(
                active == objective,
                "active resolver did not return canonical first active objective"))
            return 1;

        if (!check(
                remaster_emerald_quest_state(&save, objective)
                    == REMASTER_EMERALD_QUEST_PROGRESS_ACTIVE,
                "objective state should be active"))
            return 1;

        if (!check(
                remaster_emerald_quest_active_region_marker(&save, &marker)
                && marker.map_section_id == objective->map_section_id
                && marker.width > 0
                && marker.height > 0,
                "active objective region marker missing"))
            return 1;

        if (objective->target_type == REMASTER_EMERALD_QUEST_TARGET_MAP
            || objective->target_type == REMASTER_EMERALD_QUEST_TARGET_OBJECT_EVENT
            || objective->target_type == REMASTER_EMERALD_QUEST_TARGET_COORDINATE) {
            if (!check(
                    remaster_emerald_quest_target_matches_map(
                        objective,
                        objective->map_group,
                        objective->map_num),
                    "local/map target must match its canonical map"))
                return 1;
        } else {
            if (!check(
                    !remaster_emerald_quest_target_matches_map(objective, 0, 0),
                    "region-only target must not claim a local map match"))
                return 1;
        }
    }

    /*
     * Complete the full declared chain. FLAG_SYS_GAME_CLEAR is the final
     * completion condition, so no normal main-story objective may remain.
     */
    memset(&save, 0, sizeof(save));
    for (i = 0; i < remaster_emerald_quest_objective_count(); ++i) {
        objective = remaster_emerald_quest_objective_at(i);
        if (!check(
                objective != 0
                && set_conditions_truth(
                    &save,
                    objective->completion,
                    objective->completion_count,
                    1),
                "failed to apply objective completion"))
            return 1;
    }

    if (!check(
            remaster_emerald_quest_active(&save) == 0,
            "game-clear state must not expose a normal main-story objective"))
        return 1;

    puts("R10 derived quest progression regression passed.");
    return 0;
}
