#pragma once

#include "CoreMinimal.h"

struct FRemasterConnectionIR
{
    FString Map;
    FString Direction;
    int32 Offset = 0;
    int32 DestGroupNum = -1;
    int32 DestMapNum = -1;
};

struct FRemasterObjectEventIR
{
    int32 LocalId = 0;
    FString GraphicsId;
    int32 GraphicsIdNum = -1;
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString MovementType;
    int32 MovementTypeNum = -1;
    int32 MovementRangeX = 0;
    int32 MovementRangeY = 0;
    FString TrainerType;
    int32 TrainerTypeNum = -1;
    FString TrainerSightOrBerryTreeId;
    int32 TrainerSightOrBerryTreeIdNum = -1;
    FString Script;
    FString Flag;
    int32 FlagId = 0;
};

struct FRemasterWarpEventIR
{
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString DestMap;
    FString DestWarpId;
    int32 DestWarpIdNum = -1;
    int32 DestGroupNum = -1;
    int32 DestMapNum = -1;
    bool bDynamicTarget = false;
};

struct FRemasterCoordEventIR
{
    FString Type;
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString Var;
    int32 VarId = -1;
    FString VarValue;
    int32 VarValueNum = -1;
    FString Weather;
    int32 WeatherId = -1;
    FString Script;
};

struct FRemasterBackgroundEventIR
{
    FString Type;
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString Facing;
    int32 FacingId = -1;
    int32 KindId = -1;
    FString Script;
    FString Item;
    int32 ItemId = -1;
    FString Flag;
    int32 FlagId = -1;
    int32 SecretBaseId = -1;
};

struct FRemasterMapIR
{
    int32 SchemaVersion = 0;

    FString Id;
    FString Name;
    FString GroupName;
    int32 GroupNum = -1;
    int32 MapNum = -1;
    FString LayoutId;
    int32 LayoutNum = -1;
    FString Music;
    FString RegionMapSection;
    FString Weather;
    int32 WeatherId = -1;
    FString MapType;
    int32 MapTypeId = -1;
    FString BattleScene;

    bool bRequiresFlash = false;
    bool bAllowCycling = false;
    bool bAllowEscaping = false;
    bool bAllowRunning = false;
    bool bShowMapName = false;

    int32 Width = 0;
    int32 Height = 0;
    FString PrimaryTileset;
    FString SecondaryTileset;
    int32 SourceWordCount = 0;
    int32 ActiveWordCount = 0;
    int32 BorderSourceWordCount = 0;
    TArray<uint16> BorderActiveWords;
    TArray<uint16> BorderTrailingWords;
    TArray<uint16> RawBlocks;
    TArray<uint16> TrailingWords;
    TArray<uint16> MetatileIds;
    TArray<uint8> Collision;
    TArray<uint8> Elevation;

    TArray<uint16> PrimaryMetatileAttributes;
    TArray<uint16> SecondaryMetatileAttributes;
    TArray<uint8> PrimaryMetatileBehavior;
    TArray<uint8> SecondaryMetatileBehavior;
    TArray<uint8> PrimaryMetatileLayer;
    TArray<uint8> SecondaryMetatileLayer;

    TArray<FRemasterConnectionIR> Connections;
    TArray<FRemasterObjectEventIR> ObjectEvents;
    TArray<FRemasterWarpEventIR> WarpEvents;
    TArray<FRemasterCoordEventIR> CoordEvents;
    TArray<FRemasterBackgroundEventIR> BackgroundEvents;

    bool IsValid() const
    {
        return SchemaVersion == 1
            && Width > 0
            && Height > 0
            && ActiveWordCount == Width * Height
            && RawBlocks.Num() == ActiveWordCount
            && SourceWordCount == RawBlocks.Num() + TrailingWords.Num()
            && BorderActiveWords.Num() == 4
            && BorderSourceWordCount
                == BorderActiveWords.Num() + BorderTrailingWords.Num()
            && MetatileIds.Num() == RawBlocks.Num()
            && Collision.Num() == RawBlocks.Num()
            && Elevation.Num() == RawBlocks.Num()
            && !PrimaryMetatileAttributes.IsEmpty()
            && !SecondaryMetatileAttributes.IsEmpty()
            && PrimaryMetatileBehavior.Num()
                == PrimaryMetatileAttributes.Num()
            && SecondaryMetatileBehavior.Num()
                == SecondaryMetatileAttributes.Num()
            && PrimaryMetatileLayer.Num()
                == PrimaryMetatileAttributes.Num()
            && SecondaryMetatileLayer.Num()
                == SecondaryMetatileAttributes.Num()
            && GroupNum >= 0
            && MapNum >= 0
            && LayoutNum > 0
            && WeatherId >= 0
            && MapTypeId >= 0
            && !Id.IsEmpty();
    }
};

class FRemasterWorldData
{
public:
    static bool LoadMapJson(
        const FString& AbsolutePath,
        FRemasterMapIR& OutMap,
        FString& OutError);
};
