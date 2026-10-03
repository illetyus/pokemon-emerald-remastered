#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterCameraRig.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterCameraRig : public AActor
{
    GENERATED_BODY()

public:
    ARemasterCameraRig();
    virtual void Tick(float DeltaSeconds) override;

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

    UPROPERTY(EditAnywhere, Category="Remaster|Camera")
    float LookAheadDistance = 80.0f;

private:
    TWeakObjectPtr<AActor> FollowTarget;
};
