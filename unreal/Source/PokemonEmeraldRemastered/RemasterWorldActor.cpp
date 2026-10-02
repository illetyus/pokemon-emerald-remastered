#include "RemasterWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "RemasterVisualStyle.h"
#include "UObject/ConstructorHelpers.h"

ARemasterWorldActor::ARemasterWorldActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    BlockInstances =
        CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
            TEXT("FallbackBlockInstances"));
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

    for (TPair<int32, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Pair
         : VisualComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->DestroyComponent();
        }
    }

    VisualComponents.Reset();
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

    ClearWorld();
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

UHierarchicalInstancedStaticMeshComponent*
ARemasterWorldActor::ComponentForBlock(uint16 RawBlock)
{
    if (!VisualStyle)
    {
        return BlockInstances;
    }

    const FRemasterTileVisualRule* Rule =
        VisualStyle->TileRules.FindByPredicate(
            [RawBlock](const FRemasterTileVisualRule& Candidate)
            {
                return Candidate.RawBlockValue == static_cast<int32>(RawBlock);
            });

    if (!Rule)
    {
        return BlockInstances;
    }

    const int32 Key = Rule->RawBlockValue;

    if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Existing =
            VisualComponents.Find(Key))
    {
        return Existing->Get();
    }

    UStaticMesh* Mesh = Rule->Mesh.LoadSynchronous();
    if (!Mesh)
    {
        return BlockInstances;
    }

    UHierarchicalInstancedStaticMeshComponent* Component =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(
            this,
            *FString::Printf(TEXT("TileRule_%d"), Key));

    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(true);

    if (UMaterialInterface* Material = Rule->Material.LoadSynchronous())
    {
        Component->SetMaterial(0, Material);
    }

    Component->RegisterComponent();
    VisualComponents.Add(Key, Component);
    return Component;
}

void ARemasterWorldActor::BuildPreviewInstances()
{
    BlockInstances->ClearInstances();

    if (!LoadedMap.IsValid() || TileWorldSize <= 0.0f)
    {
        return;
    }

    const float EngineCubeSize = 100.0f;
    const FVector FallbackScale(
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

            const uint16 RawBlock = LoadedMap.RawBlocks[Index];
            UHierarchicalInstancedStaticMeshComponent* Target =
                ComponentForBlock(RawBlock);

            FVector Scale = FallbackScale;
            float HeightOffset = -PreviewThickness * 0.5f;

            if (VisualStyle)
            {
                if (const FRemasterTileVisualRule* Rule =
                        VisualStyle->TileRules.FindByPredicate(
                            [RawBlock](const FRemasterTileVisualRule& Candidate)
                            {
                                return Candidate.RawBlockValue ==
                                    static_cast<int32>(RawBlock);
                            }))
                {
                    Scale = Rule->Scale;
                    HeightOffset = Rule->HeightOffset;
                }
            }

            const FVector Location(
                static_cast<float>(X) * TileWorldSize,
                static_cast<float>(Y) * TileWorldSize,
                HeightOffset);

            Target->AddInstance(
                FTransform(
                    FRotator::ZeroRotator,
                    Location,
                    Scale));
        }
    }
}
