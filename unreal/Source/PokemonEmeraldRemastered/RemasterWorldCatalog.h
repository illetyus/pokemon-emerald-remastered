#pragma once

#include "CoreMinimal.h"

struct FRemasterWorldCatalogEntry
{
    FString Id;
    FString Name;
    FString RelativeFile;
    int32 Width = 0;
    int32 Height = 0;
    int32 GroupNum = -1;
    int32 MapNum = -1;
};

class FRemasterWorldCatalog
{
public:
    bool LoadManifest(
        const FString& AbsoluteManifestPath,
        FString& OutError);

    const FRemasterWorldCatalogEntry* Find(
        int32 GroupNum,
        int32 MapNum) const;

    const TArray<FRemasterWorldCatalogEntry>& Entries() const
    {
        return CatalogEntries;
    }

private:
    static uint32 NumericKey(int32 GroupNum, int32 MapNum);

    TArray<FRemasterWorldCatalogEntry> CatalogEntries;
    TMap<uint32, int32> NumericIndex;
};
