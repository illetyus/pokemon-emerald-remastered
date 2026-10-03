#include "RemasterWorldCatalog.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

uint32 FRemasterWorldCatalog::NumericKey(
    int32 GroupNum,
    int32 MapNum)
{
    return (static_cast<uint32>(GroupNum) << 16u)
        | static_cast<uint32>(MapNum & 0xFFFF);
}

bool FRemasterWorldCatalog::LoadManifest(
    const FString& AbsoluteManifestPath,
    FString& OutError)
{
    FString JsonText;

    CatalogEntries.Reset();
    NumericIndex.Reset();

    if (!FFileHelper::LoadFileToString(
            JsonText,
            *AbsoluteManifestPath))
    {
        OutError = FString::Printf(
            TEXT("Unable to read world manifest: %s"),
            *AbsoluteManifestPath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonText);

    if (!FJsonSerializer::Deserialize(Reader, Root)
        || !Root.IsValid())
    {
        OutError = TEXT("World manifest is not valid JSON.");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Maps = nullptr;
    if (!Root->TryGetArrayField(TEXT("maps"), Maps)
        || Maps == nullptr)
    {
        OutError = TEXT("World manifest is missing maps array.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *Maps)
    {
        const TSharedPtr<FJsonObject> Object = Value->AsObject();
        if (!Object.IsValid())
        {
            OutError = TEXT("World manifest contains a non-object map entry.");
            return false;
        }

        FRemasterWorldCatalogEntry Entry;
        double Number = 0.0;

        Object->TryGetStringField(TEXT("id"), Entry.Id);
        Object->TryGetStringField(TEXT("name"), Entry.Name);
        Object->TryGetStringField(TEXT("file"), Entry.RelativeFile);

        if (Object->TryGetNumberField(TEXT("width"), Number))
            Entry.Width = static_cast<int32>(Number);
        if (Object->TryGetNumberField(TEXT("height"), Number))
            Entry.Height = static_cast<int32>(Number);
        if (Object->TryGetNumberField(TEXT("group_num"), Number))
            Entry.GroupNum = static_cast<int32>(Number);
        if (Object->TryGetNumberField(TEXT("map_num"), Number))
            Entry.MapNum = static_cast<int32>(Number);

        if (Entry.Id.IsEmpty()
            || Entry.RelativeFile.IsEmpty()
            || Entry.GroupNum < 0
            || Entry.MapNum < 0)
        {
            OutError = TEXT("World manifest contains an invalid map entry.");
            CatalogEntries.Reset();
            NumericIndex.Reset();
            return false;
        }

        const uint32 Key = NumericKey(
            Entry.GroupNum,
            Entry.MapNum);

        if (NumericIndex.Contains(Key))
        {
            OutError = FString::Printf(
                TEXT("Duplicate numeric map address %d,%d"),
                Entry.GroupNum,
                Entry.MapNum);
            CatalogEntries.Reset();
            NumericIndex.Reset();
            return false;
        }

        const int32 Index = CatalogEntries.Add(MoveTemp(Entry));
        NumericIndex.Add(Key, Index);
    }

    return true;
}

const FRemasterWorldCatalogEntry*
FRemasterWorldCatalog::Find(
    int32 GroupNum,
    int32 MapNum) const
{
    if (GroupNum < 0 || MapNum < 0)
        return nullptr;

    const int32* Index =
        NumericIndex.Find(NumericKey(GroupNum, MapNum));

    return Index && CatalogEntries.IsValidIndex(*Index)
        ? &CatalogEntries[*Index]
        : nullptr;
}
