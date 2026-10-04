#include "RemasterRenderCatalogSubsystem.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString StringField(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Name)
{
    FString Value;
    Object->TryGetStringField(Name, Value);
    return Value;
}

int32 IntFieldDefault(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Name,
    int32 DefaultValue)
{
    double Value = 0.0;
    return Object->TryGetNumberField(Name, Value)
        ? static_cast<int32>(Value)
        : DefaultValue;
}

bool BoolField(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Name)
{
    bool Value = false;
    Object->TryGetBoolField(Name, Value);
    return Value;
}

bool IsHexSha256(const FString& Value)
{
    if (Value.Len() != 64)
        return false;

    for (const TCHAR Character : Value)
    {
        if (!FChar::IsHexDigit(Character))
            return false;
    }

    return true;
}

bool ParseStringArray(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Name,
    TArray<FString>& OutValues)
{
    OutValues.Reset();

    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object->TryGetArrayField(Name, Values) || Values == nullptr)
        return false;

    OutValues.Reserve(Values->Num());
    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        FString Text;
        if (!Value.IsValid()
            || !Value->TryGetString(Text)
            || Text.IsEmpty())
        {
            OutValues.Reset();
            return false;
        }

        OutValues.Add(MoveTemp(Text));
    }

    return true;
}

bool IsSafePackageRelative(const FString& RelativePath)
{
    if (RelativePath.IsEmpty()
        || !FPaths::IsRelative(RelativePath))
    {
        return false;
    }

    FString Normalized = RelativePath;
    FPaths::NormalizeFilename(Normalized);

    if (Normalized.StartsWith(TEXT("../"))
        || Normalized.Contains(TEXT("/../"))
        || Normalized.Equals(TEXT("..")))
    {
        return false;
    }

    return !Normalized.StartsWith(TEXT("/"));
}
}

void URemasterRenderCatalogSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!ReloadCatalog())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("R5 render catalog is unavailable: %s"),
            *GetManifestPath());
    }
}

FString URemasterRenderCatalogSubsystem::GetPackageRoot() const
{
    return FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("Generated"),
        TEXT("Render"));
}

FString URemasterRenderCatalogSubsystem::GetManifestPath() const
{
    return FPaths::Combine(
        GetPackageRoot(),
        TEXT("manifest.json"));
}

bool URemasterRenderCatalogSubsystem::ResolvePackageFile(
    const FString& RelativePath,
    FString& OutAbsolutePath) const
{
    OutAbsolutePath.Reset();

    if (!IsSafePackageRelative(RelativePath))
        return false;

    const FString PackageRoot =
        FPaths::ConvertRelativePathToFull(GetPackageRoot());

    FString Candidate =
        FPaths::ConvertRelativePathToFull(
            FPaths::Combine(
                PackageRoot,
                RelativePath));

    FPaths::NormalizeFilename(Candidate);

    FString NormalizedRoot = PackageRoot;
    FPaths::NormalizeFilename(NormalizedRoot);

    if (!Candidate.StartsWith(NormalizedRoot + TEXT("/"))
        && !Candidate.Equals(NormalizedRoot))
    {
        return false;
    }

    if (!FPaths::FileExists(Candidate))
        return false;

    OutAbsolutePath = MoveTemp(Candidate);
    return true;
}

bool URemasterRenderCatalogSubsystem::ReloadCatalog()
{
    bCatalogReady = false;
    SourceRepository.Reset();
    SourceCommit.Reset();
    ContentSha256.Reset();
    Entries.Reset();
    DescriptorCache.Reset();

    FString JsonText;
    if (!FFileHelper::LoadFileToString(
            JsonText,
            *GetManifestPath()))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Unable to read R5 render manifest: %s"),
            *GetManifestPath());
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonText);

    if (!FJsonSerializer::Deserialize(Reader, Root)
        || !Root.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 render manifest is not valid JSON."));
        return false;
    }

    if (IntFieldDefault(Root, TEXT("schema_version"), -1) != 1)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Unsupported R5 render manifest schema."));
        return false;
    }

    SourceRepository =
        StringField(Root, TEXT("source_repository"));
    SourceCommit =
        StringField(Root, TEXT("source_commit"));
    ContentSha256 =
        StringField(Root, TEXT("content_sha256"));

    if (SourceRepository.IsEmpty()
        || !IsHexSha256(SourceCommit)
        || !IsHexSha256(ContentSha256))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 render manifest provenance is invalid."));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Tilesets = nullptr;
    if (!Root->TryGetArrayField(TEXT("tilesets"), Tilesets)
        || Tilesets == nullptr
        || Tilesets->IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 render manifest has no tilesets."));
        return false;
    }

    const int32 ExpectedTilesetCount =
        IntFieldDefault(Root, TEXT("tileset_count"), -1);

    for (const TSharedPtr<FJsonValue>& Value : *Tilesets)
    {
        const TSharedPtr<FJsonObject> Object =
            Value.IsValid() ? Value->AsObject() : nullptr;

        if (!Object.IsValid())
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("R5 render manifest contains a non-object tileset."));
            Entries.Reset();
            return false;
        }

        FRemasterRenderCatalogEntry Entry;
        Entry.Id = StringField(Object, TEXT("id"));
        Entry.bIsSecondary =
            BoolField(Object, TEXT("is_secondary"));
        Entry.DescriptorFile =
            StringField(Object, TEXT("descriptor_file"));
        Entry.DescriptorSha256 =
            StringField(Object, TEXT("descriptor_sha256"));
        Entry.TilesPngFile =
            StringField(Object, TEXT("tiles_png_file"));
        Entry.TilesPngSha256 =
            StringField(Object, TEXT("tiles_png_sha256"));
        Entry.TilesIndex8File =
            StringField(Object, TEXT("tiles_index8_file"));
        Entry.TilesIndex8Sha256 =
            StringField(Object, TEXT("tiles_index8_sha256"));
        Entry.PaletteLutFile =
            StringField(Object, TEXT("palette_lut_file"));
        Entry.PaletteLutSha256 =
            StringField(Object, TEXT("palette_lut_sha256"));
        Entry.MetatileCount =
            IntFieldDefault(Object, TEXT("metatile_count"), -1);

        if (!ParseStringArray(
                Object,
                TEXT("palette_files"),
                Entry.PaletteFiles)
            || Entry.Id.IsEmpty()
            || !IsSafePackageRelative(Entry.DescriptorFile)
            || !IsSafePackageRelative(Entry.TilesPngFile)
            || !IsSafePackageRelative(Entry.TilesIndex8File)
            || !IsHexSha256(Entry.DescriptorSha256)
            || !IsHexSha256(Entry.TilesPngSha256)
            || !IsHexSha256(Entry.TilesIndex8Sha256)
            || !IsSafePackageRelative(Entry.PaletteLutFile)
            || !IsHexSha256(Entry.PaletteLutSha256)
            || Entry.PaletteFiles.Num() != 16
            || Entry.MetatileCount <= 0)
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("R5 render manifest has invalid tileset entry: %s"),
                *Entry.Id);
            Entries.Reset();
            return false;
        }

        for (const FString& Palette : Entry.PaletteFiles)
        {
            if (!IsSafePackageRelative(Palette))
            {
                UE_LOG(
                    LogTemp,
                    Error,
                    TEXT("Unsafe R5 palette path for %s: %s"),
                    *Entry.Id,
                    *Palette);
                Entries.Reset();
                return false;
            }
        }

        if (Entries.Contains(Entry.Id))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("Duplicate R5 tileset identity: %s"),
                *Entry.Id);
            Entries.Reset();
            return false;
        }

        FString Resolved;
        if (!ResolvePackageFile(Entry.DescriptorFile, Resolved)
            || !ResolvePackageFile(Entry.TilesPngFile, Resolved)
            || !ResolvePackageFile(Entry.TilesIndex8File, Resolved)
            || !ResolvePackageFile(Entry.PaletteLutFile, Resolved))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("R5 tileset payload is missing for %s"),
                *Entry.Id);
            Entries.Reset();
            return false;
        }

        for (const FString& Palette : Entry.PaletteFiles)
        {
            if (!ResolvePackageFile(Palette, Resolved))
            {
                UE_LOG(
                    LogTemp,
                    Error,
                    TEXT("R5 tileset palette is missing for %s: %s"),
                    *Entry.Id,
                    *Palette);
                Entries.Reset();
                return false;
            }
        }

        Entries.Add(Entry.Id, MoveTemp(Entry));
    }

    if (ExpectedTilesetCount != Entries.Num())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("R5 render manifest tileset count mismatch: expected=%d actual=%d"),
            ExpectedTilesetCount,
            Entries.Num());
        Entries.Reset();
        return false;
    }

    bCatalogReady = true;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("R5 render catalog loaded: %d tilesets, content=%s"),
        Entries.Num(),
        *ContentSha256);

    return true;
}

const FRemasterRenderCatalogEntry*
URemasterRenderCatalogSubsystem::FindTileset(
    const FString& TilesetId) const
{
    if (!bCatalogReady || TilesetId.IsEmpty())
        return nullptr;

    return Entries.Find(TilesetId);
}

bool URemasterRenderCatalogSubsystem::LoadDescriptor(
    const FRemasterRenderCatalogEntry& Entry,
    FRemasterTilesetRenderDescriptor& OutDescriptor,
    FString& OutError) const
{
    OutDescriptor = FRemasterTilesetRenderDescriptor{};
    OutError.Reset();

    FString AbsoluteDescriptor;
    if (!ResolvePackageFile(
            Entry.DescriptorFile,
            AbsoluteDescriptor))
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor is missing: %s"),
            *Entry.DescriptorFile);
        return false;
    }

    FString JsonText;
    if (!FFileHelper::LoadFileToString(
            JsonText,
            *AbsoluteDescriptor))
    {
        OutError = FString::Printf(
            TEXT("Unable to read R5 descriptor: %s"),
            *AbsoluteDescriptor);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonText);

    if (!FJsonSerializer::Deserialize(Reader, Root)
        || !Root.IsValid())
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor is not valid JSON: %s"),
            *Entry.Id);
        return false;
    }

    if (IntFieldDefault(Root, TEXT("schema_version"), -1) != 1)
    {
        OutError = FString::Printf(
            TEXT("Unsupported R5 descriptor schema: %s"),
            *Entry.Id);
        return false;
    }

    OutDescriptor.Id = StringField(Root, TEXT("id"));
    OutDescriptor.bIsSecondary =
        BoolField(Root, TEXT("is_secondary"));
    OutDescriptor.MetatileAssetRootSource =
        StringField(Root, TEXT("metatile_asset_root_source"));
    OutDescriptor.VisualAssetRootSource =
        StringField(Root, TEXT("visual_asset_root_source"));
    OutDescriptor.TileSymbol =
        StringField(Root, TEXT("tile_symbol"));
    OutDescriptor.PaletteSymbol =
        StringField(Root, TEXT("palette_symbol"));
    OutDescriptor.TilesPngRelative =
        StringField(Root, TEXT("tiles_png"));
    OutDescriptor.TilesIndex8Relative =
        StringField(Root, TEXT("tiles_index8"));
    OutDescriptor.PaletteLutRelative =
        StringField(Root, TEXT("palette_lut_file"));
    OutDescriptor.PaletteLutSha256 =
        StringField(Root, TEXT("palette_lut_sha256"));
    OutDescriptor.PaletteLutWidth =
        IntFieldDefault(Root, TEXT("palette_lut_width"), -1);
    OutDescriptor.PaletteLutHeight =
        IntFieldDefault(Root, TEXT("palette_lut_height"), -1);
    OutDescriptor.TilesPngWidth =
        IntFieldDefault(Root, TEXT("tiles_png_width"), -1);
    OutDescriptor.TilesPngHeight =
        IntFieldDefault(Root, TEXT("tiles_png_height"), -1);
    OutDescriptor.TileCount =
        IntFieldDefault(Root, TEXT("tile_count"), -1);
    OutDescriptor.MetatileCount =
        IntFieldDefault(Root, TEXT("metatile_count"), -1);

    if (!ParseStringArray(
            Root,
            TEXT("palette_files"),
            OutDescriptor.PaletteFilesRelative)
        || OutDescriptor.Id != Entry.Id
        || OutDescriptor.bIsSecondary != Entry.bIsSecondary
        || OutDescriptor.MetatileCount != Entry.MetatileCount
        || !IsSafePackageRelative(OutDescriptor.TilesPngRelative)
        || !IsSafePackageRelative(OutDescriptor.TilesIndex8Relative)
        || !IsSafePackageRelative(OutDescriptor.PaletteLutRelative)
        || !IsHexSha256(OutDescriptor.PaletteLutSha256)
        || OutDescriptor.PaletteLutWidth != 16
        || OutDescriptor.PaletteLutHeight != 16
        || OutDescriptor.PaletteFilesRelative.Num() != 16)
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor header mismatch: %s"),
            *Entry.Id);
        return false;
    }

    FString Resolved;
    if (!ResolvePackageFile(
            OutDescriptor.TilesPngRelative,
            Resolved))
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor tile sheet is missing: %s"),
            *Entry.Id);
        return false;
    }

    if (!ResolvePackageFile(
            OutDescriptor.TilesIndex8Relative,
            Resolved))
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor index texture is missing: %s"),
            *Entry.Id);
        return false;
    }

    if (!ResolvePackageFile(
            OutDescriptor.PaletteLutRelative,
            Resolved))
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor palette LUT is missing: %s"),
            *Entry.Id);
        return false;
    }

    for (const FString& Palette : OutDescriptor.PaletteFilesRelative)
    {
        if (!IsSafePackageRelative(Palette)
            || !ResolvePackageFile(Palette, Resolved))
        {
            OutError = FString::Printf(
                TEXT("R5 descriptor palette is missing/unsafe: %s"),
                *Entry.Id);
            return false;
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Metatiles = nullptr;
    if (!Root->TryGetArrayField(TEXT("metatiles"), Metatiles)
        || Metatiles == nullptr)
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor has no metatiles: %s"),
            *Entry.Id);
        return false;
    }

    OutDescriptor.Metatiles.Reserve(Metatiles->Num());

    for (const TSharedPtr<FJsonValue>& Value : *Metatiles)
    {
        const TSharedPtr<FJsonObject> Object =
            Value.IsValid() ? Value->AsObject() : nullptr;

        if (!Object.IsValid())
        {
            OutError = FString::Printf(
                TEXT("R5 descriptor has non-object metatile: %s"),
                *Entry.Id);
            return false;
        }

        FRemasterMetatileRenderDescriptor Metatile;
        Metatile.LocalMetatileId =
            IntFieldDefault(Object, TEXT("local_metatile_id"), -1);
        Metatile.Behavior =
            IntFieldDefault(Object, TEXT("behavior"), -1);
        Metatile.LayerType =
            IntFieldDefault(Object, TEXT("layer_type"), -1);

        if (!ParseStringArray(
                Object,
                TEXT("render_planes"),
                Metatile.RenderPlanes))
        {
            OutError = FString::Printf(
                TEXT("R5 metatile render planes are invalid: %s/%d"),
                *Entry.Id,
                Metatile.LocalMetatileId);
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* EntriesJson = nullptr;
        if (!Object->TryGetArrayField(TEXT("entries"), EntriesJson)
            || EntriesJson == nullptr)
        {
            OutError = FString::Printf(
                TEXT("R5 metatile entries missing: %s/%d"),
                *Entry.Id,
                Metatile.LocalMetatileId);
            return false;
        }

        Metatile.Entries.Reserve(EntriesJson->Num());

        for (const TSharedPtr<FJsonValue>& EntryValue : *EntriesJson)
        {
            const TSharedPtr<FJsonObject> EntryObject =
                EntryValue.IsValid()
                    ? EntryValue->AsObject()
                    : nullptr;

            if (!EntryObject.IsValid())
            {
                OutError = FString::Printf(
                    TEXT("R5 tile entry is invalid: %s/%d"),
                    *Entry.Id,
                    Metatile.LocalMetatileId);
                return false;
            }

            FRemasterRenderTileEntry Tile;
            Tile.EntryIndex =
                IntFieldDefault(EntryObject, TEXT("entry_index"), -1);
            Tile.SourceLayer =
                IntFieldDefault(EntryObject, TEXT("source_layer"), -1);
            Tile.Quadrant =
                IntFieldDefault(EntryObject, TEXT("quadrant"), -1);
            Tile.X =
                IntFieldDefault(EntryObject, TEXT("x"), -1);
            Tile.Y =
                IntFieldDefault(EntryObject, TEXT("y"), -1);
            Tile.TileIdRaw =
                IntFieldDefault(EntryObject, TEXT("tile_id_raw"), -1);
            Tile.bHFlip =
                BoolField(EntryObject, TEXT("h_flip"));
            Tile.bVFlip =
                BoolField(EntryObject, TEXT("v_flip"));
            Tile.Palette =
                IntFieldDefault(EntryObject, TEXT("palette"), -1);
            Tile.RawU16 =
                IntFieldDefault(EntryObject, TEXT("raw_u16"), -1);

            const int32 Reconstructed =
                Tile.TileIdRaw
                | (Tile.bHFlip ? 0x0400 : 0)
                | (Tile.bVFlip ? 0x0800 : 0)
                | (Tile.Palette << 12);

            if (!Tile.IsValid()
                || Reconstructed != Tile.RawU16)
            {
                OutError = FString::Printf(
                    TEXT("R5 tile entry integrity failure: %s/%d/%d"),
                    *Entry.Id,
                    Metatile.LocalMetatileId,
                    Tile.EntryIndex);
                return false;
            }

            Metatile.Entries.Add(MoveTemp(Tile));
        }

        if (!Metatile.IsValid())
        {
            OutError = FString::Printf(
                TEXT("R5 metatile integrity failure: %s/%d"),
                *Entry.Id,
                Metatile.LocalMetatileId);
            return false;
        }

        if (Metatile.LocalMetatileId
            != OutDescriptor.Metatiles.Num())
        {
            OutError = FString::Printf(
                TEXT("R5 metatile ids are not dense: %s/%d"),
                *Entry.Id,
                Metatile.LocalMetatileId);
            return false;
        }

        OutDescriptor.Metatiles.Add(MoveTemp(Metatile));
    }

    if (!OutDescriptor.IsValid())
    {
        OutError = FString::Printf(
            TEXT("R5 descriptor failed integrity validation: %s"),
            *Entry.Id);
        return false;
    }

    return true;
}

bool URemasterRenderCatalogSubsystem::LoadTileset(
    const FString& TilesetId,
    const FRemasterTilesetRenderDescriptor*& OutDescriptor,
    FString& OutError)
{
    OutDescriptor = nullptr;
    OutError.Reset();

    if (!bCatalogReady)
    {
        OutError = TEXT("R5 render catalog is not ready.");
        return false;
    }

    const FRemasterRenderCatalogEntry* Entry =
        Entries.Find(TilesetId);

    if (!Entry)
    {
        OutError = FString::Printf(
            TEXT("Unknown exact R5 tileset identity: %s"),
            *TilesetId);
        return false;
    }

    if (const TSharedPtr<FRemasterTilesetRenderDescriptor>* Cached =
            DescriptorCache.Find(TilesetId))
    {
        if (Cached->IsValid())
        {
            OutDescriptor = Cached->Get();
            return true;
        }
    }

    TSharedPtr<FRemasterTilesetRenderDescriptor> Descriptor =
        MakeShared<FRemasterTilesetRenderDescriptor>();

    if (!LoadDescriptor(*Entry, *Descriptor, OutError))
        return false;

    DescriptorCache.Add(TilesetId, Descriptor);
    OutDescriptor = Descriptor.Get();
    return true;
}

bool URemasterRenderCatalogSubsystem::ResolveMetatile(
    const FString& TilesetId,
    int32 LocalMetatileId,
    const FRemasterTilesetRenderDescriptor*& OutTileset,
    const FRemasterMetatileRenderDescriptor*& OutMetatile,
    FString& OutError)
{
    OutTileset = nullptr;
    OutMetatile = nullptr;
    OutError.Reset();

    if (!LoadTileset(
            TilesetId,
            OutTileset,
            OutError)
        || !OutTileset)
    {
        return false;
    }

    if (LocalMetatileId < 0
        || !OutTileset->Metatiles.IsValidIndex(LocalMetatileId))
    {
        OutError = FString::Printf(
            TEXT("R5 metatile id %d is outside tileset %s"),
            LocalMetatileId,
            *TilesetId);
        OutTileset = nullptr;
        return false;
    }

    OutMetatile =
        &OutTileset->Metatiles[LocalMetatileId];
    return true;
}
