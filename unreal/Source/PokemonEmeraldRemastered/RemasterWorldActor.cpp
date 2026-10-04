#include "RemasterWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "RemasterMetatileRenderMath.h"
#include "RemasterRenderCatalogSubsystem.h"
#include "RemasterVisualStyle.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterWorldGridMath.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
remaster::metatile_render::PlaneOrder PlaneOrderFromName(
    const FString& Plane)
{
    using remaster::metatile_render::PlaneOrder;

    if (Plane.Equals(TEXT("bottom"), ESearchCase::CaseSensitive))
        return PlaneOrder::Bottom;
    if (Plane.Equals(TEXT("middle"), ESearchCase::CaseSensitive))
        return PlaneOrder::Middle;
    if (Plane.Equals(TEXT("top"), ESearchCase::CaseSensitive))
        return PlaneOrder::Top;

    return PlaneOrder::Invalid;
}
}

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
            TEXT("R5 failed to build authoritative render chunks for %s"),
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
            TEXT("R5 debug map render package unavailable for %s"),
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
    const FString& RenderPlane,
    int32 PlaneIndex,
    const FIntPoint& Chunk)
{
    uint32 IdentityHash = GetTypeHash(Visual.Tileset);
    IdentityHash = HashCombine(
        IdentityHash,
        GetTypeHash(Visual.LocalMetatileId));
    IdentityHash = HashCombine(
        IdentityHash,
        GetTypeHash(RenderPlane));
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
    if (!GI)
        return false;

    URemasterRenderCatalogSubsystem* RenderCatalog =
        GI->GetSubsystem<URemasterRenderCatalogSubsystem>();

    if (!RenderCatalog || !RenderCatalog->IsCatalogReady())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 packaged render catalog is not ready."));
        return false;
    }

    const float EnginePlaneSize = 100.0f;
    const FVector FallbackScale(
        TileWorldSize / EnginePlaneSize,
        TileWorldSize / EnginePlaneSize,
        1.0f);

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
            FString DescriptorError;

            if (!RenderCatalog->ResolveMetatile(
                    Visual.Tileset,
                    Visual.LocalMetatileId,
                    Tileset,
                    Metatile,
                    DescriptorError)
                || !Tileset
                || !Metatile
                || Metatile->RenderPlanes.Num() != 2)
            {
                UE_LOG(
                    LogTemp,
                    Error,
                    TEXT("R5 descriptor resolution failed for %s:%d: %s"),
                    *Visual.Tileset,
                    Visual.LocalMetatileId,
                    *DescriptorError);
                return false;
            }

            FVector Scale = FallbackScale;
            float RuleHeightOffset = 0.0f;

            if (Visual.Rule)
            {
                Scale = Visual.Rule->Scale;
                RuleHeightOffset = Visual.Rule->HeightOffset;
            }

            for (int32 PlaneIndex = 0;
                 PlaneIndex < 2;
                 ++PlaneIndex)
            {
                const FString& RenderPlane =
                    Metatile->RenderPlanes[PlaneIndex];

                const remaster::metatile_render::PlaneOrder PlaneOrder =
                    PlaneOrderFromName(RenderPlane);

                if (PlaneOrder
                    == remaster::metatile_render::PlaneOrder::Invalid)
                {
                    UE_LOG(
                        LogTemp,
                        Error,
                        TEXT("R5 invalid render plane %s for %s:%d"),
                        *RenderPlane,
                        *Visual.Tileset,
                        Visual.LocalMetatileId);
                    return false;
                }

                UHierarchicalInstancedStaticMeshComponent* Target =
                    ComponentForMetatile(
                        Visual,
                        RenderPlane,
                        PlaneIndex,
                        Chunk);

                if (!Target)
                    return false;

                const float HeightOffset =
                    RuleHeightOffset
                    + static_cast<float>(
                        remaster::metatile_render::plane_height(
                            PlaneOrder,
                            RenderPlaneWorldSpacing));

                const int32 InstanceIndex = Target->AddInstance(
                    FTransform(
                        FRotator::ZeroRotator,
                        TileToLocalLocation(
                            X,
                            Y,
                            HeightOffset),
                        Scale));

                bool SeenQuadrants[
                    remaster::metatile_render::QuadrantsPerPlane] = {};

                for (const FRemasterRenderTileEntry& Tile
                     : Metatile->Entries)
                {
                    if (Tile.SourceLayer != PlaneIndex)
                        continue;

                    if (Tile.Quadrant < 0
                        || Tile.Quadrant
                            >= remaster::metatile_render::QuadrantsPerPlane
                        || SeenQuadrants[Tile.Quadrant])
                    {
                        return false;
                    }

                    SeenQuadrants[Tile.Quadrant] = true;

                    const int32 TileIdIndex =
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

                    if (TileIdIndex < 0
                        || PaletteIndex < 0
                        || HFlipIndex < 0
                        || VFlipIndex < 0)
                    {
                        return false;
                    }

                    Target->SetCustomDataValue(
                        InstanceIndex,
                        TileIdIndex,
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

                for (bool bSeen : SeenQuadrants)
                {
                    if (!bSeen)
                        return false;
                }

                Target->MarkRenderStateDirty();
            }
        }
    }

    return true;
}
