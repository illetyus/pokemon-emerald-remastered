#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterWorldData.h"
#include "RemasterWorldActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class URemasterVisualStyle;
class USceneComponent;
class UStaticMesh;

struct FRemasterChunkVisualKey
{
    int32 ChunkX = 0;
    int32 ChunkY = 0;
    uint32 VisualIdentityHash = 0;

    bool operator==(const FRemasterChunkVisualKey& Other) const
    {
        return ChunkX == Other.ChunkX
            && ChunkY == Other.ChunkY
            && VisualIdentityHash == Other.VisualIdentityHash;
    }
};

FORCEINLINE uint32 GetTypeHash(const FRemasterChunkVisualKey& Key)
{
    uint32 Hash = GetTypeHash(Key.ChunkX);
    Hash = HashCombine(Hash, GetTypeHash(Key.ChunkY));
    return HashCombine(Hash, GetTypeHash(Key.VisualIdentityHash));
}

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterWorldActor : public AActor
{
    GENERATED_BODY()

public:
    ARemasterWorldActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /*
     * Production path: copy the authoritative map already loaded by the
     * gameplay subsystem and rebuild renderer chunks from that exact IR.
     */
    UFUNCTION(BlueprintCallable, Category="Remaster|World")
    bool LoadAuthoritativeMap();

    /*
     * Debug/preview path only. Production map transitions are driven by the
     * gameplay subsystem's map-change event.
     */
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

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> FallbackMesh;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    TObjectPtr<URemasterVisualStyle> VisualStyle;

    UPROPERTY(EditAnywhere, Category="Remaster|World", meta=(ClampMin="1"))
    int32 ChunkTileSize = 16;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    float TileWorldSize = 100.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    float PreviewThickness = 10.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    float RenderPlaneWorldSpacing = 2.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World", meta=(ClampMin="0.0"))
    float RenderPlaneSpacing = 1.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World|Debug")
    FString StartupMapJson;

private:
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

    struct FResolvedMetatileVisual
    {
        FString Tileset;
        int32 LocalMetatileId = 0;
        const FRemasterTileVisualRule* Rule = nullptr;
    };

    FResolvedMetatileVisual ResolveMetatileVisual(uint16 MetatileId) const;

    UHierarchicalInstancedStaticMeshComponent* ComponentForMetatile(
        const FResolvedMetatileVisual& Visual,
        const FString& PlaneName,
        int32 PlaneIndex,
        const FIntPoint& Chunk);

    FRemasterMapIR LoadedMap;
    TMap<
        FRemasterChunkVisualKey,
        UHierarchicalInstancedStaticMeshComponent*>
        ChunkVisualComponents;
};
