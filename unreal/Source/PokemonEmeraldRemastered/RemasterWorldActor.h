#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterWorldData.h"
#include "RemasterWorldActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class URemasterVisualStyle;
class URemasterEnvironmentAssetSet;
class USceneComponent;
class UStaticMesh;
struct FRemasterTileVisualRule;

struct FRemasterChunkVisualKey
{
    int32 ChunkX = 0;
    int32 ChunkY = 0;
    FString VisualIdentity;

    bool operator==(const FRemasterChunkVisualKey& Other) const
    {
        return ChunkX == Other.ChunkX
            && ChunkY == Other.ChunkY
            && VisualIdentity == Other.VisualIdentity;
    }
};

FORCEINLINE uint32 GetTypeHash(const FRemasterChunkVisualKey& Key)
{
    uint32 Hash = GetTypeHash(Key.ChunkX);
    Hash = HashCombine(Hash, GetTypeHash(Key.ChunkY));
    return HashCombine(Hash, GetTypeHash(Key.VisualIdentity));
}

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterWorldActor : public AActor
{
    GENERATED_BODY()

public:
    ARemasterWorldActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="Remaster|World")
    bool LoadAuthoritativeMap();

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Debug")
    bool LoadMapFromGeneratedData(const FString& RelativeJsonPath);

    UFUNCTION(BlueprintCallable, Category="Remaster|World")
    void ClearWorld();

    UFUNCTION(BlueprintPure, Category="Remaster|World")
    FVector TileToWorldLocation(
        int32 TileX,
        int32 TileY,
        float HeightOffset = 0.0f) const;

    UFUNCTION(BlueprintPure, Category="Remaster|World")
    FIntPoint WorldToTileLocation(const FVector& WorldLocation) const;

    const FRemasterMapIR& GetLoadedMap() const
    {
        return LoadedMap;
    }

    uint64 GetPresentationRevision() const { return PresentationRevision; }
    void UpdateCameraOcclusion(const FVector& Camera, const FVector& Player);
    void RestoreCameraOcclusion();

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> FallbackMesh;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    TObjectPtr<URemasterVisualStyle> VisualStyle;

    UPROPERTY(Transient)
    TObjectPtr<URemasterEnvironmentAssetSet> EnvironmentAssets;

    UPROPERTY(EditAnywhere, Category="Remaster|World", meta=(ClampMin="1"))
    int32 ChunkTileSize = 16;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    float TileWorldSize = 100.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World", meta=(ClampMin="0.0"))
    float RenderPlaneWorldSpacing = 2.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World|Debug")
    FString StartupMapJson;

private:
    struct FResolvedMetatileVisual;
    UFUNCTION()
    void HandleGameplayMapChanged(
        int32 MapGroup,
        int32 MapNum,
        FString MapId);

    FVector TileToLocalLocation(
        int32 TileX,
        int32 TileY,
        float HeightOffset) const;

    FIntPoint ChunkForTile(int32 TileX, int32 TileY) const;

    bool BuildRenderChunks();
    bool AddMissingDescriptorFallback(const FResolvedMetatileVisual& Visual,
        int32 TileX, int32 TileY, const FIntPoint& Chunk);
    void AddEnvironmentVisual(const FResolvedMetatileVisual& Visual,
        int32 TileX, int32 TileY, const FIntPoint& Chunk);

    struct FResolvedMetatileVisual
    {
        FString Tileset;
        int32 LocalMetatileId = 0;
        const FRemasterTileVisualRule* Rule = nullptr;
    };

    FResolvedMetatileVisual ResolveMetatileVisual(uint16 MetatileId) const;

    UHierarchicalInstancedStaticMeshComponent* ComponentForMetatile(
        const FResolvedMetatileVisual& Visual,
        const FString& RenderPlane,
        int32 PlaneIndex,
        const FIntPoint& Chunk);

    FRemasterMapIR LoadedMap;
    uint64 PresentationRevision = 0;
    struct FEnvironmentChunkBudget
    {
        int32 Components = 0;
        int32 Instances = 0;
        int32 Lod0Triangles = 0;
    };
    TMap<FIntPoint, FEnvironmentChunkBudget> EnvironmentBudgets;
    struct FCameraOccluder
    {
        TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> Component;
        int32 InstanceIndex = INDEX_NONE;
        FTransform OriginalTransform;
        FBox LocalBounds;
        bool bHidden = false;
    };
    TArray<FCameraOccluder> CameraOccluders;
    TMap<
        FRemasterChunkVisualKey,
        UHierarchicalInstancedStaticMeshComponent*>
        ChunkVisualComponents;
};
