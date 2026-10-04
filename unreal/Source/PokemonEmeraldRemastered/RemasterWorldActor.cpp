#include "RemasterWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "RemasterMetatileRenderMath.h"
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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
        TEXT("/Engine/BasicShapes/Plane.Plane"));

    if (PlaneMesh.Succeeded())
    {
        FallbackMesh = PlaneMesh.Object;
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

    if (!BuildRenderChunks())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 authoritative render build failed: %s"),
            *LoadedMap.Id);
        ClearWorld();
        return false;
    }

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

    if (!BuildRenderChunks())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 debug render build failed: %s"),
            *LoadedMap.Id);
        ClearWorld();
        return false;
    }

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
    const FString& PlaneName,
    int32 PlaneIndex,
    const FIntPoint& Chunk)
{
    uint32 IdentityHash = GetTypeHash(Visual.Tileset);
    IdentityHash = HashCombine(
        IdentityHash,
        GetTypeHash(Visual.LocalMetatileId));
    IdentityHash = HashCombine(
        IdentityHash,
        GetTypeHash(PlaneName));
    IdentityHash = HashCombine(
        IdentityHash,
        GetTypeHash(PlaneIndex));

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

    /*
     * R5 descriptor geometry is always a tile-aligned plane. A VisualStyle
     * may supply the material now; replacement 3D environment meshes remain
     * an R6 concern.
     */
    if (Visual.Rule)
    {
        Material = Visual.Rule->Material.LoadSynchronous();
    }

    if (!Mesh)
        return nullptr;

    UHierarchicalInstancedStaticMeshComponent* Component =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(
            this,
            *FString::Printf(
                TEXT("Chunk_%d_%d_Visual_%08X_%s"),
                Chunk.X,
                Chunk.Y,
                IdentityHash,
                *PlaneName));

    if (!Component)
        return nullptr;

    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(false);
    Component->NumCustomDataFloats =
        remaster::metatile_render::CustomDataFloats;

    if (Material)
    {
        Component->SetMaterial(0, Material);
    }

    Component->RegisterComponent();
    ChunkVisualComponents.Add(Key, Component);
    return Component;
}


bool ARemasterWorldActor::BuildRenderChunks()
{
    if (!LoadedMap.IsValid()
        || TileWorldSize <= 0.0f
        || ChunkTileSize <= 0)
    {
        return false;
    }

    UGameInstance* GI = GetGameInstance();
    URemasterRenderCatalogSubsystem* RenderCatalog =
        GI ? GI->GetSubsystem<URemasterRenderCatalogSubsystem>() : nullptr;

    if (!RenderCatalog || !RenderCatalog->IsCatalogReady())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 render catalog is unavailable for map %s"),
            *LoadedMap.Id);
        return false;
    }

    const float EnginePlaneSize = 100.0f;
    const FVector PlaneScale(
        TileWorldSize / EnginePlaneSize,
        TileWorldSize / EnginePlaneSize,
        1.0f);
    TSet<UHierarchicalInstancedStaticMeshComponent*> DirtyComponents;

    for (int32 Y = 0; Y < LoadedMap.Height; ++Y)
    {
        for (int32 X = 0; X < LoadedMap.Width; ++X)
        {
            const int32 Index = Y * LoadedMap.Width + X;
            if (!LoadedMap.MetatileIds.IsValidIndex(Index))
                return false;

            const uint16 MetatileId = LoadedMap.MetatileIds[Index];
            const FResolvedMetatileVisual Visual =
                ResolveMetatileVisual(MetatileId);
            const FIntPoint Chunk = ChunkForTile(X, Y);

            const FRemasterTilesetRenderDescriptor* Tileset = nullptr;
            const FRemasterMetatileRenderDescriptor* Metatile = nullptr;
            FString Error;

            if (!RenderCatalog->ResolveMetatile(
                    Visual.Tileset,
                    Visual.LocalMetatileId,
                    Tileset,
                    Metatile,
                    Error)
                || !Tileset
                || !Metatile)
            {
                UE_LOG(
                    LogTemp,
                    Error,
                    TEXT("R5 metatile resolve failed: map=%s tile=(%d,%d) global=%u tileset=%s local=%d error=%s"),
                    *LoadedMap.Id,
                    X,
                    Y,
                    static_cast<uint32>(MetatileId),
                    *Visual.Tileset,
                    Visual.LocalMetatileId,
                    *Error);
                return false;
            }

            const bool bExpectedSecondary = MetatileId >= 512u;
            if (Tileset->bIsSecondary != bExpectedSecondary
                || Metatile->RenderPlanes.Num() != 2
                || Metatile->Entries.Num() != 8)
            {
                UE_LOG(
                    LogTemp,
                    Error,
                    TEXT("R5 metatile descriptor identity mismatch: %s/%d"),
                    *Visual.Tileset,
                    Visual.LocalMetatileId);
                return false;
            }

            for (int32 PlaneIndex = 0;
                 PlaneIndex < Metatile->RenderPlanes.Num();
                 ++PlaneIndex)
            {
                const FString& PlaneName =
                    Metatile->RenderPlanes[PlaneIndex];

                remaster::metatile_render::PlaneOrder PlaneOrder =
                    remaster::metatile_render::PlaneOrder::Invalid;

                if (PlaneName.Equals(TEXT("bottom"), ESearchCase::CaseSensitive))
                {
                    PlaneOrder =
                        remaster::metatile_render::PlaneOrder::Bottom;
                }
                else if (PlaneName.Equals(TEXT("middle"), ESearchCase::CaseSensitive))
                {
                    PlaneOrder =
                        remaster::metatile_render::PlaneOrder::Middle;
                }
                else if (PlaneName.Equals(TEXT("top"), ESearchCase::CaseSensitive))
                {
                    PlaneOrder =
                        remaster::metatile_render::PlaneOrder::Top;
                }
                else
                {
                    UE_LOG(
                        LogTemp,
                        Error,
                        TEXT("R5 metatile has unknown render plane: %s/%d/%s"),
                        *Visual.Tileset,
                        Visual.LocalMetatileId,
                        *PlaneName);
                    return false;
                }

                UHierarchicalInstancedStaticMeshComponent* Target =
                    ComponentForMetatile(
                        Visual,
                        PlaneName,
                        PlaneIndex,
                        Chunk);

                if (!Target)
                    return false;

                float HeightOffset =
                    static_cast<float>(
                        remaster::metatile_render::plane_height(
                            PlaneOrder,
                            RenderPlaneSpacing));

                if (Visual.Rule)
                {
                    HeightOffset += Visual.Rule->HeightOffset;
                }

                const int32 InstanceIndex =
                    Target->AddInstance(
                        FTransform(
                            FRotator::ZeroRotator,
                            TileToLocalLocation(
                                X,
                                Y,
                                HeightOffset),
                            PlaneScale));

                if (InstanceIndex < 0)
                    return false;

                int32 PlaneEntryCount = 0;

                for (const FRemasterRenderTileEntry& Tile : Metatile->Entries)
                {
                    if (Tile.SourceLayer != PlaneIndex)
                        continue;

                    ++PlaneEntryCount;

                    const int32 TileIndex =
                        remaster::metatile_render::custom_data_index(
                            Tile.Quadrant,
                            remaster::metatile_render::CustomField::TileId);
                    const int32 PaletteIndex =
                        remaster::metatile_render::custom_data_index(
                            Tile.Quadrant,
                            remaster::metatile_render::CustomField::Palette);
                    const int32 HFlipIndex =
                        remaster::metatile_render::custom_data_index(
                            Tile.Quadrant,
                            remaster::metatile_render::CustomField::HFlip);
                    const int32 VFlipIndex =
                        remaster::metatile_render::custom_data_index(
                            Tile.Quadrant,
                            remaster::metatile_render::CustomField::VFlip);

                    if (TileIndex < 0
                        || PaletteIndex < 0
                        || HFlipIndex < 0
                        || VFlipIndex < 0)
                    {
                        return false;
                    }

                    Target->SetCustomDataValue(
                        InstanceIndex,
                        TileIndex,
                        static_cast<float>(Tile.TileIdRaw),
                        false);
                    Target->SetCustomDataValue(
                        InstanceIndex,
                        PaletteIndex,
                        static_cast<float>(Tile.Palette),
                        false);
                    Target->SetCustomDataValue(
                        InstanceIndex,
                        HFlipIndex,
                        Tile.bHFlip ? 1.0f : 0.0f,
                        false);
                    Target->SetCustomDataValue(
                        InstanceIndex,
                        VFlipIndex,
                        Tile.bVFlip ? 1.0f : 0.0f,
                        false);
                }

                if (PlaneEntryCount
                    != remaster::metatile_render::QuadrantsPerPlane)
                {
                    UE_LOG(
                        LogTemp,
                        Error,
                        TEXT("R5 plane does not contain four quadrants: %s/%d/%s"),
                        *Visual.Tileset,
                        Visual.LocalMetatileId,
                        *PlaneName);
                    return false;
                }

                DirtyComponents.Add(Target);
            }
        }
    }

    for (UHierarchicalInstancedStaticMeshComponent* Component
         : DirtyComponents)
    {
        if (Component)
            Component->MarkRenderStateDirty();
    }

    return true;
}
