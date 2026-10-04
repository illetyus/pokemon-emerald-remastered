#include "RemasterWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "RemasterVisualStyle.h"
#include "RemasterRenderCatalogSubsystem.h"
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

ARemasterWorldActor::FResolvedMetatileVisual
ARemasterWorldActor::ResolveMetatileVisual(uint16 MetatileId) const
{
    FResolvedMetatileVisual Visual;

    if (MetatileId < 512u)
    {
        Visual.Tileset = LoadedMap.PrimaryTileset;
        Visual.LocalMetatileId = static_cast<int32>(MetatileId);
    }
    else
    {
        Visual.Tileset = LoadedMap.SecondaryTileset;
        Visual.LocalMetatileId =
            static_cast<int32>(MetatileId - 512u);
    }

    if (VisualStyle)
    {
        Visual.Rule =
            VisualStyle->TileRules.FindByPredicate(
                [&Visual](const FRemasterTileVisualRule& Candidate)
                {
                    return Candidate.LocalMetatileId
                            == Visual.LocalMetatileId
                        && Candidate.Tileset.Equals(
                            Visual.Tileset,
                            ESearchCase::CaseSensitive);
                });
    }

    return Visual;
}

UHierarchicalInstancedStaticMeshComponent*
ARemasterWorldActor::ComponentForMetatile(
    const FResolvedMetatileVisual& Visual,
    const FString& RenderPlane,
    const FIntPoint& Chunk)
{
    uint32 IdentityHash = GetTypeHash(Visual.Tileset);
    IdentityHash = HashCombine(
        IdentityHash,
        GetTypeHash(Visual.LocalMetatileId));
    IdentityHash = HashCombine(
        IdentityHash,
        GetTypeHash(RenderPlane));

    const FRemasterChunkVisualKey Key{
        Chunk.X,
        Chunk.Y,
        IdentityHash
    };

    if (UHierarchicalInstancedStaticMeshComponent** Existing =
            ChunkVisualComponents.Find(Key))
    {
        return *Existing;
    }

    UStaticMesh* Mesh = FallbackMesh;
    UMaterialInterface* Material = nullptr;
    bool bCastShadow = false;

    if (Visual.Rule)
    {
        if (UStaticMesh* RuleMesh =
                Visual.Rule->Mesh.LoadSynchronous())
        {
            Mesh = RuleMesh;
            bCastShadow = true;
        }

        Material = Visual.Rule->Material.LoadSynchronous();
    }

    if (!Mesh)
        return nullptr;

    UHierarchicalInstancedStaticMeshComponent* Component =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(
            this,
            *FString::Printf(
                TEXT("Chunk_%d_%d_Visual_%08X"),
                Chunk.X,
                Chunk.Y,
                IdentityHash));

    if (!Component)
        return nullptr;

    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(bCastShadow);
    Component->NumCustomDataFloats = 16;

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

    URemasterRenderCatalogSubsystem* RenderCatalog = nullptr;
    if (UGameInstance* GI = GetGameInstance())
    {
        RenderCatalog =
            GI->GetSubsystem<URemasterRenderCatalogSubsystem>();
    }

    const float EngineCubeSize = 100.0f;
    const FVector FallbackScale(
        TileWorldSize / EngineCubeSize,
        TileWorldSize / EngineCubeSize,
        PreviewThickness / EngineCubeSize);

    TSet<FString> ReportedDescriptorFailures;

    for (int32 Y = 0; Y < LoadedMap.Height; ++Y)
    {
        for (int32 X = 0; X < LoadedMap.Width; ++X)
        {
            const int32 Index = Y * LoadedMap.Width + X;
            if (!LoadedMap.MetatileIds.IsValidIndex(Index))
                continue;

            const uint16 MetatileId = LoadedMap.MetatileIds[Index];
            const FResolvedMetatileVisual Visual =
                ResolveMetatileVisual(MetatileId);
            const FIntPoint Chunk = ChunkForTile(X, Y);

            FVector Scale = FallbackScale;
            float BaseHeightOffset = -PreviewThickness * 0.5f;

            if (Visual.Rule)
            {
                Scale = Visual.Rule->Scale;
                BaseHeightOffset = Visual.Rule->HeightOffset;
            }

            const FRemasterTilesetRenderDescriptor* Tileset = nullptr;
            const FRemasterMetatileRenderDescriptor* Metatile = nullptr;
            FString DescriptorError;

            const bool bHasDescriptor =
                RenderCatalog
                && RenderCatalog->IsCatalogReady()
                && RenderCatalog->ResolveMetatile(
                    Visual.Tileset,
                    Visual.LocalMetatileId,
                    Tileset,
                    Metatile,
                    DescriptorError)
                && Tileset
                && Metatile;

            if (!bHasDescriptor)
            {
                const FString FailureKey = FString::Printf(
                    TEXT("%s:%d"),
                    *Visual.Tileset,
                    Visual.LocalMetatileId);

                if (!ReportedDescriptorFailures.Contains(FailureKey))
                {
                    ReportedDescriptorFailures.Add(FailureKey);
                    UE_LOG(
                        LogTemp,
                        Error,
                        TEXT("R5 descriptor unavailable for %s: %s"),
                        *FailureKey,
                        DescriptorError.IsEmpty()
                            ? TEXT("render catalog unavailable")
                            : *DescriptorError);
                }

                UHierarchicalInstancedStaticMeshComponent* Fallback =
                    ComponentForMetatile(
                        Visual,
                        TEXT("fallback"),
                        Chunk);

                if (Fallback)
                {
                    Fallback->AddInstance(
                        FTransform(
                            FRotator::ZeroRotator,
                            TileToLocalLocation(
                                X,
                                Y,
                                BaseHeightOffset),
                            Scale));
                }
                continue;
            }

            for (int32 PlaneIndex = 0; PlaneIndex < 2; ++PlaneIndex)
            {
                if (!Metatile->RenderPlanes.IsValidIndex(PlaneIndex))
                    continue;

                const FString& RenderPlane =
                    Metatile->RenderPlanes[PlaneIndex];

                UHierarchicalInstancedStaticMeshComponent* Target =
                    ComponentForMetatile(
                        Visual,
                        RenderPlane,
                        Chunk);

                if (!Target)
                    continue;

                float PlaneOrder = 0.0f;
                if (RenderPlane.Equals(
                        TEXT("middle"),
                        ESearchCase::CaseSensitive))
                {
                    PlaneOrder = 1.0f;
                }
                else if (RenderPlane.Equals(
                             TEXT("top"),
                             ESearchCase::CaseSensitive))
                {
                    PlaneOrder = 2.0f;
                }

                const int32 InstanceIndex = Target->AddInstance(
                    FTransform(
                        FRotator::ZeroRotator,
                        TileToLocalLocation(
                            X,
                            Y,
                            BaseHeightOffset
                                + PlaneOrder * RenderPlaneWorldSpacing),
                        Scale));

                for (const FRemasterRenderTileEntry& Tile
                     : Metatile->Entries)
                {
                    if (Tile.SourceLayer != PlaneIndex
                        || Tile.Quadrant < 0
                        || Tile.Quadrant > 3)
                    {
                        continue;
                    }

                    const int32 CustomBase = Tile.Quadrant * 4;
                    Target->SetCustomDataValue(
                        InstanceIndex,
                        CustomBase + 0,
                        static_cast<float>(Tile.TileIdRaw),
                        false);
                    Target->SetCustomDataValue(
                        InstanceIndex,
                        CustomBase + 1,
                        static_cast<float>(Tile.Palette),
                        false);
                    Target->SetCustomDataValue(
                        InstanceIndex,
                        CustomBase + 2,
                        Tile.bHFlip ? 1.0f : 0.0f,
                        false);
                    Target->SetCustomDataValue(
                        InstanceIndex,
                        CustomBase + 3,
                        Tile.bVFlip ? 1.0f : 0.0f,
                        false);
                }

                Target->MarkRenderStateDirty();
            }
        }
    }
}

