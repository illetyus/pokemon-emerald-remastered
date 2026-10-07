#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterNpcPresentationWorld.generated.h"

class URemasterCharacterVisualComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterCharacterPresentationActor : public AActor
{
    GENERATED_BODY()
public:
    ARemasterCharacterPresentationActor();
    virtual void BeginPlay() override;
    void ApplyGraphicsId(int32 GraphicsId);
private:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Placeholder;
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> Skeletal;
    UPROPERTY()
    TObjectPtr<URemasterCharacterVisualComponent> Visual;
};

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterNpcPresentationWorld : public AActor
{
    GENERATED_BODY()
public:
    ARemasterNpcPresentationWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION()
    void HandleMapChanged(int32 MapGroup, int32 MapNum, FString MapId);
    void ClearInstances();
    FString InstanceMapId;
    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<ARemasterCharacterPresentationActor>> Instances;
};
