#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterNavigationSubsystem.generated.h"

UENUM(BlueprintType)
enum class ERemasterObjectiveTargetType : uint8
{
    None,
    Region,
    Map,
    ObjectEvent,
    Coordinate
};

USTRUCT(BlueprintType)
struct FRemasterQuestObjective
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 ObjectiveId = 0;

    UPROPERTY(BlueprintReadOnly)
    FName QuestId;

    UPROPERTY(BlueprintReadOnly)
    FText Title;

    UPROPERTY(BlueprintReadOnly)
    FText Description;

    UPROPERTY(BlueprintReadOnly)
    int32 MapSectionId = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 RegionMarkerX = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RegionMarkerY = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RegionMarkerWidth = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RegionMarkerHeight = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MapGroup = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 MapNum = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 LocalId = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TileX = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TileY = 0;

    UPROPERTY(BlueprintReadOnly)
    ERemasterObjectiveTargetType TargetType =
        ERemasterObjectiveTargetType::None;

    UPROPERTY(BlueprintReadOnly)
    bool bTargetMatchesCurrentMap = false;

    UPROPERTY(BlueprintReadOnly)
    bool bTargetObjectVisible = false;
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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable)
    FRemasterObjectiveChanged OnActiveObjectiveChanged;

    /*
     * Re-derives the active main-story objective from the authoritative
     * Emerald save flags/vars. This never writes story or quest state.
     */
    UFUNCTION(BlueprintCallable, Category="Remaster|Navigation")
    bool RefreshFromCore();

    UFUNCTION(BlueprintPure, Category="Remaster|Navigation")
    bool HasActiveObjective() const
    {
        return bHasObjective;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|Navigation")
    FRemasterQuestObjective GetActiveObjective() const
    {
        return ActiveObjective;
    }

private:
    UFUNCTION()
    void HandleGameplayMapChanged(
        int32 MapGroup,
        int32 MapNum,
        FString MapId);

    void ClearDerivedObjective();

    UPROPERTY()
    FRemasterQuestObjective ActiveObjective;

    bool bHasObjective = false;
};
