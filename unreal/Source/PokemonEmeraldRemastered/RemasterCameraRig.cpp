#include "RemasterCameraRig.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "RemasterEnvironmentMath.h"
#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldActor.h"

ARemasterCameraRig::ARemasterCameraRig()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

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
    bHasAuthoritativeTarget = false;
}

void ARemasterCameraRig::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (auto* Renderer = WorldRenderer.Get()) Renderer->RestoreCameraOcclusion();
    Super::EndPlay(EndPlayReason);
}

void ARemasterCameraRig::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ARemasterWorldActor* Renderer = WorldRenderer.Get();
    if (!Renderer)
    {
        for (TActorIterator<ARemasterWorldActor> It(GetWorld()); It; ++It)
        {
            Renderer = *It;
            WorldRenderer = Renderer;
            break;
        }
    }
    UGameInstance* GI = GetGameInstance();
    const URemasterVanillaPlusSaveSubsystem* Save = GI
        ? GI->GetSubsystem<URemasterVanillaPlusSaveSubsystem>() : nullptr;
    FRemasterLegacyOverworldSnapshot Snapshot;
    if (!Renderer || !Save || !Save->GetOverworldSnapshot(Snapshot))
    {
        bHasAuthoritativeTarget = false;
        if (Renderer) Renderer->RestoreCameraOcclusion();
        return;
    }
    const FRemasterMapIR& Map = Renderer->GetLoadedMap();
    if (!Map.IsValid() || Map.GroupNum != Snapshot.MapGroup || Map.MapNum != Snapshot.MapNum)
    {
        bHasAuthoritativeTarget = false;
        Renderer->RestoreCameraOcclusion();
        return;
    }
    const auto Profile = remaster::environment::framing(remaster::environment::context(Map.MapTypeId));
    SpringArm->TargetArmLength = static_cast<float>(Profile.arm);
    SpringArm->SetRelativeRotation(FRotator(Profile.pitch, -45.0, 0.0));
    Camera->SetFieldOfView(static_cast<float>(Profile.fov));
    const FVector Desired = Renderer->TileToWorldLocation(Snapshot.PlayerX, Snapshot.PlayerY);
    const uint64 Revision = Renderer->GetPresentationRevision();
    // Snap on every authoritative map rebuild, including reload of the same map.
    const bool bSnap = !bHasAuthoritativeTarget || LastMapRevision != Revision;
    const double Alpha = remaster::environment::follow_alpha(FollowSpeed, DeltaSeconds);
    SetActorLocation(bSnap ? Desired : FMath::Lerp(GetActorLocation(), Desired, Alpha));
    LastMapRevision = Revision;
    bHasAuthoritativeTarget = true;
    const FVector CameraPosition = GetActorLocation()
        - SpringArm->GetComponentRotation().Vector() * SpringArm->TargetArmLength;
    Renderer->UpdateCameraOcclusion(CameraPosition, Desired + FVector(0, 0, 35));
}
