#include "RemasterWorldCatalogSubsystem.h"

#include "Misc/Paths.h"

void URemasterWorldCatalogSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadCatalog();
}

FString URemasterWorldCatalogSubsystem::GetManifestPath() const
{
    return FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("Generated"),
        TEXT("World"),
        TEXT("manifest.json"));
}

bool URemasterWorldCatalogSubsystem::ReloadCatalog()
{
    FString Error;
    bCatalogReady = Catalog.LoadManifest(
        GetManifestPath(),
        Error);

    if (!bCatalogReady)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Remaster world catalog unavailable: %s"),
            *Error);
        return false;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Remaster world catalog loaded: %d maps"),
        Catalog.Entries().Num());

    return true;
}

bool URemasterWorldCatalogSubsystem::ResolveMapJson(
    int32 GroupNum,
    int32 MapNum,
    FString& OutAbsolutePath) const
{
    if (!bCatalogReady)
        return false;

    const FRemasterWorldCatalogEntry* Entry =
        Catalog.Find(GroupNum, MapNum);

    if (!Entry)
        return false;

    OutAbsolutePath = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("Generated"),
        TEXT("World"),
        Entry->RelativeFile);

    return true;
}

bool URemasterWorldCatalogSubsystem::LoadMap(
    int32 GroupNum,
    int32 MapNum,
    FRemasterMapIR& OutMap,
    FString& OutError) const
{
    FString AbsolutePath;

    if (!ResolveMapJson(
            GroupNum,
            MapNum,
            AbsolutePath))
    {
        OutError = FString::Printf(
            TEXT("Unknown numeric map address %d,%d"),
            GroupNum,
            MapNum);
        return false;
    }

    return FRemasterWorldData::LoadMapJson(
        AbsolutePath,
        OutMap,
        OutError);
}
