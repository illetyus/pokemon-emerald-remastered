#pragma once

#include "CoreMinimal.h"

struct FRemasterConnectionIR
{
    FString Map;
    FString Direction;
    int32 Offset = 0;
};

struct FRemasterObjectEventIR
{
    FString GraphicsId;
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString MovementType;
    FString TrainerType;
    FString Script;
    FString Flag;
};

struct FRemasterWarpEventIR
{
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString DestMap;
    FString DestWarpId;
};

struct FRemasterCoordEventIR
{
    FString Type;
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString Var;
    FString VarValue;
    FString Script;
};

struct FRemasterBackgroundEventIR
{
    FString Type;
    int32 X = 0;
    int32 Y = 0;
    int32 Elevation = 0;
    FString Facing;
    FString Script;
};

struct FRemasterMapIR
{
    int32 SchemaVersion = 0;

    FString Id;
    FString Name;
    FString LayoutId;
    FString Music;
    FString RegionMapSection;
    FString Weather;
    FString MapType;
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
