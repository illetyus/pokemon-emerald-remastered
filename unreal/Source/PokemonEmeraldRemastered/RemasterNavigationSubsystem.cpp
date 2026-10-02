#include "RemasterNavigationSubsystem.h"

void URemasterNavigationSubsystem::SetActiveObjective(
    const FRemasterQuestObjective& Objective)
{
    ActiveObjective = Objective;
    bHasObjective = true;
    OnActiveObjectiveChanged.Broadcast(ActiveObjective);
}

void URemasterNavigationSubsystem::ClearActiveObjective()
{
    ActiveObjective = FRemasterQuestObjective{};
    bHasObjective = false;
    OnActiveObjectiveChanged.Broadcast(ActiveObjective);
}
