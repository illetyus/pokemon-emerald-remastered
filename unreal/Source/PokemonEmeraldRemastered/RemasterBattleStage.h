#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterBattlePresentationSubsystem.h"
#include "RemasterBattleStage.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UTextRenderComponent;
class UCameraComponent;
class URemasterPokemonAssetSet;
class FStreamableHandle;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterBattleStage : public AActor
{
    GENERATED_BODY()

public:
    ARemasterBattleStage();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> PlayerAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> OpponentAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> CameraAnchor;

    UPROPERTY() TObjectPtr<UCameraComponent> BattleCamera;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Floor;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> EffectPulse;
    UPROPERTY() TObjectPtr<UTextRenderComponent> EventLabel;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> SlotRoots;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Placeholders;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> StaticVisuals;
    UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalVisuals;
    UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Labels;
    UPROPERTY() TObjectPtr<URemasterPokemonAssetSet> LocalAssets;
    UPROPERTY() FRemasterBattlePresentationEvent CurrentCue;
    UPROPERTY() FRemasterBattleSnapshot Snapshot;
    UPROPERTY() TWeakObjectPtr<AActor> PreviousViewTarget;

    UFUNCTION() void HandleCoreSnapshot(const FRemasterBattleSnapshot& InSnapshot);
    UFUNCTION() void ResetPresentation();

    UFUNCTION()
    void HandlePresentationEvent(
        const FRemasterBattlePresentationEvent& Event);

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Battle")
    void BP_HandlePresentationEvent(
        const FRemasterBattlePresentationEvent& Event);

private:
    void ApplyIdentity(int32 Slot, const FString& Key, int32 FallbackHeight);
    void FinishAssetLoad(int32 Slot, uint64 Generation, const FString& Key);
    bool PlaySemantic(int32 Slot, FName Semantic, bool bLoop);
    void RestoreCamera();
    TSharedPtr<FStreamableHandle> PendingLoads[6];
    uint64 LoadGeneration[6]{};
    FString ModelKeys[6];
    bool bClipActive[6]{};
    bool bFaintPresented[4]{};
    double Elapsed = 0.0;
    bool bHasCue = false, bCameraOwned = false;
};
