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
    OutMap.GroupName = StringField(Map, TEXT("group_name"));
    OutMap.GroupNum = IntField(Map, TEXT("group_num"));
    OutMap.MapNum = IntField(Map, TEXT("map_num"));
    OutMap.LayoutId = StringField(Map, TEXT("layout"));
    OutMap.LayoutNum = IntFieldDefault(Map, TEXT("layout_num"), -1);
    OutMap.Music = StringField(Map, TEXT("music"));
    OutMap.RegionMapSection = StringField(Map, TEXT("region_map_section"));
    OutMap.Weather = StringField(Map, TEXT("weather"));
    OutMap.WeatherId = IntFieldDefault(Map, TEXT("weather_id"), -1);
    OutMap.MapType = StringField(Map, TEXT("map_type"));
    OutMap.MapTypeId = IntFieldDefault(Map, TEXT("map_type_id"), -1);
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
    OutMap.BorderSourceWordCount =
        IntField(Layout, TEXT("border_source_word_count"));
    OutMap.PrimaryTileset = StringField(Layout, TEXT("primary_tileset"));
    OutMap.SecondaryTileset = StringField(Layout, TEXT("secondary_tileset"));

    const TArray<TSharedPtr<FJsonValue>>* BorderActive = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* BorderTrailing = nullptr;

    if (!Layout->TryGetArrayField(
            TEXT("border_active_words_u16"),
            BorderActive)
        || !Layout->TryGetArrayField(
            TEXT("border_trailing_words_u16"),
            BorderTrailing)
        || BorderActive == nullptr
        || BorderTrailing == nullptr)
    {
        OutError = TEXT("Map IR is missing Emerald border data.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *BorderActive)
        OutMap.BorderActiveWords.Add(
            static_cast<uint16>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *BorderTrailing)
        OutMap.BorderTrailingWords.Add(
            static_cast<uint16>(Value->AsNumber()));

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

    const TArray<TSharedPtr<FJsonValue>>* PrimaryAttributes = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* SecondaryAttributes = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* PrimaryBehavior = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* SecondaryBehavior = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* PrimaryLayer = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* SecondaryLayer = nullptr;

    if (!Layout->TryGetArrayField(
            TEXT("primary_metatile_attributes_u16"),
            PrimaryAttributes)
        || !Layout->TryGetArrayField(
            TEXT("secondary_metatile_attributes_u16"),
            SecondaryAttributes)
        || !Layout->TryGetArrayField(
            TEXT("primary_metatile_behavior_u8"),
            PrimaryBehavior)
        || !Layout->TryGetArrayField(
            TEXT("secondary_metatile_behavior_u8"),
            SecondaryBehavior)
        || !Layout->TryGetArrayField(
            TEXT("primary_metatile_layer_u8"),
            PrimaryLayer)
        || !Layout->TryGetArrayField(
            TEXT("secondary_metatile_layer_u8"),
            SecondaryLayer)
        || PrimaryAttributes == nullptr
        || SecondaryAttributes == nullptr
        || PrimaryBehavior == nullptr
        || SecondaryBehavior == nullptr
        || PrimaryLayer == nullptr
        || SecondaryLayer == nullptr)
    {
        OutError = TEXT("Map IR is missing metatile attribute tables.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *PrimaryAttributes)
        OutMap.PrimaryMetatileAttributes.Add(
            static_cast<uint16>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *SecondaryAttributes)
        OutMap.SecondaryMetatileAttributes.Add(
            static_cast<uint16>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *PrimaryBehavior)
        OutMap.PrimaryMetatileBehavior.Add(
            static_cast<uint8>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *SecondaryBehavior)
        OutMap.SecondaryMetatileBehavior.Add(
            static_cast<uint8>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *PrimaryLayer)
        OutMap.PrimaryMetatileLayer.Add(
            static_cast<uint8>(Value->AsNumber()));

    for (const TSharedPtr<FJsonValue>& Value : *SecondaryLayer)
        OutMap.SecondaryMetatileLayer.Add(
            static_cast<uint8>(Value->AsNumber()));

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
            Item.DestGroupNum =
                IntFieldDefault(Object, TEXT("dest_group_num"), -1);
            Item.DestMapNum =
                IntFieldDefault(Object, TEXT("dest_map_num"), -1);
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
            Item.LocalId = IntField(Object, TEXT("local_id"));
            Item.GraphicsId = StringField(Object, TEXT("graphics_id"));
            Item.GraphicsIdNum = IntFieldDefault(Object, TEXT("graphics_id_u16"), -1);
            Item.X = IntField(Object, TEXT("x"));
            Item.Y = IntField(Object, TEXT("y"));
            Item.Elevation = IntField(Object, TEXT("elevation"));
            Item.MovementType = StringField(Object, TEXT("movement_type"));
            Item.MovementTypeNum =
                IntFieldDefault(Object, TEXT("movement_type_u8"), -1);
            Item.MovementRangeX =
                IntFieldDefault(Object, TEXT("movement_range_x"), 0);
            Item.MovementRangeY =
                IntFieldDefault(Object, TEXT("movement_range_y"), 0);
            Item.TrainerType = StringField(Object, TEXT("trainer_type"));
            Item.TrainerTypeNum =
                IntFieldDefault(Object, TEXT("trainer_type_u16"), -1);
            Item.TrainerSightOrBerryTreeId =
                StringField(Object, TEXT("trainer_sight_or_berry_tree_id"));
            Item.TrainerSightOrBerryTreeIdNum = IntFieldDefault(
                Object,
                TEXT("trainer_sight_or_berry_tree_id_u16"),
                -1);
            Item.Script = StringField(Object, TEXT("script"));
            Item.Flag = StringField(Object, TEXT("flag"));
            Item.FlagId = IntFieldDefault(Object, TEXT("flag_id"), -1);
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
            Item.DestWarpIdNum =
                IntFieldDefault(Object, TEXT("dest_warp_id_u16"), -1);
            Item.DestGroupNum =
                IntFieldDefault(Object, TEXT("dest_group_num"), -1);
            Item.DestMapNum =
                IntFieldDefault(Object, TEXT("dest_map_num"), -1);
            Item.bDynamicTarget =
                BoolField(Object, TEXT("dynamic_target"));
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
            Item.VarId = IntFieldDefault(Object, TEXT("var_id"), -1);
            Item.VarValue = StringField(Object, TEXT("var_value"));
            Item.VarValueNum =
                IntFieldDefault(Object, TEXT("var_value_u16"), -1);
            Item.Weather = StringField(Object, TEXT("weather"));
            Item.WeatherId =
                IntFieldDefault(Object, TEXT("weather_id"), -1);
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
            Item.FacingId = IntFieldDefault(
                Object,
                TEXT("player_facing_dir_id"),
                -1);
            Item.KindId = IntFieldDefault(
                Object,
                TEXT("kind_id"),
                -1);
            Item.Script = StringField(Object, TEXT("script"));
            Item.Item = StringField(Object, TEXT("item"));
            Item.ItemId = IntFieldDefault(
                Object,
                TEXT("item_id"),
                -1);
            Item.Flag = StringField(Object, TEXT("flag"));
            Item.FlagId = IntFieldDefault(
                Object,
                TEXT("flag_id"),
                -1);
            Item.SecretBaseId = IntFieldDefault(
                Object,
                TEXT("secret_base_id_u16"),
                -1);
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
