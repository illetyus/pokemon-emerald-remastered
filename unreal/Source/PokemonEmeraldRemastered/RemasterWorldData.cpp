#include "RemasterWorldData.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
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

int32 IntField(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Name)
{
    double Value = 0.0;
    Object->TryGetNumberField(Name, Value);
    return static_cast<int32>(Value);
}

bool BoolField(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Name)
{
    bool Value = false;
    Object->TryGetBoolField(Name, Value);
    return Value;
}
}

bool FRemasterWorldData::LoadMapJson(
    const FString& AbsolutePath,
    FRemasterMapIR& OutMap,
    FString& OutError)
{
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
    {
        OutError = FString::Printf(
            TEXT("Unable to read map IR: %s"),
            *AbsolutePath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonText);

    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Map IR is not valid JSON.");
        return false;
    }

    OutMap = FRemasterMapIR{};
    OutMap.SchemaVersion = IntField(Root, TEXT("schema_version"));

    const TSharedPtr<FJsonObject>* MapPtr = nullptr;
    const TSharedPtr<FJsonObject>* LayoutPtr = nullptr;

    if (!Root->TryGetObjectField(TEXT("map"), MapPtr) ||
        !Root->TryGetObjectField(TEXT("layout"), LayoutPtr) ||
        MapPtr == nullptr ||
        LayoutPtr == nullptr)
    {
        OutError = TEXT("Map IR is missing map/layout objects.");
        return false;
    }

    const TSharedPtr<FJsonObject>& Map = *MapPtr;
    const TSharedPtr<FJsonObject>& Layout = *LayoutPtr;

    OutMap.Id = StringField(Map, TEXT("id"));
    OutMap.Name = StringField(Map, TEXT("name"));
    OutMap.LayoutId = StringField(Map, TEXT("layout"));
    OutMap.Music = StringField(Map, TEXT("music"));
    OutMap.RegionMapSection = StringField(Map, TEXT("region_map_section"));
    OutMap.Weather = StringField(Map, TEXT("weather"));
    OutMap.MapType = StringField(Map, TEXT("map_type"));
    OutMap.BattleScene = StringField(Map, TEXT("battle_scene"));

    OutMap.bRequiresFlash = BoolField(Map, TEXT("requires_flash"));
    OutMap.bAllowCycling = BoolField(Map, TEXT("allow_cycling"));
    OutMap.bAllowEscaping = BoolField(Map, TEXT("allow_escaping"));
    OutMap.bAllowRunning = BoolField(Map, TEXT("allow_running"));
    OutMap.bShowMapName = BoolField(Map, TEXT("show_map_name"));

    OutMap.Width = IntField(Layout, TEXT("width"));
    OutMap.Height = IntField(Layout, TEXT("height"));
    OutMap.SourceWordCount = IntField(Layout, TEXT("source_word_count"));
    OutMap.ActiveWordCount = IntField(Layout, TEXT("active_word_count"));
    OutMap.PrimaryTileset = StringField(Layout, TEXT("primary_tileset"));
    OutMap.SecondaryTileset = StringField(Layout, TEXT("secondary_tileset"));

    const TArray<TSharedPtr<FJsonValue>>* RawBlocks = nullptr;
    if (!Layout->TryGetArrayField(TEXT("raw_blocks_u16"), RawBlocks) ||
        RawBlocks == nullptr)
    {
        OutError = TEXT("Map IR is missing raw_blocks_u16.");
        return false;
    }

    OutMap.RawBlocks.Reserve(RawBlocks->Num());
    for (const TSharedPtr<FJsonValue>& Value : *RawBlocks)
    {
        OutMap.RawBlocks.Add(
            static_cast<uint16>(Value->AsNumber()));
    }

    const TArray<TSharedPtr<FJsonValue>>* TrailingWords = nullptr;
    if (!Layout->TryGetArrayField(TEXT("trailing_words_u16"), TrailingWords) ||
        TrailingWords == nullptr)
    {
        OutError = TEXT("Map IR is missing trailing_words_u16.");
        return false;
    }

    OutMap.TrailingWords.Reserve(TrailingWords->Num());
    for (const TSharedPtr<FJsonValue>& Value : *TrailingWords)
    {
        OutMap.TrailingWords.Add(
            static_cast<uint16>(Value->AsNumber()));
    }

    const TArray<TSharedPtr<FJsonValue>>* MetatileIds = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Collision = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Elevation = nullptr;

    if (!Layout->TryGetArrayField(TEXT("metatile_ids_u16"), MetatileIds) ||
        !Layout->TryGetArrayField(TEXT("collision_u8"), Collision) ||
        !Layout->TryGetArrayField(TEXT("elevation_u8"), Elevation) ||
        MetatileIds == nullptr ||
        Collision == nullptr ||
        Elevation == nullptr)
    {
        OutError = TEXT("Map IR is missing decoded block fields.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *MetatileIds)
        OutMap.MetatileIds.Add(static_cast<uint16>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *Collision)
        OutMap.Collision.Add(static_cast<uint8>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *Elevation)
        OutMap.Elevation.Add(static_cast<uint8>(Value->AsNumber()));

    const TArray<TSharedPtr<FJsonValue>>* Connections = nullptr;
    if (Map->TryGetArrayField(TEXT("connections"), Connections) &&
        Connections != nullptr)
    {
        for (const TSharedPtr<FJsonValue>& Value : *Connections)
        {
            const TSharedPtr<FJsonObject> Object = Value->AsObject();
            if (!Object.IsValid())
                continue;

            FRemasterConnectionIR Item;
            Item.Map = StringField(Object, TEXT("map"));
            Item.Direction = StringField(Object, TEXT("direction"));
            Item.Offset = IntField(Object, TEXT("offset"));
            OutMap.Connections.Add(MoveTemp(Item));
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Objects = nullptr;
    if (Map->TryGetArrayField(TEXT("object_events"), Objects) &&
        Objects != nullptr)
    {
        for (const TSharedPtr<FJsonValue>& Value : *Objects)
        {
            const TSharedPtr<FJsonObject> Object = Value->AsObject();
            if (!Object.IsValid())
                continue;

            FRemasterObjectEventIR Item;
            Item.GraphicsId = StringField(Object, TEXT("graphics_id"));
            Item.X = IntField(Object, TEXT("x"));
            Item.Y = IntField(Object, TEXT("y"));
            Item.Elevation = IntField(Object, TEXT("elevation"));
            Item.MovementType = StringField(Object, TEXT("movement_type"));
            Item.TrainerType = StringField(Object, TEXT("trainer_type"));
            Item.Script = StringField(Object, TEXT("script"));
            Item.Flag = StringField(Object, TEXT("flag"));
            OutMap.ObjectEvents.Add(MoveTemp(Item));
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Warps = nullptr;
    if (Map->TryGetArrayField(TEXT("warp_events"), Warps) &&
        Warps != nullptr)
    {
        for (const TSharedPtr<FJsonValue>& Value : *Warps)
        {
            const TSharedPtr<FJsonObject> Object = Value->AsObject();
            if (!Object.IsValid())
                continue;

            FRemasterWarpEventIR Item;
            Item.X = IntField(Object, TEXT("x"));
            Item.Y = IntField(Object, TEXT("y"));
            Item.Elevation = IntField(Object, TEXT("elevation"));
            Item.DestMap = StringField(Object, TEXT("dest_map"));
            Item.DestWarpId = StringField(Object, TEXT("dest_warp_id"));
            OutMap.WarpEvents.Add(MoveTemp(Item));
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Coords = nullptr;
    if (Map->TryGetArrayField(TEXT("coord_events"), Coords) &&
        Coords != nullptr)
    {
        for (const TSharedPtr<FJsonValue>& Value : *Coords)
        {
            const TSharedPtr<FJsonObject> Object = Value->AsObject();
            if (!Object.IsValid())
                continue;

            FRemasterCoordEventIR Item;
            Item.Type = StringField(Object, TEXT("type"));
            Item.X = IntField(Object, TEXT("x"));
            Item.Y = IntField(Object, TEXT("y"));
            Item.Elevation = IntField(Object, TEXT("elevation"));
            Item.Var = StringField(Object, TEXT("var"));
            Item.VarValue = StringField(Object, TEXT("var_value"));
            Item.Script = StringField(Object, TEXT("script"));
            OutMap.CoordEvents.Add(MoveTemp(Item));
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Background = nullptr;
    if (Map->TryGetArrayField(TEXT("bg_events"), Background) &&
        Background != nullptr)
    {
        for (const TSharedPtr<FJsonValue>& Value : *Background)
        {
            const TSharedPtr<FJsonObject> Object = Value->AsObject();
            if (!Object.IsValid())
                continue;

            FRemasterBackgroundEventIR Item;
            Item.Type = StringField(Object, TEXT("type"));
            Item.X = IntField(Object, TEXT("x"));
            Item.Y = IntField(Object, TEXT("y"));
            Item.Elevation = IntField(Object, TEXT("elevation"));
            Item.Facing = StringField(Object, TEXT("player_facing_dir"));
            Item.Script = StringField(Object, TEXT("script"));
            OutMap.BackgroundEvents.Add(MoveTemp(Item));
        }
    }

    if (!OutMap.IsValid())
    {
        OutError = TEXT("Map IR failed schema/integrity validation.");
        return false;
    }

    return true;
}
