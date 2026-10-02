#include "RemasterWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

ARemasterWorldActor::ARemasterWorldActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    BlockInstances =
        CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
            TEXT("BlockInstances"));
    BlockInstances->SetupAttachment(SceneRoot);
    BlockInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlockInstances->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (CubeMesh.Succeeded())
    {
        BlockInstances->SetStaticMesh(CubeMesh.Object);
    }
}

void ARemasterWorldActor::BeginPlay()
{
    Super::BeginPlay();

    if (!StartupMapJson.IsEmpty())
    {
        LoadMapFromGeneratedData(StartupMapJson);
    }
}

void ARemasterWorldActor::ClearWorld()
{
    BlockInstances->ClearInstances();
    LoadedMap = FRemasterMapIR{};
}

bool ARemasterWorldActor::LoadMapFromGeneratedData(
    const FString& RelativeJsonPath)
{
    FRemasterMapIR Candidate;
    FString Error;

    const FString AbsolutePath =
        FPaths::ConvertRelativePathToFull(
            FPaths::Combine(
                FPaths::ProjectContentDir(),
                RelativeJsonPath));

    if (!FRemasterWorldData::LoadMapJson(
            AbsolutePath,
            Candidate,
            Error))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Remaster world load failed: %s"),
            *Error);
        return false;
    }

    LoadedMap = MoveTemp(Candidate);
    BuildPreviewInstances();

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Loaded remaster map %s (%dx%d, %d raw blocks)"),
        *LoadedMap.Id,
        LoadedMap.Width,
        LoadedMap.Height,
        LoadedMap.RawBlocks.Num());

    return true;
}

void ARemasterWorldActor::BuildPreviewInstances()
{
    BlockInstances->ClearInstances();

    if (!LoadedMap.IsValid() || TileWorldSize <= 0.0f)
    {
        return;
    }

    const float EngineCubeSize = 100.0f;
    const FVector Scale(
        TileWorldSize / EngineCubeSize,
        TileWorldSize / EngineCubeSize,
        PreviewThickness / EngineCubeSize);

    for (int32 Y = 0; Y < LoadedMap.Height; ++Y)
    {
        for (int32 X = 0; X < LoadedMap.Width; ++X)
        {
            const int32 Index = Y * LoadedMap.Width + X;
            if (!LoadedMap.RawBlocks.IsValidIndex(Index))
            {
                continue;
            }

            /*
             * R4 preview intentionally does not decode gameplay semantics
             * from metatile bits. Raw block values are preserved for the
             * future visual mapping/import stage.
             */
            const FVector Location(
                static_cast<float>(X) * TileWorldSize,
                static_cast<float>(Y) * TileWorldSize,
                -PreviewThickness * 0.5f);

            BlockInstances->AddInstance(
                FTransform(
                    FRotator::ZeroRotator,
                    Location,
                    Scale));
        }
    }
}
