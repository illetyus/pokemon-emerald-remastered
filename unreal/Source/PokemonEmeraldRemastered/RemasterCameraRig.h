#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterCameraRig.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;
class ARemasterWorldActor;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterCameraRig : public AActor
{
    GENERATED_BODY()

public:
    ARemasterCameraRig();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="Remaster|Camera")
    void SetFollowTarget(AActor* Target);

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(EditAnywhere, Category="Remaster|Camera")
    float FollowSpeed = 8.0f;

private:
    TWeakObjectPtr<AActor> FollowTarget;
    TWeakObjectPtr<ARemasterWorldActor> WorldRenderer;
    uint64 LastMapRevision = 0;
    bool bHasAuthoritativeTarget = false;
};
