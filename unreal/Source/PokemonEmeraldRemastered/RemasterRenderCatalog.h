#pragma once

#include "CoreMinimal.h"

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
};

struct FRemasterRenderMetatile
{
    int32 LocalMetatileId = -1;
    int32 Behavior = -1;
    int32 LayerType = -1;
    int32 AttributeU16 = -1;
    TArray<FString> RenderPlanes;
    TArray<FRemasterRenderTileEntry> Entries;
};

struct FRemasterRenderTilesetDescriptor
{
    int32 SchemaVersion = 0;
    FString Id;
    bool bSecondary = false;
    FString TileSymbol;
    FString PaletteSymbol;
    FString TilesPng;
    int32 TilesPngWidth = 0;
    int32 TilesPngHeight = 0;
    int32 TileCount = 0;
    TArray<FString> PaletteFiles;
    int32 MetatileCount = 0;
    TArray<FRemasterRenderMetatile> Metatiles;

    bool IsValid() const
    {
        return SchemaVersion == 1
            && !Id.IsEmpty()
            && !TilesPng.IsEmpty()
            && TilesPngWidth > 0
            && TilesPngHeight > 0
            && TileCount > 0
            && PaletteFiles.Num() == 16
            && MetatileCount > 0
            && Metatiles.Num() == MetatileCount;
    }

    const FRemasterRenderMetatile* FindMetatile(
        int32 LocalMetatileId) const;
};

struct FRemasterRenderCatalogEntry
{
    FString Id;
    bool bSecondary = false;
    FString DescriptorFile;
    FString DescriptorSha256;
    FString TilesPngFile;
    FString TilesPngSha256;
    int32 MetatileCount = 0;
};

class FRemasterRenderCatalog
{
public:
    bool LoadManifest(
        const FString& AbsoluteManifestPath,
        FString& OutError);

    const FRemasterRenderCatalogEntry* Find(
        const FString& TilesetId) const;

    const TArray<FRemasterRenderCatalogEntry>& Entries() const
    {
        return CatalogEntries;
    }

    static bool LoadDescriptor(
        const FString& AbsoluteRenderRoot,
        const FRemasterRenderCatalogEntry& Entry,
        FRemasterRenderTilesetDescriptor& OutDescriptor,
        FString& OutError);

    static bool IsSafePackageRelativePath(
        const FString& RelativePath);

private:
    TArray<FRemasterRenderCatalogEntry> CatalogEntries;
    TMap<FString, int32> IdIndex;
};
