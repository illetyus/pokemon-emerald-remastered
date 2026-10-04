#include "RemasterOverworldPawn.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldActor.h"
#include "UObject/ConstructorHelpers.h"

extern "C"
{
#include "remaster/emerald_map.h"
}

ARemasterOverworldPawn::ARemasterOverworldPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    PresentationMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("PresentationMesh"));
    PresentationMesh->SetupAttachment(SceneRoot);
    PresentationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PresentationMesh->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    if (CylinderMesh.Succeeded())
    {
        PresentationMesh->SetStaticMesh(CylinderMesh.Object);
        PresentationMesh->SetRelativeScale3D(
            FVector(0.28f, 0.28f, 0.7f));
    }
}

void ARemasterOverworldPawn::BeginPlay()
{
    Super::BeginPlay();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterWorldGameplaySubsystem* Gameplay =
                GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
        {
            Gameplay->OnGameplayMapChanged.AddDynamic(
                this,
                &ARemasterOverworldPawn::HandleGameplayMapChanged);
        }
    }

    SyncFromAuthoritativeState();
}

void ARemasterOverworldPawn::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterWorldGameplaySubsystem* Gameplay =
                GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
        {
            Gameplay->OnGameplayMapChanged.RemoveDynamic(
                this,
                &ARemasterOverworldPawn::HandleGameplayMapChanged);
        }
    }

    Super::EndPlay(EndPlayReason);
}

ARemasterWorldActor* ARemasterOverworldPawn::FindWorldRenderer() const
{
    UWorld* World = GetWorld();
    if (!World)
        return nullptr;

    for (TActorIterator<ARemasterWorldActor> It(World); It; ++It)
    {
        return *It;
    }

    return nullptr;
}

bool ARemasterOverworldPawn::SyncFromAuthoritativeState()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return false;

    const URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GI->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    ARemasterWorldActor* WorldRenderer = FindWorldRenderer();

    if (!SaveSubsystem
        || !SaveSubsystem->HasUsableSave()
        || !WorldRenderer)
    {
        return false;
    }

    FRemasterLegacyOverworldSnapshot Snapshot;
    if (!SaveSubsystem->GetOverworldSnapshot(Snapshot))
        return false;

    SetActorLocation(
        WorldRenderer->TileToWorldLocation(
            Snapshot.PlayerX,
            Snapshot.PlayerY,
            PlayerHeightOffset));

    return true;
}

bool ARemasterOverworldPawn::ApplyAuthoritativeStep(
    const FRemasterPlayerStepResult& StepResult)
{
    if (StepResult.Kind == ERemasterPlayerStepKind::Invalid)
        return false;

    ARemasterWorldActor* WorldRenderer = FindWorldRenderer();
    if (!WorldRenderer)
        return false;

    SetActorLocation(
        WorldRenderer->TileToWorldLocation(
            StepResult.PlayerX,
            StepResult.PlayerY,
            PlayerHeightOffset));

    ApplyFacing(StepResult.Direction);
    return true;
}

void ARemasterOverworldPawn::HandleGameplayMapChanged(
    int32,
    int32,
    FString)
{
    SyncFromAuthoritativeState();
}

void ARemasterOverworldPawn::ApplyFacing(int32 Direction)
{
    float Yaw = GetActorRotation().Yaw;

    switch (Direction)
    {
    case REMASTER_EMERALD_DIR_SOUTH:
        Yaw = 90.0f;
        break;
    case REMASTER_EMERALD_DIR_NORTH:
        Yaw = -90.0f;
        break;
    case REMASTER_EMERALD_DIR_WEST:
        Yaw = 180.0f;
        break;
    case REMASTER_EMERALD_DIR_EAST:
        Yaw = 0.0f;
        break;
    default:
        return;
    }

    SetActorRotation(FRotator(0.0f, Yaw, 0.0f));
}
