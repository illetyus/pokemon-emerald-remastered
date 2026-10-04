#include "RemasterRenderResourceSubsystem.h"

#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "RemasterRenderCatalogSubsystem.h"

namespace
{
bool ParseJascPalette(
    const FString& Text,
    TArray<FColor>& OutColors,
    FString& OutError)
{
    TArray<FString> Lines;
    Text.ParseIntoArrayLines(Lines, true);

    TArray<FString> Clean;
    for (FString Line : Lines)
    {
        Line.TrimStartAndEndInline();
        if (!Line.IsEmpty())
            Clean.Add(MoveTemp(Line));
    }

    if (Clean.Num() != 19
        || Clean[0] != TEXT("JASC-PAL")
        || Clean[1] != TEXT("0100")
        || Clean[2] != TEXT("16"))
    {
        OutError = TEXT("R5 palette is not JASC-PAL 0100 with 16 colors.");
        return false;
    }

    OutColors.Reset();
    OutColors.Reserve(16);

    for (int32 Index = 3; Index < 19; ++Index)
    {
        TArray<FString> Parts;
        Clean[Index].ParseIntoArrayWS(Parts);

        if (Parts.Num() != 3)
        {
            OutError = FString::Printf(
                TEXT("R5 palette row %d does not contain RGB."),
                Index - 3);
            OutColors.Reset();
            return false;
        }

        int32 R = 0;
        int32 G = 0;
        int32 B = 0;

        if (!LexTryParseString(R, *Parts[0])
            || !LexTryParseString(G, *Parts[1])
            || !LexTryParseString(B, *Parts[2])
            || R < 0 || R > 255
            || G < 0 || G > 255
            || B < 0 || B > 255)
        {
            OutError = FString::Printf(
                TEXT("R5 palette row %d has invalid RGB."),
                Index - 3);
            OutColors.Reset();
            return false;
        }

        OutColors.Add(
            FColor(
                static_cast<uint8>(R),
                static_cast<uint8>(G),
                static_cast<uint8>(B),
                255));
    }

    return OutColors.Num() == 16;
}

bool CopyTextureBytes(
    UTexture2D* Texture,
    const void* Source,
    int64 ByteCount,
    FString& OutError)
{
    if (!Texture
        || !Texture->GetPlatformData()
        || Texture->GetPlatformData()->Mips.Num() != 1
        || Source == nullptr
        || ByteCount <= 0)
    {
        OutError = TEXT("R5 transient texture storage is invalid.");
        return false;
    }

    FTexture2DMipMap& Mip =
        Texture->GetPlatformData()->Mips[0];

    void* Destination =
        Mip.BulkData.Lock(LOCK_READ_WRITE);

    if (!Destination
        || Mip.BulkData.GetBulkDataSize() != ByteCount)
    {
        if (Destination)
            Mip.BulkData.Unlock();

        OutError = TEXT("R5 transient texture byte size mismatch.");
        return false;
    }

    FMemory::Memcpy(
        Destination,
        Source,
        static_cast<SIZE_T>(ByteCount));

    Mip.BulkData.Unlock();
    Texture->UpdateResource();
    return true;
}
}

void URemasterRenderResourceSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<
        URemasterRenderCatalogSubsystem>();

    Super::Initialize(Collection);
}

void URemasterRenderResourceSubsystem::Deinitialize()
{
    ResourceCache.Reset();
    IndexTextureCache.Reset();
    PaletteTextureCache.Reset();

    Super::Deinitialize();
}

FString URemasterRenderResourceSubsystem::MakeCacheKey(
    const FString& TilesetId,
    const FString& DescriptorSha256,
    const FString& IndexSha256) const
{
    return FString::Printf(
        TEXT("%s:%s:%s"),
        *TilesetId,
        *DescriptorSha256,
        *IndexSha256);
}

UTexture2D*
URemasterRenderResourceSubsystem::CreateIndexTexture(
    const FString& DebugName,
    int32 Width,
    int32 Height,
    const TArray<uint8>& Pixels,
    FString& OutError)
{
    if (Width <= 0
        || Height <= 0
        || Pixels.Num() != Width * Height)
    {
        OutError = FString::Printf(
            TEXT("R5 index texture dimensions mismatch: %s"),
            *DebugName);
        return nullptr;
    }

    for (const uint8 Pixel : Pixels)
    {
        if (Pixel > 15u)
        {
            OutError = FString::Printf(
                TEXT("R5 index texture contains palette index >15: %s"),
                *DebugName);
            return nullptr;
        }
    }

    UTexture2D* Texture =
        UTexture2D::CreateTransient(
            Width,
            Height,
            PF_G8,
            *DebugName);

    if (!Texture)
    {
        OutError = FString::Printf(
            TEXT("Failed to allocate R5 index texture: %s"),
            *DebugName);
        return nullptr;
    }

    Texture->SRGB = false;
    Texture->Filter = TF_Nearest;
    Texture->NeverStream = true;
    Texture->AddressX = TA_Clamp;
    Texture->AddressY = TA_Clamp;

    if (!CopyTextureBytes(
            Texture,
            Pixels.GetData(),
            Pixels.Num(),
            OutError))
    {
        return nullptr;
    }

    return Texture;
}

UTexture2D*
URemasterRenderResourceSubsystem::CreatePaletteTexture(
    const FString& DebugName,
    const TArray<FColor>& Colors,
    FString& OutError)
{
    constexpr int32 PaletteWidth = 16;
    constexpr int32 PaletteHeight = 16;

    if (Colors.Num() != PaletteWidth * PaletteHeight)
    {
        OutError = FString::Printf(
            TEXT("R5 palette texture requires 256 colors: %s"),
            *DebugName);
        return nullptr;
    }

    UTexture2D* Texture =
        UTexture2D::CreateTransient(
            PaletteWidth,
            PaletteHeight,
            PF_B8G8R8A8,
            *DebugName);

    if (!Texture)
    {
        OutError = FString::Printf(
            TEXT("Failed to allocate R5 palette texture: %s"),
            *DebugName);
        return nullptr;
    }

    Texture->SRGB = true;
    Texture->Filter = TF_Nearest;
    Texture->NeverStream = true;
    Texture->AddressX = TA_Clamp;
    Texture->AddressY = TA_Clamp;

    TArray<FColor> BgraColors;
    BgraColors.Reserve(Colors.Num());

    for (const FColor& Color : Colors)
    {
        /*
         * PF_B8G8R8A8 consumes FColor's native BGRA byte layout.
         */
        BgraColors.Add(Color);
    }

    if (!CopyTextureBytes(
            Texture,
            BgraColors.GetData(),
            static_cast<int64>(BgraColors.Num())
                * sizeof(FColor),
            OutError))
    {
        return nullptr;
    }

    return Texture;
}

bool URemasterRenderResourceSubsystem::LoadPaletteColors(
    const TArray<FString>& RelativePaletteFiles,
    TArray<FColor>& OutColors,
    FString& OutError) const
{
    OutColors.Reset();
    OutError.Reset();

    if (RelativePaletteFiles.Num() != 16)
    {
        OutError = TEXT("R5 tileset does not reference 16 palettes.");
        return false;
    }

    UGameInstance* GI = GetGameInstance();
    URemasterRenderCatalogSubsystem* Catalog =
        GI
            ? GI->GetSubsystem<URemasterRenderCatalogSubsystem>()
            : nullptr;

    if (!Catalog || !Catalog->IsCatalogReady())
    {
        OutError = TEXT("R5 render catalog is unavailable.");
        return false;
    }

    OutColors.Reserve(256);

    for (int32 PaletteIndex = 0;
         PaletteIndex < RelativePaletteFiles.Num();
         ++PaletteIndex)
    {
        FString AbsolutePath;
        if (!Catalog->ResolvePackageFile(
                RelativePaletteFiles[PaletteIndex],
                AbsolutePath))
        {
            OutError = FString::Printf(
                TEXT("R5 palette payload is unavailable: %s"),
                *RelativePaletteFiles[PaletteIndex]);
            OutColors.Reset();
            return false;
        }

        FString Text;
        if (!FFileHelper::LoadFileToString(
                Text,
                *AbsolutePath))
        {
            OutError = FString::Printf(
                TEXT("Unable to read R5 palette: %s"),
                *AbsolutePath);
            OutColors.Reset();
            return false;
        }

        TArray<FColor> PaletteColors;
        if (!ParseJascPalette(
                Text,
                PaletteColors,
                OutError))
        {
            OutError = FString::Printf(
                TEXT("%s (%s)"),
                *OutError,
                *AbsolutePath);
            OutColors.Reset();
            return false;
        }

        OutColors.Append(PaletteColors);
    }

    return OutColors.Num() == 256;
}

bool URemasterRenderResourceSubsystem::LoadTilesetResources(
    const FString& TilesetId,
    FRemasterTilesetRenderResources& OutResources,
    FString& OutError)
{
    OutResources = FRemasterTilesetRenderResources{};
    OutError.Reset();

    UGameInstance* GI = GetGameInstance();
    URemasterRenderCatalogSubsystem* Catalog =
        GI
            ? GI->GetSubsystem<URemasterRenderCatalogSubsystem>()
            : nullptr;

    if (!Catalog || !Catalog->IsCatalogReady())
    {
        OutError = TEXT("R5 render catalog is unavailable.");
        return false;
    }

    const FRemasterRenderCatalogEntry* Entry =
        Catalog->FindTileset(TilesetId);
    const FRemasterTilesetRenderDescriptor* Descriptor = nullptr;

    if (!Entry
        || !Catalog->LoadTileset(
            TilesetId,
            Descriptor,
            OutError)
        || !Descriptor)
    {
        if (OutError.IsEmpty())
        {
            OutError = FString::Printf(
                TEXT("Unknown R5 tileset: %s"),
                *TilesetId);
        }
        return false;
    }

    const FString CacheKey =
        MakeCacheKey(
            TilesetId,
            Entry->DescriptorSha256,
            Entry->TilesIndex8Sha256);

    if (const FRemasterTilesetRenderResources* Cached =
            ResourceCache.Find(CacheKey))
    {
        if (Cached->IsValid())
        {
            OutResources = *Cached;
            return true;
        }
    }

    FString AbsoluteIndex;
    if (!Catalog->ResolvePackageFile(
            Descriptor->TilesIndex8Relative,
            AbsoluteIndex))
    {
        OutError = FString::Printf(
            TEXT("R5 index texture payload is missing: %s"),
            *TilesetId);
        return false;
    }

    TArray<uint8> IndexPixels;
    if (!FFileHelper::LoadFileToArray(
            IndexPixels,
            *AbsoluteIndex)
        || IndexPixels.Num()
            != Descriptor->TilesPngWidth
                * Descriptor->TilesPngHeight)
    {
        OutError = FString::Printf(
            TEXT("R5 index texture byte count mismatch: %s"),
            *TilesetId);
        return false;
    }

    TArray<FColor> PaletteColors;
    if (!LoadPaletteColors(
            Descriptor->PaletteFilesRelative,
            PaletteColors,
            OutError))
    {
        return false;
    }

    UTexture2D* IndexTexture =
        CreateIndexTexture(
            FString::Printf(
                TEXT("R5_Index_%s"),
                *TilesetId),
            Descriptor->TilesPngWidth,
            Descriptor->TilesPngHeight,
            IndexPixels,
            OutError);

    if (!IndexTexture)
        return false;

    UTexture2D* PaletteTexture =
        CreatePaletteTexture(
            FString::Printf(
                TEXT("R5_Palettes_%s"),
                *TilesetId),
            PaletteColors,
            OutError);

    if (!PaletteTexture)
        return false;

    IndexTextureCache.Add(CacheKey, IndexTexture);
    PaletteTextureCache.Add(CacheKey, PaletteTexture);

    FRemasterTilesetRenderResources Resources;
    Resources.TileIndexTexture = IndexTexture;
    Resources.PaletteTexture = PaletteTexture;
    Resources.TileSheetWidth = Descriptor->TilesPngWidth;
    Resources.TileSheetHeight = Descriptor->TilesPngHeight;
    Resources.TilesPerRow =
        Descriptor->TilesPngWidth / 8;

    if (!Resources.IsValid()
        || Descriptor->TilesPngWidth % 8 != 0
        || Descriptor->TilesPngHeight % 8 != 0)
    {
        IndexTextureCache.Remove(CacheKey);
        PaletteTextureCache.Remove(CacheKey);

        OutError = FString::Printf(
            TEXT("R5 tileset resource geometry is invalid: %s"),
            *TilesetId);
        return false;
    }

    ResourceCache.Add(CacheKey, Resources);
    OutResources = Resources;
    return true;
}
