#include "RemasterWorldActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "RemasterMetatileRenderMath.h"
#include "RemasterRenderCatalogSubsystem.h"
#include "RemasterRenderResourceSubsystem.h"
#include "RemasterVisualStyle.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterWorldGridMath.h"
#include "RemasterEnvironmentAssetSet.h"
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
    const auto* Settings = GetDefault<URemasterEnvironmentPresentationSettings>();
    EnvironmentAssets = Settings ? Settings->AssetSet.LoadSynchronous() : nullptr;

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
    ++PresentationRevision;
    CameraOccluders.Reset();
    EnvironmentBudgets.Reset();
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
    const FString FullIdentity = FString::Printf(TEXT("%s:%d:%s:%d"),
        *Visual.Tileset, Visual.LocalMetatileId, *RenderPlane, PlaneIndex);
    const FRemasterChunkVisualKey Key{
        Chunk.X,
        Chunk.Y,
        FullIdentity
    };

    if (UHierarchicalInstancedStaticMeshComponent** Existing =
            ChunkVisualComponents.Find(Key))
    {
        return *Existing;
    }

    UGameInstance* GI = GetGameInstance();
    URemasterRenderResourceSubsystem* ResourceSubsystem =
        GI
            ? GI->GetSubsystem<URemasterRenderResourceSubsystem>()
            : nullptr;

    if (!ResourceSubsystem)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 render resource subsystem is unavailable."));
        return nullptr;
    }

    FRemasterTilesetRenderResources Resources;
    FString ResourceError;

    if (!ResourceSubsystem->LoadTilesetResources(
            Visual.Tileset,
            Resources,
            ResourceError))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 render resource load failed for %s: %s"),
            *Visual.Tileset,
            *ResourceError);
        return nullptr;
    }

    UStaticMesh* Mesh = FallbackMesh;
    if (!Mesh)
        return nullptr;

    UMaterialInterface* BaseMaterial = nullptr;

    if (Visual.Rule)
    {
        BaseMaterial = Visual.Rule->Material.LoadSynchronous();
    }

    if (!BaseMaterial && VisualStyle)
    {
        BaseMaterial =
            VisualStyle->MetatileMaterial.LoadSynchronous();
    }

    if (!BaseMaterial)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 indexed metatile material is not configured; using visible default material for %s."),
            *Visual.Tileset);
        BaseMaterial =
            UMaterial::GetDefaultMaterial(MD_Surface);
    }

    if (!BaseMaterial)
        return nullptr;

    UHierarchicalInstancedStaticMeshComponent* Component =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(
            this,
            *FString::Printf(
                TEXT("Chunk_%d_%d_%s_%d_%s_%d"),
                Chunk.X,
                Chunk.Y,
                *Visual.Tileset,
                Visual.LocalMetatileId,
                *RenderPlane,
                PlaneIndex));

    if (!Component)
        return nullptr;

    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false);
    Component->SetCanEverAffectNavigation(false);
    Component->SetCastShadow(false);
    Component->NumCustomDataFloats =
        remaster::metatile_render::CustomDataFloats;

    UMaterialInstanceDynamic* Material =
        UMaterialInstanceDynamic::Create(
            BaseMaterial,
            Component);

    if (!Material)
    {
        Component->DestroyComponent();
        return nullptr;
    }

    Material->SetTextureParameterValue(
        TEXT("R5_TileIndexTexture"),
        Resources.TileIndexTexture);
    Material->SetTextureParameterValue(
        TEXT("R5_PaletteTexture"),
        Resources.PaletteTexture);
    Material->SetScalarParameterValue(
        TEXT("R5_TileSheetWidth"),
        static_cast<float>(Resources.TileSheetWidth));
    Material->SetScalarParameterValue(
        TEXT("R5_TileSheetHeight"),
        static_cast<float>(Resources.TileSheetHeight));
    Material->SetScalarParameterValue(
        TEXT("R5_TilesPerRow"),
        static_cast<float>(Resources.TilesPerRow));
    Material->SetScalarParameterValue(
        TEXT("R5_SourceTilePixels"),
        8.0f);

    Component->SetMaterial(0, Material);
    Component->RegisterComponent();

    ChunkVisualComponents.Add(Key, Component);
    return Component;
}


bool ARemasterWorldActor::BuildRenderChunks()
{
    if (!LoadedMap.IsValid()
        || !FMath::IsFinite(TileWorldSize)
        || !FMath::IsFinite(RenderPlaneWorldSpacing)
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
                if (!AddMissingDescriptorFallback(Visual, X, Y, Chunk)) return false;
                continue;
            }

            FVector Scale = FallbackScale;
            float RuleHeightOffset = 0.0f;

            if (Visual.Rule)
            {
                Scale = Visual.Rule->Scale;
                RuleHeightOffset = Visual.Rule->HeightOffset;
            }

            // Optional validated 3D decoration. The R5 surface remains underneath
            // during load failures, camera cutaway and distance culling.
            AddEnvironmentVisual(Visual, X, Y, Chunk);

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

void ARemasterWorldActor::AddEnvironmentVisual(
    const FResolvedMetatileVisual& Visual, int32 TileX, int32 TileY, const FIntPoint& Chunk)
{
    if (!EnvironmentAssets) return;
    const FString Identity = FString::Printf(TEXT("%s:%d"), *Visual.Tileset, Visual.LocalMetatileId);
    const FRemasterEnvironmentBinding* Binding = EnvironmentAssets->Models.Find(Identity);
    if (!Binding || !Binding->IsUsable()) return;
    FEnvironmentChunkBudget& Budget = EnvironmentBudgets.FindOrAdd(Chunk);
    // Deterministic row-order admission; exceeding a source budget retains R5.
    const FRemasterChunkVisualKey Key{Chunk.X, Chunk.Y, TEXT("R7:") + Identity};
    UHierarchicalInstancedStaticMeshComponent* Component = nullptr;
    if (auto** Existing = ChunkVisualComponents.Find(Key)) Component = *Existing;
    if (!remaster::environment::can_admit(Budget.Components, Budget.Instances,
            Budget.Lod0Triangles, Binding->Lod0Triangles, !Component)) return;
    if (!Component)
    {
        UStaticMesh* Mesh = Binding->Mesh.LoadSynchronous();
        if (!Mesh) return;
        Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        if (!Component) return;
        Component->SetupAttachment(SceneRoot);
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->SetCastShadow(false);
        Component->SetCullDistances(0, FMath::RoundToInt(Binding->CullDistance));
        Component->RegisterComponent();
        ChunkVisualComponents.Add(Key, Component);
        ++Budget.Components;
    }
    const FTransform Transform(FRotator::ZeroRotator,
        TileToLocalLocation(TileX, TileY, Binding->HeightOffset), Binding->Scale);
    const int32 Instance = Component->AddInstance(Transform);
    if (Instance == INDEX_NONE) return;
    ++Budget.Instances;
    Budget.Lod0Triangles += Binding->Lod0Triangles;
    if (Binding->bCameraOccluder)
    {
        FCameraOccluder Item;
        Item.Component = Component;
        Item.InstanceIndex = Instance;
        Item.OriginalTransform = Transform;
        Item.LocalBounds = Component->GetStaticMesh()->GetBoundingBox().TransformBy(Transform);
        CameraOccluders.Add(Item);
    }
}

void ARemasterWorldActor::UpdateCameraOcclusion(const FVector& Camera, const FVector& Player)
{
    using namespace remaster::environment;
    const FVector LocalCamera = GetActorTransform().InverseTransformPosition(Camera);
    const FVector LocalPlayer = GetActorTransform().InverseTransformPosition(Player);
    for (FCameraOccluder& Item : CameraOccluders)
    {
        auto* Component = Item.Component.Get();
        if (!Component || !Item.LocalBounds.IsValid) continue;
        const FBox& Bounds = Item.LocalBounds;
        const bool Hidden = occludes({LocalCamera.X, LocalCamera.Y, LocalCamera.Z},
            {LocalPlayer.X, LocalPlayer.Y, LocalPlayer.Z},
            {Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z}, {Bounds.Max.X, Bounds.Max.Y, Bounds.Max.Z});
        if (Hidden == Item.bHidden) continue;
        FTransform RenderTransform = Item.OriginalTransform;
        if (Hidden) RenderTransform.SetScale3D(FVector::ZeroVector);
        if (Component->UpdateInstanceTransform(Item.InstanceIndex, RenderTransform, false, true, true))
            Item.bHidden = Hidden;
    }
}

void ARemasterWorldActor::RestoreCameraOcclusion()
{
    for (FCameraOccluder& Item : CameraOccluders)
    {
        if (Item.bHidden)
        {
            if (auto* Component = Item.Component.Get())
            {
                if (Component->UpdateInstanceTransform(Item.InstanceIndex, Item.OriginalTransform, false, true, true))
                    Item.bHidden = false;
            }
        }
    }
}

bool ARemasterWorldActor::AddMissingDescriptorFallback(
    const FResolvedMetatileVisual& Visual, int32 TileX, int32 TileY, const FIntPoint& Chunk)
{
    const FRemasterChunkVisualKey Key{Chunk.X, Chunk.Y,
        FString::Printf(TEXT("R7_missing:%s:%d"), *Visual.Tileset, Visual.LocalMetatileId)};
    UHierarchicalInstancedStaticMeshComponent* Component = nullptr;
    if (auto** Existing = ChunkVisualComponents.Find(Key)) Component = *Existing;
    if (!Component)
    {
        if (!FallbackMesh) return false;
        UE_LOG(LogTemp, Warning, TEXT("R7 visible descriptor fallback: %s in %s"),
            *Key.VisualIdentity, *LoadedMap.Id);
        Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        if (!Component) return false;
        Component->SetupAttachment(SceneRoot);
        Component->SetStaticMesh(FallbackMesh);
        Component->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->SetCastShadow(false);
        Component->RegisterComponent();
        ChunkVisualComponents.Add(Key, Component);
    }
    return Component->AddInstance(FTransform(FRotator::ZeroRotator,
        TileToLocalLocation(TileX, TileY, 0.0f), FVector(TileWorldSize / 100.0f,
        TileWorldSize / 100.0f, 1.0f))) != INDEX_NONE;
}
