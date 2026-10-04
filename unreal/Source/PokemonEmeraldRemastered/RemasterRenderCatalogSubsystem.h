#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterRenderCatalogSubsystem.generated.h"

struct FRemasterRenderTileEntry
{
    int32 EntryIndex = -1;
    int32 SourceLayer = -1;
    int32 Quadrant = -1;
    int32 X = 0;
    int32 Y = 0;
    int32 TileIdRaw = -1;
    bool bHFlip = false;
    bool bVFlip = false;
    int32 Palette = -1;
    int32 RawU16 = -1;

    bool IsValid() const
    {
        return EntryIndex >= 0
            && EntryIndex < 8
            && SourceLayer >= 0
            && SourceLayer < 2
            && Quadrant >= 0
            && Quadrant < 4
            && X >= 0
            && X < 2
            && Y >= 0
            && Y < 2
            && TileIdRaw >= 0
            && TileIdRaw <= 0x03FF
            && Palette >= 0
            && Palette < 16
            && RawU16 >= 0
            && RawU16 <= 0xFFFF;
    }
};

struct FRemasterMetatileRenderDescriptor
{
    int32 LocalMetatileId = -1;
    int32 Behavior = -1;
    int32 LayerType = -1;
    TArray<FString> RenderPlanes;
    TArray<FRemasterRenderTileEntry> Entries;

    bool IsValid() const
    {
        return LocalMetatileId >= 0
            && Behavior >= 0
            && Behavior <= 0xFF
            && LayerType >= 0
            && LayerType <= 2
            && RenderPlanes.Num() == 2
            && Entries.Num() == 8;
    }
};

struct FRemasterTilesetRenderDescriptor
{
    FString Id;
    bool bIsSecondary = false;
    FString MetatileAssetRootSource;
    FString VisualAssetRootSource;
    FString TileSymbol;
    FString PaletteSymbol;
    FString TilesPngRelative;
    FString TilesIndex8Relative;
    FString PaletteLutRelative;
    FString PaletteLutSha256;
    int32 PaletteLutWidth = 0;
    int32 PaletteLutHeight = 0;
    int32 TilesPngWidth = 0;
    int32 TilesPngHeight = 0;
    int32 TileCount = 0;
    TArray<FString> PaletteFilesRelative;
    int32 MetatileCount = 0;
    TArray<FRemasterMetatileRenderDescriptor> Metatiles;

    bool IsValid() const
    {
        return !Id.IsEmpty()
            && !TilesPngRelative.IsEmpty()
            && !TilesIndex8Relative.IsEmpty()
            && !PaletteLutRelative.IsEmpty()
            && PaletteLutWidth == 16
            && PaletteLutHeight == 16
            && TilesPngWidth > 0
            && TilesPngHeight > 0
            && TileCount > 0
            && PaletteFilesRelative.Num() == 16
            && MetatileCount > 0
            && Metatiles.Num() == MetatileCount;
    }
};

struct FRemasterRenderCatalogEntry
{
    FString Id;
    bool bIsSecondary = false;
    FString DescriptorFile;
    FString DescriptorSha256;
    FString TilesPngFile;
    FString TilesPngSha256;
    FString TilesIndex8File;
    FString TilesIndex8Sha256;
    FString PaletteLutFile;
    FString PaletteLutSha256;
    TArray<FString> PaletteFiles;
    int32 MetatileCount = 0;
};

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterRenderCatalogSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Render")
    bool ReloadCatalog();

    UFUNCTION(BlueprintPure, Category="Remaster|World|Render")
    bool IsCatalogReady() const
    {
        return bCatalogReady;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|World|Render")
    FString GetManifestPath() const;

    FString GetPackageRoot() const;

    const FRemasterRenderCatalogEntry* FindTileset(
        const FString& TilesetId) const;

    bool LoadTileset(
        const FString& TilesetId,
        const FRemasterTilesetRenderDescriptor*& OutDescriptor,
        FString& OutError);

    bool ResolveMetatile(
        const FString& TilesetId,
        int32 LocalMetatileId,
        const FRemasterTilesetRenderDescriptor*& OutTileset,
        const FRemasterMetatileRenderDescriptor*& OutMetatile,
        FString& OutError);

    bool ResolvePackageFile(
        const FString& RelativePath,
        FString& OutAbsolutePath) const;

private:
    bool LoadDescriptor(
        const FRemasterRenderCatalogEntry& Entry,
        FRemasterTilesetRenderDescriptor& OutDescriptor,
        FString& OutError) const;

    bool bCatalogReady = false;
    FString SourceRepository;
    FString SourceCommit;
    FString ContentSha256;
    TMap<FString, FRemasterRenderCatalogEntry> Entries;
    TMap<
        FString,
        TSharedPtr<FRemasterTilesetRenderDescriptor>>
        DescriptorCache;
};
