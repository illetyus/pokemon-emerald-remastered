#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterOverworldPawn.generated.h"

class ARemasterWorldActor;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterOverworldPawn : public APawn
{
    GENERATED_BODY()

public:
    ARemasterOverworldPawn();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Player")
    bool SyncFromAuthoritativeState();

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Player")
    bool ApplyAuthoritativeStep(
        const FRemasterPlayerStepResult& StepResult);

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> PresentationMesh;

    UPROPERTY(EditAnywhere, Category="Remaster|World|Player")
    float PlayerHeightOffset = 50.0f;

private:
    UFUNCTION()
    void HandleGameplayMapChanged(
        int32 MapGroup,
        int32 MapNum,
        FString MapId);

    ARemasterWorldActor* FindWorldRenderer() const;
    void ApplyFacing(int32 Direction);
};
