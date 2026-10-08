#include "RemasterCharacterCatalogSubsystem.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <cmath>
#include <limits>

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
        && std::isfinite(Value) && std::trunc(Value) == Value
        && Value >= 0 && Value <= 65535
        ? static_cast<int32>(Value)
        : DefaultValue;
}

float FloatFieldDefault(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Name,
    float DefaultValue)
{
    double Value = 0.0;
    return Object->TryGetNumberField(Name, Value)
        && std::isfinite(Value) && std::abs(Value) <= 1.0e6
        ? static_cast<float>(Value)
        : DefaultValue;
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
        if (!Value.IsValid() || !Value->TryGetString(Text))
        {
            OutValues.Reset();
            return false;
        }
        OutValues.Add(MoveTemp(Text));
    }
    return true;
}

bool IsLogicalId(const FString& Value)
{
    if (Value.IsEmpty())
        return false;

    for (int32 Index = 0; Index < Value.Len(); ++Index)
    {
        const TCHAR Character = Value[Index];
        const bool bAllowed =
            (Character >= TEXT('a') && Character <= TEXT('z'))
            || (Character >= TEXT('0') && Character <= TEXT('9'))
            || Character == TEXT('.')
            || Character == TEXT('_')
            || Character == TEXT('-');
        if (!bAllowed || (Index == 0 && !FChar::IsAlnum(Character)))
            return false;
    }
    return true;
}

bool IsSha256(const FString& Value)
{
    if (Value.Len() != 64 || Value != Value.ToLower())
        return false;
    for (const TCHAR Character : Value)
    {
        if (!FChar::IsHexDigit(Character))
            return false;
    }
    return true;
}

bool AreHashesValid(const TArray<FString>& Values)
{
    for (const FString& Value : Values)
    {
        if (!IsSha256(Value))
            return false;
    }
    return true;
}

bool IsAllowedKind(const FString& Kind)
{
    return Kind == TEXT("human")
        || Kind == TEXT("pokemon_overworld")
        || Kind == TEXT("special_object");
}

bool IsEntryValid(const FRemasterCharacterPresentationEntry& Entry)
{
    if (!Entry.GraphicsName.StartsWith(TEXT("OBJ_EVENT_GFX_")))
        return false;
    for (const TCHAR Character : Entry.GraphicsName)
        if (!((Character >= TEXT('A') && Character <= TEXT('Z'))
            || (Character >= TEXT('0') && Character <= TEXT('9'))
            || Character == TEXT('_')))
            return false;
    if (Entry.GraphicsId < 0
        || Entry.GraphicsName.IsEmpty()
        || !IsAllowedKind(Entry.PresentationKind)
        || Entry.Scale <= 0.0f
        || !FMath::IsFinite(Entry.Scale)
        || !FMath::IsFinite(Entry.GroundOffsetCm)
        || !FMath::IsFinite(Entry.YawOffsetDeg))
    {
        return false;
    }

    for (const FString* LogicalId : {
             &Entry.PresentationId,
             &Entry.SourceFamily,
             &Entry.SkeletonFamily,
             &Entry.ModelId,
             &Entry.AnimationSetId,
             &Entry.LodProfile,
             &Entry.FallbackId,
             &Entry.ProvenanceId})
    {
        if (!IsLogicalId(*LogicalId))
            return false;
    }

    for (const FString& MaterialId : Entry.MaterialIds)
    {
        if (!IsLogicalId(MaterialId))
            return false;
    }

    return AreHashesValid(Entry.SourceSha256)
        && AreHashesValid(Entry.NormalizedSha256)
        && (Entry.SourceFamily == TEXT("project_placeholder")
            || (!Entry.SourceSha256.IsEmpty() && Entry.ProvenanceId != TEXT("project.placeholder.r6")))
        && (Entry.NormalizedSha256.IsEmpty() || !Entry.SourceSha256.IsEmpty());
}
}

void URemasterCharacterCatalogSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!ReloadCatalog())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("R6 character presentation catalog is unavailable: %s"),
            *GetManifestPath());
    }
}

FString URemasterCharacterCatalogSubsystem::GetManifestPath() const
{
    return FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("Generated"),
        TEXT("Characters"),
        TEXT("manifest.json"));
}

bool URemasterCharacterCatalogSubsystem::ReloadCatalog()
{
    ++CatalogRevision;
    bCatalogReady = false;
    ContentSha256.Reset();
    EntriesById.Reset();
    GraphicsNameToId.Reset();

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *GetManifestPath()))
        return false;

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        return false;

    if (IntFieldDefault(Root, TEXT("schema_version"), -1) != 1)
        return false;

    const int32 ExpectedCount =
        IntFieldDefault(Root, TEXT("identity_count"), -1);
    const int32 ExpectedMin =
        IntFieldDefault(Root, TEXT("min_graphics_id"), -1);
    const int32 ExpectedMax =
        IntFieldDefault(Root, TEXT("max_graphics_id"), -1);
    ContentSha256 = StringField(Root, TEXT("content_sha256"));

    if (ExpectedCount <= 0
        || ExpectedMin < 0
        || ExpectedMax < ExpectedMin
        || ExpectedCount != ExpectedMax - ExpectedMin + 1
        || !IsSha256(ContentSha256))
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Root->TryGetArrayField(TEXT("entries"), Values)
        || Values == nullptr
        || Values->Num() != ExpectedCount)
    {
        return false;
    }

    int32 LastGraphicsId = ExpectedMin - 1;
    TSet<FString> PresentationIds;

    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        const TSharedPtr<FJsonObject> Object =
            Value.IsValid() ? Value->AsObject() : nullptr;
        if (!Object.IsValid())
            return false;

        if (IntFieldDefault(Object, TEXT("schema_version"), -1) != 1)
            return false;

        FRemasterCharacterPresentationEntry Entry;
        Entry.GraphicsId =
            IntFieldDefault(Object, TEXT("graphics_id"), -1);
        Entry.GraphicsName =
            StringField(Object, TEXT("graphics_name"));
        Entry.PresentationId =
            StringField(Object, TEXT("presentation_id"));
        Entry.PresentationKind =
            StringField(Object, TEXT("presentation_kind"));
        Entry.SourceFamily =
            StringField(Object, TEXT("source_family"));
        Entry.SkeletonFamily =
            StringField(Object, TEXT("skeleton_family"));
        Entry.ModelId =
            StringField(Object, TEXT("model_id"));
        Entry.AnimationSetId =
            StringField(Object, TEXT("animation_set_id"));
        Entry.Scale =
            FloatFieldDefault(Object, TEXT("scale"), -1.0f);
        Entry.GroundOffsetCm =
            FloatFieldDefault(Object, TEXT("ground_offset_cm"), std::numeric_limits<float>::quiet_NaN());
        Entry.YawOffsetDeg =
            FloatFieldDefault(Object, TEXT("yaw_offset_deg"), std::numeric_limits<float>::quiet_NaN());
        Entry.LodProfile =
            StringField(Object, TEXT("lod_profile"));
        Entry.FallbackId =
            StringField(Object, TEXT("fallback_id"));
        Entry.ProvenanceId =
            StringField(Object, TEXT("provenance_id"));

        if (!ParseStringArray(Object, TEXT("material_ids"), Entry.MaterialIds)
            || !ParseStringArray(
                Object,
                TEXT("source_sha256"),
                Entry.SourceSha256)
            || !ParseStringArray(
                Object,
                TEXT("normalized_sha256"),
                Entry.NormalizedSha256)
            || !IsEntryValid(Entry))
        {
            return false;
        }

        if (Entry.GraphicsId != LastGraphicsId + 1
            || EntriesById.Contains(Entry.GraphicsId)
            || GraphicsNameToId.Contains(Entry.GraphicsName)
            || PresentationIds.Contains(Entry.PresentationId))
        {
            return false;
        }

        LastGraphicsId = Entry.GraphicsId;
        PresentationIds.Add(Entry.PresentationId);
        GraphicsNameToId.Add(Entry.GraphicsName, Entry.GraphicsId);
        EntriesById.Add(Entry.GraphicsId, MoveTemp(Entry));
    }

    if (EntriesById.Num() != ExpectedCount
        || ExpectedMin != 0
        || LastGraphicsId != ExpectedMax)
    {
        EntriesById.Reset();
        GraphicsNameToId.Reset();
        return false;
    }

    bCatalogReady = true;
    return true;
}

bool URemasterCharacterCatalogSubsystem::ResolveGraphicsId(
    int32 GraphicsId,
    FRemasterCharacterPresentationEntry& OutEntry) const
{
    OutEntry = FRemasterCharacterPresentationEntry{};
    if (!bCatalogReady)
        return false;

    const FRemasterCharacterPresentationEntry* Entry =
        EntriesById.Find(GraphicsId);
    if (!Entry)
        return false;

    OutEntry = *Entry;
    return true;
}

bool URemasterCharacterCatalogSubsystem::ResolveGraphicsName(
    const FString& GraphicsName,
    FRemasterCharacterPresentationEntry& OutEntry) const
{
    OutEntry = FRemasterCharacterPresentationEntry{};
    if (!bCatalogReady)
        return false;

    const int32* GraphicsId = GraphicsNameToId.Find(GraphicsName);
    return GraphicsId != nullptr
        && ResolveGraphicsId(*GraphicsId, OutEntry);
}
