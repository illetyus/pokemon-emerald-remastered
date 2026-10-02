#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterBattlePresentationSubsystem.generated.h"

UENUM(BlueprintType)
enum class ERemasterBattlePresentationEventType : uint8
{
    BattleStarted,
    BattlerEntered,
    MoveStarted,
    Hit,
    CriticalHit,
    StatusChanged,
    HpChanged,
    WeatherChanged,
    BattlerFainted,
    BattleEnded
};

USTRUCT(BlueprintType)
struct FRemasterBattlePresentationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ERemasterBattlePresentationEventType Type =
        ERemasterBattlePresentationEventType::BattleStarted;

    UPROPERTY(BlueprintReadOnly)
    int32 SourceBattler = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 TargetBattler = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 Value = 0;

    UPROPERTY(BlueprintReadOnly)
    FName Tag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FRemasterBattlePresentationDelegate,
    const FRemasterBattlePresentationEvent&,
    Event);

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterBattlePresentationSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FRemasterBattlePresentationDelegate OnBattlePresentationEvent;

    UFUNCTION(BlueprintCallable)
    void Submit(const FRemasterBattlePresentationEvent& Event);

    UFUNCTION(BlueprintCallable)
    void Clear();

private:
    UPROPERTY()
    TArray<FRemasterBattlePresentationEvent> History;
};
