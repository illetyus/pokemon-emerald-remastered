#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterNavigationSubsystem.generated.h"

UENUM(BlueprintType)
enum class ERemasterObjectiveTargetType : uint8
{
    None,
    MapPosition,
    Npc,
    Object,
    Region
};

USTRUCT(BlueprintType)
struct FRemasterQuestObjective
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName QuestId;

    UPROPERTY(BlueprintReadOnly)
    FText Title;

    UPROPERTY(BlueprintReadOnly)
    FText Description;

    UPROPERTY(BlueprintReadOnly)
    FName MapId;

    UPROPERTY(BlueprintReadOnly)
    int32 TileX = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TileY = 0;

    UPROPERTY(BlueprintReadOnly)
    ERemasterObjectiveTargetType TargetType =
        ERemasterObjectiveTargetType::None;

    UPROPERTY(BlueprintReadOnly)
    FName TargetId;

    UPROPERTY(BlueprintReadOnly)
    bool bCompleted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FRemasterObjectiveChanged,
    const FRemasterQuestObjective&,
    Objective);

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterNavigationSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FRemasterObjectiveChanged OnActiveObjectiveChanged;

    UFUNCTION(BlueprintCallable)
    void SetActiveObjective(const FRemasterQuestObjective& Objective);

    UFUNCTION(BlueprintCallable)
    void ClearActiveObjective();

    UFUNCTION(BlueprintPure)
    bool HasActiveObjective() const
    {
        return bHasObjective;
    }

    UFUNCTION(BlueprintPure)
    FRemasterQuestObjective GetActiveObjective() const
    {
        return ActiveObjective;
    }

private:
    UPROPERTY()
    FRemasterQuestObjective ActiveObjective;

    bool bHasObjective = false;
};
