#include "RemasterWorldGameplaySubsystem.h"

#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldCatalogSubsystem.h"

extern "C"
{
#include "remaster/emerald_events.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"
#include "remaster/emerald_transition.h"
}

namespace
{
bool FitsInt16(int32 Value)
{
    return Value >= MIN_int16 && Value <= MAX_int16;
}

bool FitsUInt8(int32 Value)
{
    return Value >= 0 && Value <= MAX_uint8;
}

bool FitsUInt16(int32 Value)
{
    return Value >= 0 && Value <= MAX_uint16;
}
}

void URemasterWorldGameplaySubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<URemasterVanillaPlusSaveSubsystem>();
    Collection.InitializeDependency<URemasterWorldCatalogSubsystem>();

    Super::Initialize(Collection);

    LoadCurrentMapFromSave(true);
}

bool URemasterWorldGameplaySubsystem::LoadCurrentMapFromSave(
    bool bResetTemporaryState)
{
    bMapReady = false;
    CurrentMap = FRemasterMapIR{};

    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return false;

    URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GI->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    URemasterWorldCatalogSubsystem* CatalogSubsystem =
        GI->GetSubsystem<URemasterWorldCatalogSubsystem>();

    if (!SaveSubsystem
        || !CatalogSubsystem
        || !SaveSubsystem->HasUsableSave())
    {
        return false;
    }

    FRemasterLegacyOverworldSnapshot Snapshot;
    if (!SaveSubsystem->GetOverworldSnapshot(Snapshot))
        return false;

    if (bResetTemporaryState)
    {
        RemasterEmeraldSave* NativeSave =
            static_cast<RemasterEmeraldSave*>(
                SaveSubsystem->GetMutableNativeSaveHandle());

        if (!NativeSave)
            return false;

        remaster_emerald_clear_temp_field_event_data(NativeSave);
    }

    FString Error;
    FRemasterMapIR Loaded;
    if (!CatalogSubsystem->LoadMap(
            Snapshot.MapGroup,
            Snapshot.MapNum,
            Loaded,
            Error))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Vanilla+ map load failed for %d,%d: %s"),
            Snapshot.MapGroup,
            Snapshot.MapNum,
            *Error);
        return false;
    }

    CurrentMap = MoveTemp(Loaded);
    bMapReady = true;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Vanilla+ gameplay map ready: %s (%d,%d) objects=%d warps=%d coords=%d"),
        *CurrentMap.Id,
        CurrentMap.GroupNum,
        CurrentMap.MapNum,
        CurrentMap.ObjectEvents.Num(),
        CurrentMap.WarpEvents.Num(),
        CurrentMap.CoordEvents.Num());

    return true;
}

bool URemasterWorldGameplaySubsystem::IsObjectVisible(
    int32 LocalId,
    bool& OutVisible) const
{
    OutVisible = false;

    if (!bMapReady || LocalId <= 0 || !GetGameInstance())
        return false;

    const FRemasterObjectEventIR* Event =
        CurrentMap.ObjectEvents.FindByPredicate(
            [LocalId](const FRemasterObjectEventIR& Candidate)
            {
                return Candidate.LocalId == LocalId;
            });

    if (!Event || !FitsUInt16(Event->FlagId))
        return false;

    const URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();

    if (!SaveSubsystem || !SaveSubsystem->HasUsableSave())
        return false;

    const RemasterEmeraldSave* Save =
        static_cast<const RemasterEmeraldSave*>(
            SaveSubsystem->GetNativeSaveHandle());

    if (!Save)
        return false;

    RemasterEmeraldObjectEventDef Native{};
    Native.local_id = static_cast<uint16>(Event->LocalId);
    Native.x = static_cast<int16>(Event->X);
    Native.y = static_cast<int16>(Event->Y);
    Native.elevation = static_cast<uint8>(Event->Elevation);
    Native.flag_id = static_cast<uint16>(Event->FlagId);

    OutVisible =
        remaster_emerald_object_event_visible(Save, &Native) != 0;

    return true;
}

bool URemasterWorldGameplaySubsystem::ResolveWarpAt(
    int32 X,
    int32 Y,
    int32 Elevation,
    FRemasterResolvedWarp& OutWarp) const
{
    OutWarp = FRemasterResolvedWarp{};

    if (!bMapReady
        || !FitsInt16(X)
        || !FitsInt16(Y)
        || !FitsUInt8(Elevation))
    {
        return false;
    }

    TArray<RemasterEmeraldWarpEventDef> NativeEvents;
    TArray<int32> SourceIndices;

    NativeEvents.Reserve(CurrentMap.WarpEvents.Num());
    SourceIndices.Reserve(CurrentMap.WarpEvents.Num());

    for (int32 Index = 0; Index < CurrentMap.WarpEvents.Num(); ++Index)
    {
        const FRemasterWarpEventIR& Event = CurrentMap.WarpEvents[Index];

        if (!FitsInt16(Event.X)
            || !FitsInt16(Event.Y)
            || !FitsUInt8(Event.Elevation)
            || !FitsUInt8(Event.DestWarpIdNum))
        {
            continue;
        }

        if (!Event.bDynamicTarget
            && (!FitsUInt8(Event.DestGroupNum)
                || !FitsUInt8(Event.DestMapNum)))
        {
            continue;
        }

        RemasterEmeraldWarpEventDef Native{};
        Native.x = static_cast<int16>(Event.X);
        Native.y = static_cast<int16>(Event.Y);
        Native.elevation = static_cast<uint8>(Event.Elevation);
        Native.dest_warp_id = static_cast<uint8>(Event.DestWarpIdNum);
        Native.dest_map_group = Event.bDynamicTarget
            ? 0u
            : static_cast<uint8>(Event.DestGroupNum);
        Native.dest_map_num = Event.bDynamicTarget
            ? 0u
            : static_cast<uint8>(Event.DestMapNum);

        NativeEvents.Add(Native);
        SourceIndices.Add(Index);
    }

    if (NativeEvents.IsEmpty())
        return false;

    size_t MatchIndex = 0;
    if (!remaster_emerald_find_warp(
            NativeEvents.GetData(),
            static_cast<size_t>(NativeEvents.Num()),
            static_cast<int16>(X),
            static_cast<int16>(Y),
            static_cast<uint8>(Elevation),
            &MatchIndex))
    {
        return false;
    }

    if (MatchIndex >= static_cast<size_t>(SourceIndices.Num()))
        return false;

    const int32 SourceIndex = SourceIndices[static_cast<int32>(MatchIndex)];
    if (!CurrentMap.WarpEvents.IsValidIndex(SourceIndex))
        return false;

    const FRemasterWarpEventIR& Source = CurrentMap.WarpEvents[SourceIndex];

    OutWarp.SourceEventIndex = SourceIndex;
    OutWarp.DestGroupNum = Source.DestGroupNum;
    OutWarp.DestMapNum = Source.DestMapNum;
    OutWarp.DestWarpId = Source.DestWarpIdNum;
    OutWarp.DestMap = Source.DestMap;
    OutWarp.bDynamicTarget = Source.bDynamicTarget;

    return true;
}

bool URemasterWorldGameplaySubsystem::ResolveCoordEventAt(
    int32 X,
    int32 Y,
    int32 Elevation,
    FRemasterResolvedCoordEvent& OutEvent) const
{
    OutEvent = FRemasterResolvedCoordEvent{};

    if (!bMapReady
        || !FitsInt16(X)
        || !FitsInt16(Y)
        || !FitsUInt8(Elevation)
        || !GetGameInstance())
    {
        return false;
    }

    const URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();

    if (!SaveSubsystem || !SaveSubsystem->HasUsableSave())
        return false;

    const RemasterEmeraldSave* Save =
        static_cast<const RemasterEmeraldSave*>(
            SaveSubsystem->GetNativeSaveHandle());

    if (!Save)
        return false;

    TArray<RemasterEmeraldCoordEventDef> NativeEvents;
    TArray<int32> SourceIndices;

    NativeEvents.Reserve(CurrentMap.CoordEvents.Num());
    SourceIndices.Reserve(CurrentMap.CoordEvents.Num());

    for (int32 Index = 0; Index < CurrentMap.CoordEvents.Num(); ++Index)
    {
        const FRemasterCoordEventIR& Event = CurrentMap.CoordEvents[Index];

        if (!FitsInt16(Event.X)
            || !FitsInt16(Event.Y)
            || !FitsUInt8(Event.Elevation))
        {
            continue;
        }

        RemasterEmeraldCoordEventDef Native{};
        Native.x = static_cast<int16>(Event.X);
        Native.y = static_cast<int16>(Event.Y);
        Native.elevation = static_cast<uint8>(Event.Elevation);

        if (Event.Type.Equals(TEXT("weather"), ESearchCase::IgnoreCase))
        {
            if (!FitsUInt16(Event.WeatherId))
                continue;

            Native.kind = REMASTER_EMERALD_COORD_WEATHER;
            Native.weather = static_cast<uint16>(Event.WeatherId);
        }
        else if (Event.Type.Equals(TEXT("trigger"), ESearchCase::IgnoreCase))
        {
            if (!FitsUInt16(Event.VarId)
                || !FitsUInt16(Event.VarValueNum))
            {
                continue;
            }

            Native.kind = REMASTER_EMERALD_COORD_TRIGGER;
            Native.trigger = static_cast<uint16>(Event.VarId);
            Native.index = static_cast<uint16>(Event.VarValueNum);
        }
        else
        {
            continue;
        }

        NativeEvents.Add(Native);
        SourceIndices.Add(Index);
    }

    if (NativeEvents.IsEmpty())
        return false;

    const RemasterEmeraldCoordMatch Match =
        remaster_emerald_find_coord_event(
            Save,
            NativeEvents.GetData(),
            static_cast<size_t>(NativeEvents.Num()),
            static_cast<int16>(X),
            static_cast<int16>(Y),
            static_cast<uint8>(Elevation));

    if (Match.kind == REMASTER_EMERALD_COORD_MATCH_NONE
        || Match.event_index >= static_cast<size_t>(SourceIndices.Num()))
    {
        return false;
    }

    const int32 SourceIndex =
        SourceIndices[static_cast<int32>(Match.event_index)];

    if (!CurrentMap.CoordEvents.IsValidIndex(SourceIndex))
        return false;

    const FRemasterCoordEventIR& Source =
        CurrentMap.CoordEvents[SourceIndex];

    OutEvent.SourceEventIndex = SourceIndex;
    OutEvent.Script = Source.Script;
    OutEvent.Weather = Source.Weather;
    OutEvent.WeatherId = Source.WeatherId;

    if (Match.kind == REMASTER_EMERALD_COORD_MATCH_WEATHER)
        OutEvent.Kind = ERemasterResolvedCoordKind::Weather;
    else
        OutEvent.Kind = ERemasterResolvedCoordKind::Script;

    return true;
}



bool URemasterWorldGameplaySubsystem::ResolveBackgroundEventAt(
    int32 X,
    int32 Y,
    int32 Elevation,
    int32 FacingDirection,
    FRemasterResolvedBackgroundEvent& OutEvent) const
{
    OutEvent = FRemasterResolvedBackgroundEvent{};

    if (!bMapReady
        || !FitsInt16(X)
        || !FitsInt16(Y)
        || !FitsUInt8(Elevation)
        || FacingDirection < REMASTER_EMERALD_DIR_SOUTH
        || FacingDirection > REMASTER_EMERALD_DIR_EAST
        || !GetGameInstance())
    {
        return false;
    }

    const URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();

    if (!SaveSubsystem || !SaveSubsystem->HasUsableSave())
        return false;

    const RemasterEmeraldSave* Save =
        static_cast<const RemasterEmeraldSave*>(
            SaveSubsystem->GetNativeSaveHandle());

    if (!Save)
        return false;

    TArray<RemasterEmeraldBackgroundEventDef> NativeEvents;
    TArray<int32> SourceIndices;

    NativeEvents.Reserve(CurrentMap.BackgroundEvents.Num());
    SourceIndices.Reserve(CurrentMap.BackgroundEvents.Num());

    for (int32 Index = 0; Index < CurrentMap.BackgroundEvents.Num(); ++Index)
    {
        const FRemasterBackgroundEventIR& Event =
            CurrentMap.BackgroundEvents[Index];

        if (!FitsInt16(Event.X)
            || !FitsInt16(Event.Y)
            || !FitsUInt8(Event.Elevation)
            || !FitsUInt8(Event.KindId))
        {
            continue;
        }

        RemasterEmeraldBackgroundEventDef Native{};
        Native.x = static_cast<int16>(Event.X);
        Native.y = static_cast<int16>(Event.Y);
        Native.elevation = static_cast<uint8>(Event.Elevation);
        Native.kind = static_cast<uint8>(Event.KindId);

        if (Event.KindId == REMASTER_EMERALD_BG_HIDDEN_ITEM)
        {
            if (!FitsUInt16(Event.ItemId)
                || !FitsUInt16(Event.FlagId))
            {
                continue;
            }

            Native.item_id = static_cast<uint16>(Event.ItemId);
            Native.hidden_flag_id = static_cast<uint16>(Event.FlagId);
        }
        else if (Event.KindId == REMASTER_EMERALD_BG_SECRET_BASE)
        {
            if (!FitsUInt16(Event.SecretBaseId))
                continue;

            Native.secret_base_id =
                static_cast<uint16>(Event.SecretBaseId);
        }

        NativeEvents.Add(Native);
        SourceIndices.Add(Index);
    }

    if (NativeEvents.IsEmpty())
        return false;

    const RemasterEmeraldBackgroundMatch Match =
        remaster_emerald_find_background_event(
            Save,
            NativeEvents.GetData(),
            static_cast<size_t>(NativeEvents.Num()),
            static_cast<int16>(X),
            static_cast<int16>(Y),
            static_cast<uint8>(Elevation),
            static_cast<uint8>(FacingDirection));

    if (Match.kind == REMASTER_EMERALD_BG_MATCH_NONE
        || Match.event_index >= static_cast<size_t>(SourceIndices.Num()))
    {
        return false;
    }

    const int32 SourceIndex =
        SourceIndices[static_cast<int32>(Match.event_index)];

    if (!CurrentMap.BackgroundEvents.IsValidIndex(SourceIndex))
        return false;

    const FRemasterBackgroundEventIR& Source =
        CurrentMap.BackgroundEvents[SourceIndex];

    OutEvent.SourceEventIndex = SourceIndex;
    OutEvent.Script = Source.Script;
    OutEvent.Item = Source.Item;
    OutEvent.ItemId = Source.ItemId;
    OutEvent.FlagId = Source.FlagId;
    OutEvent.SecretBaseId = Source.SecretBaseId;

    switch (Match.kind)
    {
    case REMASTER_EMERALD_BG_MATCH_SCRIPT:
        OutEvent.Kind = ERemasterResolvedBackgroundKind::Script;
        break;

    case REMASTER_EMERALD_BG_MATCH_HIDDEN_ITEM:
        OutEvent.Kind = ERemasterResolvedBackgroundKind::HiddenItem;
        break;

    case REMASTER_EMERALD_BG_MATCH_SECRET_BASE:
        OutEvent.Kind = ERemasterResolvedBackgroundKind::SecretBase;
        break;

    default:
        return false;
    }

    return true;
}

bool URemasterWorldGameplaySubsystem::ApplyResolvedWarp(
    const FRemasterResolvedWarp& Warp)
{
    if (!bMapReady || !GetGameInstance())
        return false;

    URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    URemasterWorldCatalogSubsystem* CatalogSubsystem =
        GetGameInstance()->GetSubsystem<URemasterWorldCatalogSubsystem>();

    if (!SaveSubsystem
        || !CatalogSubsystem
        || !SaveSubsystem->HasUsableSave())
    {
        return false;
    }

    RemasterEmeraldSave* Save =
        static_cast<RemasterEmeraldSave*>(
            SaveSubsystem->GetMutableNativeSaveHandle());

    if (!Save)
        return false;

    RemasterEmeraldWarpState Destination{};

    if (Warp.bDynamicTarget)
    {
        if (!remaster_emerald_dynamic_warp_get(
                Save,
                &Destination))
        {
            return false;
        }
    }
    else
    {
        if (Warp.DestGroupNum < MIN_int8
            || Warp.DestGroupNum > MAX_int8
            || Warp.DestMapNum < MIN_int8
            || Warp.DestMapNum > MAX_int8
            || Warp.DestWarpId < MIN_int8
            || Warp.DestWarpId > MAX_int8)
        {
            return false;
        }

        Destination.map_group =
            static_cast<int8>(Warp.DestGroupNum);
        Destination.map_num =
            static_cast<int8>(Warp.DestMapNum);
        Destination.warp_id =
            static_cast<int8>(Warp.DestWarpId);
        Destination.x = -1;
        Destination.y = -1;
    }

    FRemasterMapIR TargetMap;
    FString Error;

    if (!CatalogSubsystem->LoadMap(
            static_cast<int32>(Destination.map_group),
            static_cast<int32>(Destination.map_num),
            TargetMap,
            Error))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Warp target map load failed for %d,%d: %s"),
            static_cast<int32>(Destination.map_group),
            static_cast<int32>(Destination.map_num),
            *Error);
        return false;
    }

    if (TargetMap.LayoutNum <= 0
        || TargetMap.LayoutNum > MAX_uint16
        || !FitsUInt8(TargetMap.WeatherId)
        || !FitsUInt8(TargetMap.MapTypeId)
        || TargetMap.Width <= 0
        || TargetMap.Width > MAX_int16
        || TargetMap.Height <= 0
        || TargetMap.Height > MAX_int16)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Warp target map has invalid layout/dimensions: %s"),
            *TargetMap.Id);
        return false;
    }

    TArray<RemasterEmeraldWarpEventDef> TargetWarps;
    TargetWarps.Reserve(TargetMap.WarpEvents.Num());

    for (const FRemasterWarpEventIR& Event : TargetMap.WarpEvents)
    {
        if (!FitsInt16(Event.X)
            || !FitsInt16(Event.Y)
            || !FitsUInt8(Event.Elevation))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("Warp target map contains invalid warp coordinates: %s"),
                *TargetMap.Id);
            return false;
        }

        RemasterEmeraldWarpEventDef Native{};
        Native.x = static_cast<int16>(Event.X);
        Native.y = static_cast<int16>(Event.Y);
        Native.elevation = static_cast<uint8>(Event.Elevation);
        TargetWarps.Add(Native);
    }

    if (!remaster_emerald_apply_warp(
            Save,
            Destination,
            static_cast<uint16>(TargetMap.LayoutNum),
            static_cast<uint8>(TargetMap.WeatherId),
            static_cast<uint8>(TargetMap.MapTypeId),
            TargetMap.bRequiresFlash ? 1 : 0,
            static_cast<int16>(TargetMap.Width),
            static_cast<int16>(TargetMap.Height),
            TargetWarps.IsEmpty() ? nullptr : TargetWarps.GetData(),
            static_cast<size_t>(TargetWarps.Num())))
    {
        return false;
    }

    CurrentMap = MoveTemp(TargetMap);
    bMapReady = true;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Vanilla+ warp applied: map=%s (%d,%d) warp=%d dynamic=%d"),
        *CurrentMap.Id,
        CurrentMap.GroupNum,
        CurrentMap.MapNum,
        static_cast<int32>(Destination.warp_id),
        Warp.bDynamicTarget ? 1 : 0);

    return true;
}
