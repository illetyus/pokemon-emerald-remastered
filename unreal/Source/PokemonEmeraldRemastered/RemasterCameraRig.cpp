#include "RemasterCameraRig.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

ARemasterCameraRig::ARemasterCameraRig()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = 900.0f;
    SpringArm->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->ProjectionMode = ECameraProjectionMode::Perspective;
}

void ARemasterCameraRig::SetFollowTarget(AActor* Target)
{
    FollowTarget = Target;
}

void ARemasterCameraRig::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    AActor* Target = FollowTarget.Get();
    if (!Target)
    {
        return;
    }

    FVector Desired = Target->GetActorLocation();
    const FVector Velocity = Target->GetVelocity();

    if (!Velocity.IsNearlyZero())
    {
        Desired += Velocity.GetSafeNormal2D() * LookAheadDistance;
    }

    SetActorLocation(
        FMath::VInterpTo(
            GetActorLocation(),
            Desired,
            DeltaSeconds,
            FollowSpeed));
}
