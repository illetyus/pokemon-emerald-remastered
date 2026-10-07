#include "RemasterNpcPresentationWorld.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "RemasterCharacterVisualComponent.h"
#include "RemasterWorldActor.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "UObject/ConstructorHelpers.h"

ARemasterCharacterPresentationActor::ARemasterCharacterPresentationActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
    Placeholder->SetupAttachment(RootComponent);
    Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Placeholder->SetGenerateOverlapEvents(false);
    Placeholder->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.7f));
    Placeholder->SetRelativeLocation(FVector(0, 0, 35));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded())
        Placeholder->SetStaticMesh(Cylinder.Object);
    Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal"));
    Skeletal->SetupAttachment(RootComponent);
    Skeletal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual = CreateDefaultSubobject<URemasterCharacterVisualComponent>(TEXT("Visual"));
}

void ARemasterCharacterPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    Visual->SetupVisuals(Placeholder, Skeletal);
}

void ARemasterCharacterPresentationActor::ApplyGraphicsId(int32 GraphicsId)
{
    Visual->SetGraphicsId(GraphicsId);
}

ARemasterNpcPresentationWorld::ARemasterNpcPresentationWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ARemasterNpcPresentationWorld::ClearInstances()
{
    for (const auto& Pair : Instances)
        if (IsValid(Pair.Value))
            Pair.Value->Destroy();
    Instances.Reset();
    InstanceMapId.Reset();
}

void ARemasterNpcPresentationWorld::BeginPlay()
{
    Super::BeginPlay();
    if (UGameInstance* GI = GetGameInstance())
        if (URemasterWorldGameplaySubsystem* Gameplay =
            GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
            Gameplay->OnGameplayMapChanged.AddDynamic(
                this, &ARemasterNpcPresentationWorld::HandleMapChanged);
}

void ARemasterNpcPresentationWorld::HandleMapChanged(int32, int32, FString)
{
    // Also discard pending assets on a reload of the same map ID.
    ClearInstances();
}

void ARemasterNpcPresentationWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const UGameInstance* GI = GetGameInstance();
    const URemasterWorldGameplaySubsystem* Gameplay = GI
        ? GI->GetSubsystem<URemasterWorldGameplaySubsystem>() : nullptr;
    TArray<FRemasterObjectPresentationSnapshot> Snapshots;
    if (!Gameplay || !Gameplay->GetObjectPresentationSnapshots(Snapshots))
    {
        ClearInstances();
        return;
    }
    const FString MapId = Gameplay->GetCurrentMapId();
    if (InstanceMapId != MapId)
    {
        ClearInstances();
        InstanceMapId = MapId;
    }
    ARemasterWorldActor* Renderer = nullptr;
    for (TActorIterator<ARemasterWorldActor> It(GetWorld()); It; ++It)
    {
        Renderer = *It;
        break;
    }
    if (!Renderer || Renderer->GetLoadedMap().Id != MapId)
    {
        ClearInstances();
        return;
    }
    TSet<int32> PresentIds;
    for (const FRemasterObjectPresentationSnapshot& Snapshot : Snapshots)
    {
        if (!Snapshot.bVisible)
            continue;
        PresentIds.Add(Snapshot.LocalId);
        TObjectPtr<ARemasterCharacterPresentationActor>& Actor =
            Instances.FindOrAdd(Snapshot.LocalId);
        if (!IsValid(Actor))
        {
            FActorSpawnParameters Parameters;
            Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Actor = GetWorld()->SpawnActor<ARemasterCharacterPresentationActor>(
                ARemasterCharacterPresentationActor::StaticClass(), FTransform::Identity, Parameters);
        }
        if (!Actor)
            continue;
        Actor->ApplyGraphicsId(Snapshot.GraphicsId);
        Actor->SetActorLocation(Renderer->TileToWorldLocation(Snapshot.X, Snapshot.Y, 0));
        // Elevation is a collision layer, not a measured world Z coordinate.
        // Facing/motion are not exposed by R4 and are not simulated here.
    }
    for (auto It = Instances.CreateIterator(); It; ++It)
    {
        if (!PresentIds.Contains(It.Key()))
        {
            if (IsValid(It.Value()))
                It.Value()->Destroy();
            It.RemoveCurrent();
        }
    }
}

void ARemasterNpcPresentationWorld::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UGameInstance* GI = GetGameInstance())
        if (URemasterWorldGameplaySubsystem* Gameplay =
            GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
            Gameplay->OnGameplayMapChanged.RemoveDynamic(
                this, &ARemasterNpcPresentationWorld::HandleMapChanged);
    ClearInstances();
    Super::EndPlay(Reason);
}
