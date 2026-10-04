#include "RemasterRenderCatalog.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadJsonObject(
    const FString& AbsolutePath,
    TSharedPtr<FJsonObject>& OutObject,
    FString& OutError)
{
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
    {
        OutError = FString::Printf(
            TEXT("Unable to read R5 render JSON: %s"),
            *AbsolutePath);
        return false;
    }

    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonText);

    if (!FJsonSerializer::Deserialize(Reader, OutObject)
        || !OutObject.IsValid())
    {
        OutError = FString::Printf(
            TEXT("Invalid R5 render JSON: %s"),
            *AbsolutePath);
        return false;
    }

    return true;
}

bool ReadInt(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Field,
    int32& OutValue)
{
    double Number = 0.0;
    if (!Object.IsValid()
        || !Object->TryGetNumberField(Field, Number))
    {
        return false;
    }

    OutValue = static_cast<int32>(Number);
    return true;
}

bool ReadStringArray(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Field,
    TArray<FString>& OutValues)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object.IsValid()
        || !Object->TryGetArrayField(Field, Values)
        || Values == nullptr)
    {
        return false;
    }

    OutValues.Reset();
    OutValues.Reserve(Values->Num());

    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        FString Item;
        if (!Value.IsValid()
            || !Value->TryGetString(Item))
        {
            return false;
        }
        OutValues.Add(MoveTemp(Item));
    }

    return true;
}
}

const FRemasterRenderMetatile*
FRemasterRenderTilesetDescriptor::FindMetatile(
    int32 LocalMetatileId) const
{
    if (LocalMetatileId < 0
        || !Metatiles.IsValidIndex(LocalMetatileId))
    {
        return nullptr;
    }

    const FRemasterRenderMetatile& Candidate =
        Metatiles[LocalMetatileId];

    return Candidate.LocalMetatileId == LocalMetatileId
        ? &Candidate
        : nullptr;
}

bool FRemasterRenderCatalog::IsSafePackageRelativePath(
    const FString& RelativePath)
{
    if (RelativePath.IsEmpty()
        || !FPaths::IsRelative(RelativePath))
    {
        return false;
    }

    FString Normalized = RelativePath;
    FPaths::NormalizeFilename(Normalized);

    TArray<FString> Parts;
    Normalized.ParseIntoArray(Parts, TEXT("/"), true);

    return !Parts.Contains(TEXT(".."));
}

bool FRemasterRenderCatalog::LoadManifest(
    const FString& AbsoluteManifestPath,
    FString& OutError)
{
    CatalogEntries.Reset();
    IdIndex.Reset();

    TSharedPtr<FJsonObject> Root;
    if (!ReadJsonObject(
            AbsoluteManifestPath,
            Root,
            OutError))
    {
        return false;
    }

    int32 SchemaVersion = 0;
    if (!ReadInt(Root, TEXT("schema_version"), SchemaVersion)
        || SchemaVersion != 1)
    {
        OutError = TEXT("R5 render manifest schema_version must be 1.");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Tilesets = nullptr;
    if (!Root->TryGetArrayField(TEXT("tilesets"), Tilesets)
        || Tilesets == nullptr
        || Tilesets->IsEmpty())
    {
        OutError = TEXT("R5 render manifest is missing tilesets.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *Tilesets)
    {
        const TSharedPtr<FJsonObject> Object = Value->AsObject();
        if (!Object.IsValid())
        {
            OutError = TEXT("R5 render manifest contains non-object tileset.");
            return false;
        }

        FRemasterRenderCatalogEntry Entry;
        Object->TryGetStringField(TEXT("id"), Entry.Id);
        Object->TryGetBoolField(TEXT("is_secondary"), Entry.bSecondary);
        Object->TryGetStringField(
            TEXT("descriptor_file"),
            Entry.DescriptorFile);
        Object->TryGetStringField(
            TEXT("descriptor_sha256"),
            Entry.DescriptorSha256);
        Object->TryGetStringField(
            TEXT("tiles_png_file"),
            Entry.TilesPngFile);
        Object->TryGetStringField(
            TEXT("tiles_png_sha256"),
            Entry.TilesPngSha256);
        ReadInt(Object, TEXT("metatile_count"), Entry.MetatileCount);

        if (Entry.Id.IsEmpty()
            || Entry.MetatileCount <= 0
            || !IsSafePackageRelativePath(Entry.DescriptorFile)
            || !IsSafePackageRelativePath(Entry.TilesPngFile)
            || Entry.DescriptorSha256.Len() != 64
            || Entry.TilesPngSha256.Len() != 64
            || IdIndex.Contains(Entry.Id))
        {
            OutError = FString::Printf(
                TEXT("Invalid or duplicate R5 render tileset entry: %s"),
                *Entry.Id);
            CatalogEntries.Reset();
            IdIndex.Reset();
            return false;
        }

        const int32 Index = CatalogEntries.Add(MoveTemp(Entry));
        IdIndex.Add(CatalogEntries[Index].Id, Index);
    }

    return true;
}

const FRemasterRenderCatalogEntry*
FRemasterRenderCatalog::Find(
    const FString& TilesetId) const
{
    const int32* Index = IdIndex.Find(TilesetId);
    return Index && CatalogEntries.IsValidIndex(*Index)
        ? &CatalogEntries[*Index]
        : nullptr;
}

bool FRemasterRenderCatalog::LoadDescriptor(
    const FString& AbsoluteRenderRoot,
    const FRemasterRenderCatalogEntry& Entry,
    FRemasterRenderTilesetDescriptor& OutDescriptor,
    FString& OutError)
{
    OutDescriptor = FRemasterRenderTilesetDescriptor{};

    if (!IsSafePackageRelativePath(Entry.DescriptorFile))
    {
        OutError = TEXT("Unsafe R5 descriptor path.");
        return false;
    }

    const FString DescriptorPath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(
            AbsoluteRenderRoot,
            Entry.DescriptorFile));

    TSharedPtr<FJsonObject> Root;
    if (!ReadJsonObject(DescriptorPath, Root, OutError))
        return false;

    if (!ReadInt(
            Root,
            TEXT("schema_version"),
            OutDescriptor.SchemaVersion)
        || OutDescriptor.SchemaVersion != 1)
    {
        OutError = TEXT("R5 descriptor schema_version must be 1.");
        return false;
    }

    Root->TryGetStringField(TEXT("id"), OutDescriptor.Id);
    Root->TryGetBoolField(
        TEXT("is_secondary"),
        OutDescriptor.bSecondary);
    Root->TryGetStringField(
        TEXT("tile_symbol"),
        OutDescriptor.TileSymbol);
    Root->TryGetStringField(
        TEXT("palette_symbol"),
        OutDescriptor.PaletteSymbol);
    Root->TryGetStringField(
        TEXT("tiles_png"),
        OutDescriptor.TilesPng);

    if (!ReadInt(
            Root,
            TEXT("tiles_png_width"),
            OutDescriptor.TilesPngWidth)
        || !ReadInt(
            Root,
            TEXT("tiles_png_height"),
            OutDescriptor.TilesPngHeight)
        || !ReadInt(
            Root,
            TEXT("tile_count"),
            OutDescriptor.TileCount)
        || !ReadInt(
            Root,
            TEXT("metatile_count"),
            OutDescriptor.MetatileCount)
        || !ReadStringArray(
            Root,
            TEXT("palette_files"),
            OutDescriptor.PaletteFiles))
    {
        OutError = TEXT("R5 descriptor is missing numeric/palette fields.");
        return false;
    }

    if (OutDescriptor.Id != Entry.Id
        || OutDescriptor.bSecondary != Entry.bSecondary
        || !IsSafePackageRelativePath(OutDescriptor.TilesPng))
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor identity/path mismatch for %s"),
            *Entry.Id);
        return false;
    }

    for (const FString& Palette : OutDescriptor.PaletteFiles)
    {
        if (!IsSafePackageRelativePath(Palette))
        {
            OutError = FString::Printf(
                TEXT("Unsafe R5 palette path in %s"),
                *Entry.Id);
            return false;
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Metatiles = nullptr;
    if (!Root->TryGetArrayField(TEXT("metatiles"), Metatiles)
        || Metatiles == nullptr)
    {
        OutError = TEXT("R5 descriptor is missing metatiles.");
        return false;
    }

    OutDescriptor.Metatiles.Reserve(Metatiles->Num());

    for (const TSharedPtr<FJsonValue>& Value : *Metatiles)
    {
        const TSharedPtr<FJsonObject> Object = Value->AsObject();
        if (!Object.IsValid())
        {
            OutError = TEXT("R5 descriptor contains non-object metatile.");
            return false;
        }

        FRemasterRenderMetatile Metatile;
        if (!ReadInt(
                Object,
                TEXT("local_metatile_id"),
                Metatile.LocalMetatileId)
            || !ReadInt(
                Object,
                TEXT("behavior"),
                Metatile.Behavior)
            || !ReadInt(
                Object,
                TEXT("layer_type"),
                Metatile.LayerType)
            || !ReadInt(
                Object,
                TEXT("attribute_u16"),
                Metatile.AttributeU16)
            || !ReadStringArray(
                Object,
                TEXT("render_planes"),
                Metatile.RenderPlanes))
        {
            OutError = TEXT("Invalid R5 metatile fields.");
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
        if (!Object->TryGetArrayField(TEXT("entries"), Entries)
            || Entries == nullptr
            || Entries->Num() != 8)
        {
            OutError = TEXT("R5 metatile must contain eight entries.");
            return false;
        }

        Metatile.Entries.Reserve(8);

        for (const TSharedPtr<FJsonValue>& EntryValue : *Entries)
        {
            const TSharedPtr<FJsonObject> EntryObject =
                EntryValue->AsObject();
            if (!EntryObject.IsValid())
            {
                OutError = TEXT("R5 metatile contains invalid tile entry.");
                return false;
            }

            FRemasterRenderTileEntry Tile;
            if (!ReadInt(EntryObject, TEXT("entry_index"), Tile.EntryIndex)
                || !ReadInt(
                    EntryObject,
                    TEXT("source_layer"),
                    Tile.SourceLayer)
                || !ReadInt(
                    EntryObject,
                    TEXT("quadrant"),
                    Tile.Quadrant)
                || !ReadInt(EntryObject, TEXT("x"), Tile.X)
                || !ReadInt(EntryObject, TEXT("y"), Tile.Y)
                || !ReadInt(
                    EntryObject,
                    TEXT("tile_id_raw"),
                    Tile.TileIdRaw)
                || !ReadInt(
                    EntryObject,
                    TEXT("palette"),
                    Tile.Palette)
                || !ReadInt(
                    EntryObject,
                    TEXT("raw_u16"),
                    Tile.RawU16))
            {
                OutError = TEXT("Invalid R5 metatile tile entry.");
                return false;
            }

            EntryObject->TryGetBoolField(
                TEXT("h_flip"),
                Tile.bHFlip);
            EntryObject->TryGetBoolField(
                TEXT("v_flip"),
                Tile.bVFlip);

            Metatile.Entries.Add(Tile);
        }

        if (Metatile.LocalMetatileId
                != OutDescriptor.Metatiles.Num()
            || Metatile.RenderPlanes.Num() != 2)
        {
            OutError = FString::Printf(
                TEXT("Non-canonical R5 metatile order in %s"),
                *Entry.Id);
            return false;
        }

        OutDescriptor.Metatiles.Add(MoveTemp(Metatile));
    }

    if (!OutDescriptor.IsValid()
        || OutDescriptor.MetatileCount != Entry.MetatileCount)
    {
        OutError = FString::Printf(
            TEXT("Invalid R5 descriptor payload for %s"),
            *Entry.Id);
        OutDescriptor = FRemasterRenderTilesetDescriptor{};
        return false;
    }

    return true;
}
