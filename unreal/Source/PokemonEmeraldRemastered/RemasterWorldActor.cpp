#include "RemasterWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "RemasterVisualStyle.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterWorldGridMath.h"
#include "UObject/ConstructorHelpers.h"

ARemasterWorldActor::ARemasterWorldActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (CubeMesh.Succeeded())
    {
        FallbackMesh = CubeMesh.Object;
    }
}

void ARemasterWorldActor::BeginPlay()
{
    Super::BeginPlay();

    bool bLoadedAuthoritative = false;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterWorldGameplaySubsystem* Gameplay =
                GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
        {
            Gameplay->OnGameplayMapChanged.AddDynamic(
                this,
                &ARemasterWorldActor::HandleGameplayMapChanged);

            if (Gameplay->IsMapReady())
            {
                bLoadedAuthoritative = LoadAuthoritativeMap();
            }
        }
    }

    if (!bLoadedAuthoritative && !StartupMapJson.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("R5 world renderer is using debug JSON startup path: %s"),
            *StartupMapJson);
        LoadMapFromGeneratedData(StartupMapJson);
    }
}

void ARemasterWorldActor::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterWorldGameplaySubsystem* Gameplay =
                GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
        {
            Gameplay->OnGameplayMapChanged.RemoveDynamic(
                this,
                &ARemasterWorldActor::HandleGameplayMapChanged);
        }
    }

    ClearWorld();
    Super::EndPlay(EndPlayReason);
}

void ARemasterWorldActor::ClearWorld()
{
    for (TPair<
            FRemasterChunkVisualKey,
            UHierarchicalInstancedStaticMeshComponent*>& Pair
         : ChunkVisualComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->DestroyComponent();
        }
    }

    ChunkVisualComponents.Reset();
    LoadedMap = FRemasterMapIR{};
}

bool ARemasterWorldActor::LoadAuthoritativeMap()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return false;

    URemasterWorldGameplaySubsystem* Gameplay =
        GI->GetSubsystem<URemasterWorldGameplaySubsystem>();

    if (!Gameplay || !Gameplay->IsMapReady())
        return false;

    const FRemasterMapIR* Source =
        Gameplay->GetCurrentMapForPresentation();

    if (!Source || !Source->IsValid())
        return false;

    ClearWorld();
    LoadedMap = *Source;
    BuildRenderChunks();

    UE_LOG(
        LogTemp,
        Display,
        TEXT("R5 authoritative map rendered: %s (%dx%d, chunk=%d)"),
        *LoadedMap.Id,
        LoadedMap.Width,
        LoadedMap.Height,
        ChunkTileSize);

    return true;
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
            TEXT("Remaster world debug load failed: %s"),
            *Error);
        return false;
    }

    ClearWorld();
    LoadedMap = MoveTemp(Candidate);
    BuildRenderChunks();

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Loaded debug remaster map %s (%dx%d, %d raw blocks)"),
        *LoadedMap.Id,
        LoadedMap.Width,
        LoadedMap.Height,
        LoadedMap.RawBlocks.Num());

    return true;
}

void ARemasterWorldActor::HandleGameplayMapChanged(
    int32 MapGroup,
    int32 MapNum,
    FString MapId)
{
    if (!LoadAuthoritativeMap())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 renderer failed authoritative map refresh: %s (%d,%d)"),
            *MapId,
            MapGroup,
            MapNum);
    }
}

FVector ARemasterWorldActor::TileToLocalLocation(
    int32 TileX,
    int32 TileY,
    float HeightOffset) const
{
    return FVector(
        remaster::world_grid::tile_axis_to_local(
            TileX,
            TileWorldSize),
        remaster::world_grid::tile_axis_to_local(
            TileY,
            TileWorldSize),
        HeightOffset);
}

FVector ARemasterWorldActor::TileToWorldLocation(
    int32 TileX,
    int32 TileY,
    float HeightOffset) const
{
    return GetActorTransform().TransformPosition(
        TileToLocalLocation(
            TileX,
            TileY,
            HeightOffset));
}

FIntPoint ARemasterWorldActor::WorldToTileLocation(
    const FVector& WorldLocation) const
{
    if (TileWorldSize <= 0.0f)
        return FIntPoint::ZeroValue;

    const FVector Local =
        GetActorTransform().InverseTransformPosition(WorldLocation);

    return FIntPoint(
        remaster::world_grid::local_axis_to_tile(
            Local.X,
            TileWorldSize),
        remaster::world_grid::local_axis_to_tile(
            Local.Y,
            TileWorldSize));
}

FIntPoint ARemasterWorldActor::ChunkForTile(
    int32 TileX,
    int32 TileY) const
{
    const remaster::world_grid::ChunkCoord Chunk =
        remaster::world_grid::chunk_for_tile(
            TileX,
            TileY,
            FMath::Max(1, ChunkTileSize));

    return FIntPoint(Chunk.x, Chunk.y);
}

UHierarchicalInstancedStaticMeshComponent*
ARemasterWorldActor::ComponentForMetatile(
    uint16 MetatileId,
    const FIntPoint& Chunk)
{
    const FRemasterChunkVisualKey Key{
        Chunk.X,
        Chunk.Y,
        static_cast<int32>(MetatileId)
    };

    if (UHierarchicalInstancedStaticMeshComponent** Existing =
            ChunkVisualComponents.Find(Key))
    {
        return *Existing;
    }

    UStaticMesh* Mesh = FallbackMesh;
    UMaterialInterface* Material = nullptr;
    bool bCastShadow = false;

    if (VisualStyle)
    {
        if (const FRemasterTileVisualRule* Rule =
                VisualStyle->TileRules.FindByPredicate(
                    [MetatileId](const FRemasterTileVisualRule& Candidate)
                    {
                        return Candidate.MetatileId
                            == static_cast<int32>(MetatileId);
                    }))
        {
            if (UStaticMesh* RuleMesh = Rule->Mesh.LoadSynchronous())
            {
                Mesh = RuleMesh;
                bCastShadow = true;
            }

            Material = Rule->Material.LoadSynchronous();
        }
    }

    if (!Mesh)
        return nullptr;

    UHierarchicalInstancedStaticMeshComponent* Component =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(
            this,
            *FString::Printf(
                TEXT("Chunk_%d_%d_Metatile_%u"),
                Chunk.X,
                Chunk.Y,
                static_cast<uint32>(MetatileId)));

    if (!Component)
        return nullptr;

    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(bCastShadow);

    if (Material)
    {
        Component->SetMaterial(0, Material);
    }

    Component->RegisterComponent();
    ChunkVisualComponents.Add(Key, Component);
    return Component;
}

void ARemasterWorldActor::BuildRenderChunks()
{
    if (!LoadedMap.IsValid()
        || TileWorldSize <= 0.0f
        || ChunkTileSize <= 0)
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
            if (!LoadedMap.MetatileIds.IsValidIndex(Index))
            {
                continue;
            }

            const uint16 MetatileId = LoadedMap.MetatileIds[Index];
            const FIntPoint Chunk = ChunkForTile(X, Y);

            UHierarchicalInstancedStaticMeshComponent* Target =
                ComponentForMetatile(MetatileId, Chunk);

            if (!Target)
                continue;

            FVector Scale = FallbackScale;
            float HeightOffset = -PreviewThickness * 0.5f;

            if (VisualStyle)
            {
                if (const FRemasterTileVisualRule* Rule =
                        VisualStyle->TileRules.FindByPredicate(
                            [MetatileId](const FRemasterTileVisualRule& Candidate)
                            {
                                return Candidate.MetatileId
                                    == static_cast<int32>(MetatileId);
                            }))
                {
                    Scale = Rule->Scale;
                    HeightOffset = Rule->HeightOffset;
                }
            }

            Target->AddInstance(
                FTransform(
                    FRotator::ZeroRotator,
                    TileToLocalLocation(
                        X,
                        Y,
                        HeightOffset),
                    Scale));
        }
    }
}
