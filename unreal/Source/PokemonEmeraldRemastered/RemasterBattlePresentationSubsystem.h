#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterBattleRead.h"
#include "RemasterBattlePresentationSubsystem.generated.h"

UENUM(BlueprintType)
enum class ERemasterBattlePresentationEventType : uint8
{
    BattleStarted, BattlerEntered, MoveStarted, Hit, CriticalHit, StatusChanged,
    HpChanged, WeatherChanged, BattlerFainted, BattleEnded, OtherCoreEvent
};
USTRUCT(BlueprintType)
struct FRemasterBattleBattlerView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bActive = false;
    UPROPERTY(BlueprintReadOnly) bool bFainted = false;
    UPROPERTY(BlueprintReadOnly) int32 CoreSpecies = 0;
    UPROPERTY(BlueprintReadOnly) int32 NationalDex = 0;
    UPROPERTY(BlueprintReadOnly) FText Name;
    UPROPERTY(BlueprintReadOnly) FString AssetKey;
    UPROPERTY(BlueprintReadOnly) FString Form;
    UPROPERTY(BlueprintReadOnly) int32 Hp = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxHp = 0;
    UPROPERTY(BlueprintReadOnly) int32 Level = 0;
    UPROPERTY(BlueprintReadOnly) int32 Side = 0;
    UPROPERTY(BlueprintReadOnly) int32 DisplayHeightCm = 80;
    UPROPERTY(BlueprintReadOnly) FVector Position = FVector::ZeroVector;
};
USTRUCT(BlueprintType)
struct FRemasterBattleSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TArray<FRemasterBattleBattlerView> Battlers;
    UPROPERTY(BlueprintReadOnly) bool bTrainerBattle = false;
    UPROPERTY(BlueprintReadOnly) bool bEnded = false;
    UPROPERTY(BlueprintReadOnly) int32 Outcome = 0;
    UPROPERTY(BlueprintReadOnly) int32 Weather = 0;
    UPROPERTY(BlueprintReadOnly) int32 TrainerId = 0;
    UPROPERTY(BlueprintReadOnly) FString TrainerIdentity;
    UPROPERTY(BlueprintReadOnly) FText TrainerName;
};
USTRUCT(BlueprintType)
struct FRemasterBattlePresentationEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) ERemasterBattlePresentationEventType Type = ERemasterBattlePresentationEventType::OtherCoreEvent;
    UPROPERTY(BlueprintReadOnly) int32 RawKind = 0;
    UPROPERTY(BlueprintReadOnly) int32 SourceBattler = -1;
    UPROPERTY(BlueprintReadOnly) int32 TargetBattler = -1;
    UPROPERTY(BlueprintReadOnly) int32 MoveId = 0;
    UPROPERTY(BlueprintReadOnly) int32 Value = 0;
    UPROPERTY(BlueprintReadOnly) int64 Aux = 0;
    UPROPERTY(BlueprintReadOnly) FName Tag;
    UPROPERTY(BlueprintReadOnly) FName VfxIdentity;
    UPROPERTY(BlueprintReadOnly) int64 Epoch = 0;
    UPROPERTY(BlueprintReadOnly) int64 Token = 0;
    UPROPERTY(BlueprintReadOnly) int32 MotionId = 0;
    UPROPERTY(BlueprintReadOnly) int32 Actor = -1;
    UPROPERTY(BlueprintReadOnly) int32 ShotId = 0;
    UPROPERTY(BlueprintReadOnly) float Duration = 0.1f;
    UPROPERTY(BlueprintReadOnly) FVector CameraPosition = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector CameraTarget = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float CameraFov = 45.0f;
    UPROPERTY(BlueprintReadOnly) FRemasterBattleSnapshot Snapshot;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRemasterBattlePresentationDelegate, const FRemasterBattlePresentationEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRemasterBattleSnapshotDelegate, const FRemasterBattleSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRemasterBattleVisualReset);
UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterBattlePresentationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // Authoritative host only: no Blueprint API can fabricate presentation events.
    bool BeginCoreBattle(uint64 Epoch);
    bool PublishCoreBatch(uint64 Epoch, uint64 BatchSerial, const RemasterEmeraldBattleState& Core, SIZE_T FirstEvent = 0);
    bool EndCoreBattle(uint64 Epoch, const RemasterEmeraldBattleState& Core);
    bool ResyncCoreSnapshot(uint64 Epoch, uint64 BatchSerial, const RemasterEmeraldBattleState& Core);
    UPROPERTY(BlueprintAssignable) FRemasterBattlePresentationDelegate OnBattlePresentationEvent;
    UPROPERTY(BlueprintAssignable) FRemasterBattleSnapshotDelegate OnCoreSnapshot;
    UPROPERTY(BlueprintAssignable) FRemasterBattleVisualReset OnPresentationReset;
    UFUNCTION(BlueprintCallable, Category="Remaster|Battle Presentation")
    bool CompletePresentation(int64 Epoch, int64 Token);
    UFUNCTION(BlueprintCallable, Category="Remaster|Battle Presentation") void Clear();
    UFUNCTION(BlueprintPure, Category="Remaster|Battle Presentation")
    FRemasterBattleSnapshot GetLatestSnapshot() const { return Latest; }
    UFUNCTION(BlueprintPure, Category="Remaster|Battle Presentation")
    bool GetActiveCue(FRemasterBattlePresentationEvent& OutEvent) const;
private:
    void Pump();
    RemasterBattlePresentation::Feed Feed;
    UPROPERTY() FRemasterBattleSnapshot Latest;
    UPROPERTY() FRemasterBattlePresentationEvent ActiveCue;
    UPROPERTY() TArray<FRemasterBattlePresentationEvent> History;
    bool bHasCue = false;
};
