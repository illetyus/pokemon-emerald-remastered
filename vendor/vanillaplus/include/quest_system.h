#ifndef GUARD_QUEST_SYSTEM_H
#define GUARD_QUEST_SYSTEM_H

enum QuestTargetType
{
    QUEST_TARGET_NONE,
    QUEST_TARGET_REGION,
    QUEST_TARGET_MAP,
    QUEST_TARGET_OBJECT_EVENT,
    QUEST_TARGET_COORDINATE,
};

enum QuestConditionType
{
    QUEST_CONDITION_FLAG_SET,
    QUEST_CONDITION_FLAG_CLEAR,
    QUEST_CONDITION_VAR_EQ,
    QUEST_CONDITION_VAR_NE,
    QUEST_CONDITION_VAR_GE,
    QUEST_CONDITION_VAR_LT,
};

struct QuestCondition
{
    u8 type;
    u16 id;
    u16 value;
};

#define QUEST_MAX_CONDITIONS 4

struct QuestObjective
{
    u16 id;
    const u8 *title;
    const u8 *description;
    u16 mapSecId;
    u8 targetType;
    u8 mapGroup;
    u8 mapNum;
    u8 localId;
    s16 x;
    s16 y;
    u8 activationCount;
    u8 completionCount;
    struct QuestCondition activation[QUEST_MAX_CONDITIONS];
    struct QuestCondition completion[QUEST_MAX_CONDITIONS];
};

const struct QuestObjective *Quest_GetActiveObjective(void);
bool8 Quest_HasActiveObjective(void);
u16 Quest_GetActiveMapSecId(void);
bool8 Quest_TargetMatchesCurrentMap(const struct QuestObjective *objective);
void Quest_UpdateLocalMarker(void);
void Quest_RemoveLocalMarker(void);

#endif // GUARD_QUEST_SYSTEM_H
