#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterRenderResourceSubsystem.generated.h"

class UTexture2D;
class URemasterRenderCatalogSubsystem;

struct FRemasterTilesetRenderResources
{
    UTexture2D* TileIndexTexture = nullptr;
    UTexture2D* PaletteTexture = nullptr;
    int32 TileSheetWidth = 0;
    int32 TileSheetHeight = 0;
    int32 TilesPerRow = 0;

    bool IsValid() const
    {
        return TileIndexTexture != nullptr
            && PaletteTexture != nullptr
            && TileSheetWidth > 0
            && TileSheetHeight > 0
            && TilesPerRow > 0;
    }
};

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterRenderResourceSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool LoadTilesetResources(
        const FString& TilesetId,
        FRemasterTilesetRenderResources& OutResources,
        FString& OutError);

private:
    UTexture2D* CreateIndexTexture(
        const FString& DebugName,
        int32 Width,
        int32 Height,
        const TArray<uint8>& Pixels,
        FString& OutError);

    UTexture2D* CreatePaletteTexture(
        const FString& DebugName,
        const TArray<uint8>& RgbaPixels,
        FString& OutError);

    FString MakeCacheKey(
        const FString& TilesetId,
        const FString& DescriptorSha256,
        const FString& IndexSha256,
        const FString& PaletteLutSha256) const;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UTexture2D>> IndexTextureCache;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UTexture2D>> PaletteTextureCache;

    TMap<FString, FRemasterTilesetRenderResources> ResourceCache;
};
