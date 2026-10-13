#include "RemasterRenderResourceSubsystem.h"
#include "RemasterIndexedMapMath.h"

#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "RemasterRenderCatalogSubsystem.h"

namespace
{
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
    const FString& IndexSha256,
    const FString& PaletteLutSha256) const
{
    return FString::Printf(
        TEXT("%s:%s:%s:%s"),
        *TilesetId,
        *DescriptorSha256,
        *IndexSha256,
        *PaletteLutSha256);
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
    const TArray<uint8>& RgbaPixels,
    FString& OutError)
{
    constexpr int32 PaletteWidth = 16;
    constexpr int32 PaletteHeight = 16;
    constexpr int32 Channels = 4;
    constexpr int32 ExpectedBytes =
        PaletteWidth * PaletteHeight * Channels;

    if (RgbaPixels.Num() != ExpectedBytes)
    {
        OutError = FString::Printf(
            TEXT("R5 palette LUT requires %d RGBA bytes: %s"),
            ExpectedBytes,
            *DebugName);
        return nullptr;
    }

    TArray<FColor> Colors;
    Colors.SetNumUninitialized(PaletteWidth * PaletteHeight);

    for (int32 Index = 0; Index < Colors.Num(); ++Index)
    {
        const int32 Source = Index * Channels;
        Colors[Index] = FColor(
            RgbaPixels[Source + 0],
            RgbaPixels[Source + 1],
            RgbaPixels[Source + 2],
            RgbaPixels[Source + 3]);
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

    if (!CopyTextureBytes(
            Texture,
            Colors.GetData(),
            static_cast<int64>(Colors.Num()) * sizeof(FColor),
            OutError))
    {
        return nullptr;
    }

    return Texture;
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
            Entry->TilesIndex8Sha256,
            Entry->PaletteLutSha256);

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
    FString AbsolutePaletteLut;

    if (!Catalog->ResolvePackageFile(
            Descriptor->TilesIndex8Relative,
            AbsoluteIndex)
        || !Catalog->ResolvePackageFile(
            Descriptor->PaletteLutRelative,
            AbsolutePaletteLut))
    {
        OutError = FString::Printf(
            TEXT("R5 indexed render payload is missing: %s"),
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

    TArray<uint8> PaletteBytes;
    if (!FFileHelper::LoadFileToArray(
            PaletteBytes,
            *AbsolutePaletteLut)
        || PaletteBytes.Num() != 16 * 16 * 4)
    {
        OutError = FString::Printf(
            TEXT("R5 palette LUT byte count mismatch: %s"),
            *TilesetId);
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
            PaletteBytes,
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


bool URemasterRenderResourceSubsystem::LoadMapResources(
    const FString& PrimaryId,const FString& SecondaryId,
    FRemasterTilesetRenderResources& OutResources,FString& OutError)
{
    OutResources={};OutError.Reset();
    auto* Catalog=GetGameInstance()->GetSubsystem<URemasterRenderCatalogSubsystem>();
    const FRemasterTilesetRenderDescriptor* Descriptor=nullptr;
    if(!Catalog || !Catalog->LoadTileset(PrimaryId,Descriptor,OutError) || !Descriptor)return false;
    const auto Primary=*Descriptor;
    if(!Catalog->LoadTileset(SecondaryId,Descriptor,OutError) || !Descriptor)return false;
    const auto Secondary=*Descriptor;
    const auto* P=Catalog->FindTileset(PrimaryId);const auto* S=Catalog->FindTileset(SecondaryId);
    if(!P || !S || P->bIsSecondary || !S->bIsSecondary){OutError=TEXT("R5 map requires explicit primary/secondary tileset domains.");return false;}
    const FString Key=TEXT("Pair|")+MakeCacheKey(PrimaryId,P->DescriptorSha256,P->TilesIndex8Sha256,P->PaletteLutSha256)
        +TEXT("|")+MakeCacheKey(SecondaryId,S->DescriptorSha256,S->TilesIndex8Sha256,S->PaletteLutSha256);
    if(const auto* Cached=ResourceCache.Find(Key)){OutResources=*Cached;return OutResources.IsValid();}
    const auto Read=[&](const FString& Relative,TArray<uint8>& Bytes){FString Absolute;
        return Catalog->ResolvePackageFile(Relative,Absolute)&&FFileHelper::LoadFileToArray(Bytes,*Absolute);};
    TArray<uint8> PrimaryIndex,SecondaryIndex,PrimaryPalette,SecondaryPalette,Atlas,Palette;
    if(!Read(Primary.TilesIndex8Relative,PrimaryIndex)||!Read(Secondary.TilesIndex8Relative,SecondaryIndex)
       ||!Read(Primary.PaletteLutRelative,PrimaryPalette)||!Read(Secondary.PaletteLutRelative,SecondaryPalette)
       ||PrimaryPalette.Num()!=1024||SecondaryPalette.Num()!=1024){OutError=TEXT("R5 paired binary render payload is missing or malformed.");return false;}
    Atlas.SetNumZeroed(RemasterIndexedMap::AtlasWidth*RemasterIndexedMap::AtlasHeight);
    if(!RemasterIndexedMap::CopyIndex(PrimaryIndex.GetData(),PrimaryIndex.Num(),Primary.TilesPngWidth,Primary.TilesPngHeight,0,Atlas.GetData(),Atlas.Num())
       ||!RemasterIndexedMap::CopyIndex(SecondaryIndex.GetData(),SecondaryIndex.Num(),Secondary.TilesPngWidth,Secondary.TilesPngHeight,RemasterIndexedMap::PrimaryTiles,Atlas.GetData(),Atlas.Num())) {
        OutError=TEXT("R5 indexed map cannot match source VRAM tile partitions.");return false;
    }
    Palette.SetNumUninitialized(1024);
    constexpr int PrimaryPaletteBytes=RemasterIndexedMap::PrimaryPalettes*16*4;
    FMemory::Memcpy(Palette.GetData(),PrimaryPalette.GetData(),PrimaryPaletteBytes);
    FMemory::Memcpy(Palette.GetData()+PrimaryPaletteBytes,SecondaryPalette.GetData()+PrimaryPaletteBytes,1024-PrimaryPaletteBytes);
    FRemasterTilesetRenderResources Combined;
    Combined.TileIndexTexture=CreateIndexTexture(TEXT("R5_MapIndex_" )+PrimaryId+TEXT("_")+SecondaryId,RemasterIndexedMap::AtlasWidth,RemasterIndexedMap::AtlasHeight,Atlas,OutError);
    Combined.PaletteTexture=CreatePaletteTexture(TEXT("R5_MapPalette_")+PrimaryId+TEXT("_")+SecondaryId,Palette,OutError);
    if(!Combined.TileIndexTexture || !Combined.PaletteTexture)return false;
    Combined.TileSheetWidth=RemasterIndexedMap::AtlasWidth;Combined.TileSheetHeight=RemasterIndexedMap::AtlasHeight;
    Combined.TilesPerRow=RemasterIndexedMap::AtlasWidth/RemasterIndexedMap::TilePixels;
    Combined.PrimaryTileCount=FMath::Min(RemasterIndexedMap::PrimaryTiles,Primary.TilesPngWidth*Primary.TilesPngHeight/64);
    Combined.SecondaryTileCount=FMath::Min(RemasterIndexedMap::PrimaryTiles,Secondary.TilesPngWidth*Secondary.TilesPngHeight/64);
    IndexTextureCache.Add(Key,Combined.TileIndexTexture);PaletteTextureCache.Add(Key,Combined.PaletteTexture);
    ResourceCache.Add(Key,Combined);OutResources=Combined;return Combined.IsValid();
}
