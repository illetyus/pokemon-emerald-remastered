#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "RemasterCharacterImportPilot.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class URemasterCharacterVisualComponent;

// Development diagnostic scene only; does not implement locomotion or gameplay.
UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterCharacterImportPilotActor : public AActor
{
    GENERATED_BODY()
public:
    ARemasterCharacterImportPilotActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category="Import Pilot")
    int32 GraphicsId = 0;
    UPROPERTY(EditAnywhere, Category="Import Pilot")
    TSoftObjectPtr<UAnimSequence> PilotClip;
    UPROPERTY(EditAnywhere, Category="Import Pilot")
    bool bExpectFallback = false;
    UPROPERTY(EditAnywhere, Category="Import Pilot")
    bool bExpectMotion = true;

private:
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> Skeletal;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Placeholder;
    UPROPERTY()
    TObjectPtr<URemasterCharacterVisualComponent> Visual;
    UPROPERTY()
    TObjectPtr<UAnimSequence> LoadedClip;
    FVector InitialLocation = FVector::ZeroVector;
    FTransform FirstBonePose;
    uint64 InitialCoreHash = 0;
    float Elapsed = 0;
    float ClipElapsed = 0;
    float LastSampleTime = 0;
    int32 PoseChanges = 0;
    bool bStartedClip = false;
    bool bSampledPose = false;
    bool bObservedMotion = false;
    bool bVerifiedBoundary = false;
    bool bFailed = false;
};

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterCharacterImportPilotGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ARemasterCharacterImportPilotGameMode();
};
