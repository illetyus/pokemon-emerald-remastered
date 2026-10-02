#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterBattlePresentationSubsystem.h"
#include "RemasterBattleStage.generated.h"

class USceneComponent;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterBattleStage : public AActor
{
    GENERATED_BODY()

public:
    ARemasterBattleStage();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> PlayerAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> OpponentAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> CameraAnchor;

    UFUNCTION()
    void HandlePresentationEvent(
        const FRemasterBattlePresentationEvent& Event);

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Battle")
    void BP_HandlePresentationEvent(
        const FRemasterBattlePresentationEvent& Event);
};
