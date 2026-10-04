#include "RemasterWorldGameplaySubsystem.h"

#include "HAL/UnrealMemory.h"
#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldCatalogSubsystem.h"

extern "C"
{
#include "remaster/emerald_events.h"
#include "remaster/emerald_object_state.h"
#include "remaster/emerald_overworld.h"
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

int32 ConnectionDirectionFromString(const FString& Value)
{
    if (Value.Equals(TEXT("down"), ESearchCase::IgnoreCase))
        return REMASTER_EMERALD_DIR_SOUTH;
    if (Value.Equals(TEXT("up"), ESearchCase::IgnoreCase))
        return REMASTER_EMERALD_DIR_NORTH;
    if (Value.Equals(TEXT("left"), ESearchCase::IgnoreCase))
        return REMASTER_EMERALD_DIR_WEST;
    if (Value.Equals(TEXT("right"), ESearchCase::IgnoreCase))
        return REMASTER_EMERALD_DIR_EAST;
    return REMASTER_EMERALD_DIR_NONE;
}

bool BuildRuntimeObjectEvents(
    const FRemasterMapIR& Map,
    TArray<RemasterEmeraldObjectEventDef>& OutEvents)
{
    if (Map.ObjectEvents.Num() > REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT)
        return false;

    OutEvents.Reset();
    OutEvents.Reserve(Map.ObjectEvents.Num());

    for (const FRemasterObjectEventIR& Source : Map.ObjectEvents)
    {
        if (Source.LocalId <= 0
            || Source.LocalId > MAX_uint16
            || !FitsInt16(Source.X)
            || !FitsInt16(Source.Y)
            || !FitsUInt8(Source.Elevation)
            || !FitsUInt16(Source.FlagId))
        {
            return false;
        }

        RemasterEmeraldObjectEventDef Native{};
        Native.local_id = static_cast<uint16>(Source.LocalId);
        Native.x = static_cast<int16>(Source.X);
        Native.y = static_cast<int16>(Source.Y);
        Native.elevation = static_cast<uint8>(Source.Elevation);
        Native.flag_id = static_cast<uint16>(Source.FlagId);
        Native.script_id = nullptr;
        OutEvents.Add(Native);
    }

    return true;
}

bool PopulatePlayerStepSnapshot(
    const RemasterEmeraldSave* Save,
    const FRemasterMapIR& Map,
    FRemasterPlayerStepResult& OutResult)
{
    if (!Save)
        return false;

    RemasterEmeraldOverworldState State{};
    if (!remaster_emerald_overworld_get(Save, &State))
        return false;

    OutResult.PlayerX = State.player_x;
    OutResult.PlayerY = State.player_y;
    OutResult.MapId = Map.Id;
    OutResult.MapGroup = State.map_group;
    OutResult.MapNum = State.map_num;

    if (State.player_x >= 0
        && State.player_y >= 0
        && State.player_x < Map.Width
        && State.player_y < Map.Height
        && Map.RawBlocks.Num() == Map.Width * Map.Height
        && !Map.PrimaryMetatileAttributes.IsEmpty()
        && !Map.SecondaryMetatileAttributes.IsEmpty())
    {
        RemasterEmeraldMapView View{};
        View.width = Map.Width;
        View.height = Map.Height;
        View.blocks = Map.RawBlocks.GetData();
        View.block_count = static_cast<size_t>(Map.RawBlocks.Num());
        View.border = Map.BorderActiveWords.GetData();
        View.border_count = static_cast<size_t>(Map.BorderActiveWords.Num());
        View.primary_attributes = Map.PrimaryMetatileAttributes.GetData();
        View.primary_attribute_count =
            static_cast<size_t>(Map.PrimaryMetatileAttributes.Num());
        View.secondary_attributes = Map.SecondaryMetatileAttributes.GetData();
        View.secondary_attribute_count =
            static_cast<size_t>(Map.SecondaryMetatileAttributes.Num());

        OutResult.Elevation = remaster_emerald_map_elevation_at(
            &View,
            State.player_x,
            State.player_y);
    }
    else
    {
        OutResult.Elevation = 0;
    }

    return true;
}
}

void URemasterWorldGameplaySubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<URemasterVanillaPlusSaveSubsystem>();
    Collection.InitializeDependency<URemasterWorldCatalogSubsystem>();

    Super::Initialize(Collection);

    NativeObjectRuntime = FMemory::Malloc(
        sizeof(RemasterEmeraldObjectRuntime),
        alignof(RemasterEmeraldObjectRuntime));

    if (NativeObjectRuntime)
    {
        remaster_emerald_object_runtime_reset(
            static_cast<RemasterEmeraldObjectRuntime*>(
                NativeObjectRuntime));
    }

    LoadCurrentMapFromSave(false);
}

void URemasterWorldGameplaySubsystem::Deinitialize()
{
    if (NativeObjectRuntime)
    {
        FMemory::Free(NativeObjectRuntime);
        NativeObjectRuntime = nullptr;
    }

    CurrentMap = FRemasterMapIR{};
    bMapReady = false;

    Super::Deinitialize();
}


bool URemasterWorldGameplaySubsystem::ApplySavedObjectTemplateOverrides()
{
    if (!GetGameInstance())
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

    for (FRemasterObjectEventIR& Event : CurrentMap.ObjectEvents)
    {
        if (Event.LocalId <= 0 || Event.LocalId > MAX_uint8)
            continue;

        size_t TemplateIndex = 0;
        if (!remaster_emerald_object_template_find_local_id(
                Save,
                static_cast<uint8>(Event.LocalId),
                &TemplateIndex))
        {
            continue;
        }

        RemasterEmeraldObjectTemplate Saved{};
        if (!remaster_emerald_object_template_get(
                Save,
                TemplateIndex,
                &Saved))
        {
            continue;
        }

        /*
         * Script addresses in SaveBlock1 are legacy GBA ROM pointers.
         * Never consume them in Unreal; script identity comes from generated IR.
         */
        Event.GraphicsIdNum = Saved.graphics_id;
        Event.X = Saved.x;
        Event.Y = Saved.y;
        Event.Elevation = Saved.elevation;
        Event.MovementTypeNum = Saved.movement_type;
        Event.MovementRangeX = Saved.movement_range_x;
        Event.MovementRangeY = Saved.movement_range_y;
        Event.TrainerTypeNum = Saved.trainer_type;
        Event.TrainerSightOrBerryTreeIdNum =
            Saved.trainer_range_or_berry_tree_id;
        Event.FlagId = Saved.flag_id;
    }

    return true;
}

bool URemasterWorldGameplaySubsystem::RefreshSavedObjectTemplateCache(
    const FRemasterMapIR& Map)
{
    if (!GetGameInstance()
        || Map.ObjectEvents.Num() > REMASTER_EMERALD_OBJECT_TEMPLATE_COUNT)
    {
        return false;
    }

    URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();

    if (!SaveSubsystem || !SaveSubsystem->HasUsableSave())
        return false;

    RemasterEmeraldSave* Save =
        static_cast<RemasterEmeraldSave*>(
            SaveSubsystem->GetMutableNativeSaveHandle());

    if (!Save)
        return false;

    TArray<RemasterEmeraldObjectTemplate> Templates;
    Templates.Reserve(Map.ObjectEvents.Num());

    for (const FRemasterObjectEventIR& Event : Map.ObjectEvents)
    {
        if (Event.LocalId <= 0
            || Event.LocalId > MAX_uint8
            || !FitsUInt16(Event.GraphicsIdNum)
            || !FitsInt16(Event.X)
            || !FitsInt16(Event.Y)
            || !FitsUInt8(Event.Elevation)
            || !FitsUInt8(Event.MovementTypeNum)
            || Event.MovementRangeX < 0
            || Event.MovementRangeX > 0x0F
            || Event.MovementRangeY < 0
            || Event.MovementRangeY > 0x0F
            || !FitsUInt16(Event.TrainerTypeNum)
            || !FitsUInt16(Event.TrainerSightOrBerryTreeIdNum)
            || !FitsUInt16(Event.FlagId))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("Object template has unresolved numeric gameplay data: map=%s localId=%d"),
                *Map.Id,
                Event.LocalId);
            return false;
        }

        RemasterEmeraldObjectTemplate Native{};
        Native.local_id = static_cast<uint8>(Event.LocalId);
        Native.kind = 0;
        Native.graphics_id = static_cast<uint16>(Event.GraphicsIdNum);
        Native.x = static_cast<int16>(Event.X);
        Native.y = static_cast<int16>(Event.Y);
        Native.elevation = static_cast<uint8>(Event.Elevation);
        Native.movement_type = static_cast<uint8>(Event.MovementTypeNum);
        Native.movement_range_x =
            static_cast<uint8>(Event.MovementRangeX);
        Native.movement_range_y =
            static_cast<uint8>(Event.MovementRangeY);
        Native.trainer_type =
            static_cast<uint16>(Event.TrainerTypeNum);
        Native.trainer_range_or_berry_tree_id =
            static_cast<uint16>(Event.TrainerSightOrBerryTreeIdNum);
        Native.legacy_script_address = 0u;
        Native.flag_id = static_cast<uint16>(Event.FlagId);

        Templates.Add(Native);
    }

    return remaster_emerald_object_templates_replace(
        Save,
        Templates.IsEmpty() ? nullptr : Templates.GetData(),
        static_cast<size_t>(Templates.Num())) != 0;
}

bool URemasterWorldGameplaySubsystem::RebuildRuntimeObjectState()
{
    if (!NativeObjectRuntime || !GetGameInstance())
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

    RemasterEmeraldOverworldState State{};
    if (!remaster_emerald_overworld_get(Save, &State))
        return false;

    TArray<RemasterEmeraldObjectEventDef> Events;
    if (!BuildRuntimeObjectEvents(CurrentMap, Events))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Runtime object templates are invalid: %s"),
            *CurrentMap.Id);
        return false;
    }

    return remaster_emerald_object_runtime_load(
        static_cast<RemasterEmeraldObjectRuntime*>(
            NativeObjectRuntime),
        Save,
        Events.IsEmpty() ? nullptr : Events.GetData(),
        static_cast<size_t>(Events.Num()),
        State.player_x,
        State.player_y) != 0;
}

bool URemasterWorldGameplaySubsystem::SyncRuntimeObjectView()
{
    if (!NativeObjectRuntime || !GetGameInstance())
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

    RemasterEmeraldOverworldState State{};
    if (!remaster_emerald_overworld_get(Save, &State))
        return false;

    TArray<RemasterEmeraldObjectEventDef> Events;
    if (!BuildRuntimeObjectEvents(CurrentMap, Events))
        return false;

    return remaster_emerald_object_runtime_sync_view(
        static_cast<RemasterEmeraldObjectRuntime*>(
            NativeObjectRuntime),
        Save,
        Events.IsEmpty() ? nullptr : Events.GetData(),
        static_cast<size_t>(Events.Num()),
        State.player_x,
        State.player_y) != 0;
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

    if (!ApplySavedObjectTemplateOverrides())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Failed to apply saved object template overrides for %s"),
            *CurrentMap.Id);
    }

    if (!RebuildRuntimeObjectState())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Failed to build runtime object state for %s"),
            *CurrentMap.Id);
        CurrentMap = FRemasterMapIR{};
        return false;
    }

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


bool URemasterWorldGameplaySubsystem::SetRuntimeObjectActive(
    int32 LocalId,
    bool bActive)
{
    if (!NativeObjectRuntime
        || LocalId <= 0
        || LocalId > MAX_uint16)
    {
        return false;
    }

    return remaster_emerald_object_runtime_set_active(
        static_cast<RemasterEmeraldObjectRuntime*>(
            NativeObjectRuntime),
        static_cast<uint16>(LocalId),
        bActive ? 1 : 0) != 0;
}

bool URemasterWorldGameplaySubsystem::SetRuntimeObjectPosition(
    int32 LocalId,
    int32 X,
    int32 Y,
    int32 Elevation)
{
    if (!NativeObjectRuntime
        || LocalId <= 0
        || LocalId > MAX_uint16
        || !FitsUInt8(Elevation))
    {
        return false;
    }

    return remaster_emerald_object_runtime_set_position(
        static_cast<RemasterEmeraldObjectRuntime*>(
            NativeObjectRuntime),
        static_cast<uint16>(LocalId),
        X,
        Y,
        static_cast<uint8>(Elevation)) != 0;
}

bool URemasterWorldGameplaySubsystem::SetRuntimeObjectPlayerCollisionExempt(
    int32 LocalId,
    bool bExempt)
{
    if (!NativeObjectRuntime
        || LocalId <= 0
        || LocalId > MAX_uint16)
    {
        return false;
    }

    return remaster_emerald_object_runtime_set_player_collision_exempt(
        static_cast<RemasterEmeraldObjectRuntime*>(
            NativeObjectRuntime),
        static_cast<uint16>(LocalId),
        bExempt ? 1 : 0) != 0;
}


bool URemasterWorldGameplaySubsystem::ProcessCurrentStepEvents(
    int32 CoordStartIndex,
    FRemasterPlayerStepResult& OutResult)
{
    if (!bMapReady
        || CoordStartIndex < 0
        || !CurrentMap.IsValid()
        || !GetGameInstance())
    {
        return false;
    }

    URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();

    if (!SaveSubsystem || !SaveSubsystem->HasUsableSave())
        return false;

    RemasterEmeraldSave* Save =
        static_cast<RemasterEmeraldSave*>(
            SaveSubsystem->GetMutableNativeSaveHandle());

    if (!Save)
        return false;

    RemasterEmeraldOverworldState State{};
    if (!remaster_emerald_overworld_get(Save, &State))
        return false;

    if (State.player_x < 0
        || State.player_y < 0
        || State.player_x >= CurrentMap.Width
        || State.player_y >= CurrentMap.Height)
    {
        return false;
    }

    RemasterEmeraldMapView MapView{};
    MapView.width = CurrentMap.Width;
    MapView.height = CurrentMap.Height;
    MapView.blocks = CurrentMap.RawBlocks.GetData();
    MapView.block_count =
        static_cast<size_t>(CurrentMap.RawBlocks.Num());
    MapView.border = CurrentMap.BorderActiveWords.GetData();
    MapView.border_count =
        static_cast<size_t>(CurrentMap.BorderActiveWords.Num());
    MapView.primary_attributes =
        CurrentMap.PrimaryMetatileAttributes.GetData();
    MapView.primary_attribute_count =
        static_cast<size_t>(CurrentMap.PrimaryMetatileAttributes.Num());
    MapView.secondary_attributes =
        CurrentMap.SecondaryMetatileAttributes.GetData();
    MapView.secondary_attribute_count =
        static_cast<size_t>(CurrentMap.SecondaryMetatileAttributes.Num());

    TArray<RemasterEmeraldCoordEventDef> NativeCoords;
    NativeCoords.Reserve(CurrentMap.CoordEvents.Num());

    for (const FRemasterCoordEventIR& Event : CurrentMap.CoordEvents)
    {
        if (!FitsInt16(Event.X)
            || !FitsInt16(Event.Y)
            || !FitsUInt8(Event.Elevation))
        {
            return false;
        }

        RemasterEmeraldCoordEventDef Native{};
        Native.x = static_cast<int16>(Event.X);
        Native.y = static_cast<int16>(Event.Y);
        Native.elevation = static_cast<uint8>(Event.Elevation);

        if (Event.Type.Equals(TEXT("weather"), ESearchCase::IgnoreCase))
        {
            if (!FitsUInt16(Event.WeatherId))
                return false;

            Native.kind = REMASTER_EMERALD_COORD_WEATHER;
            Native.weather = static_cast<uint16>(Event.WeatherId);
        }
        else if (Event.Type.Equals(TEXT("trigger"), ESearchCase::IgnoreCase))
        {
            if (!FitsUInt16(Event.VarId)
                || !FitsUInt16(Event.VarValueNum))
            {
                return false;
            }

            Native.kind = REMASTER_EMERALD_COORD_TRIGGER;
            Native.trigger = static_cast<uint16>(Event.VarId);
            Native.index = static_cast<uint16>(Event.VarValueNum);

            /*
             * The portable processor needs only null-vs-present script
             * identity here. The authoritative FString identity is recovered
             * from the same source index below; never retain a temporary UTF-8
             * pointer across the call.
             */
            Native.script_id =
                Event.ScriptId.IsEmpty() && Event.Script.IsEmpty()
                    ? nullptr
                    : "r4-generated-script";
        }
        else
        {
            return false;
        }

        NativeCoords.Add(Native);
    }

    if (CoordStartIndex > NativeCoords.Num())
        return false;

    TArray<RemasterEmeraldWarpEventDef> NativeWarps;
    NativeWarps.Reserve(CurrentMap.WarpEvents.Num());

    for (const FRemasterWarpEventIR& Event : CurrentMap.WarpEvents)
    {
        if (!FitsInt16(Event.X)
            || !FitsInt16(Event.Y)
            || !FitsUInt8(Event.Elevation)
            || !FitsUInt8(Event.DestWarpIdNum))
        {
            return false;
        }

        if (!Event.bDynamicTarget
            && (!FitsUInt8(Event.DestGroupNum)
                || !FitsUInt8(Event.DestMapNum)))
        {
            return false;
        }

        RemasterEmeraldWarpEventDef Native{};
        Native.x = static_cast<int16>(Event.X);
        Native.y = static_cast<int16>(Event.Y);
        Native.elevation = static_cast<uint8>(Event.Elevation);
        Native.dest_warp_id =
            static_cast<uint8>(Event.DestWarpIdNum);
        Native.dest_map_group = Event.bDynamicTarget
            ? 0u
            : static_cast<uint8>(Event.DestGroupNum);
        Native.dest_map_num = Event.bDynamicTarget
            ? 0u
            : static_cast<uint8>(Event.DestMapNum);
        NativeWarps.Add(Native);
    }

    RemasterEmeraldStepEventResult NativeResult{};
    if (!remaster_emerald_process_step_events(
            Save,
            NativeCoords.IsEmpty() ? nullptr : NativeCoords.GetData(),
            static_cast<size_t>(NativeCoords.Num()),
            static_cast<size_t>(CoordStartIndex),
            NativeWarps.IsEmpty() ? nullptr : NativeWarps.GetData(),
            static_cast<size_t>(NativeWarps.Num()),
            State.player_x,
            State.player_y,
            remaster_emerald_map_elevation_at(
                &MapView,
                State.player_x,
                State.player_y),
            remaster_emerald_map_behavior_at(
                &MapView,
                State.player_x,
                State.player_y),
            nullptr,
            nullptr,
            &NativeResult))
    {
        return false;
    }

    OutResult.ScriptId.Reset();
    OutResult.NextCoordEventIndex = -1;
    OutResult.bWeatherChanged = NativeResult.weather_changed != 0u;
    OutResult.WeatherId = NativeResult.weather_changed
        ? static_cast<int32>(NativeResult.weather)
        : -1;

    switch (NativeResult.kind)
    {
    case REMASTER_EMERALD_STEP_EVENT_NONE:
        OutResult.Kind = ERemasterPlayerStepKind::Moved;
        return PopulatePlayerStepSnapshot(
            Save,
            CurrentMap,
            OutResult);

    case REMASTER_EMERALD_STEP_EVENT_IMMEDIATE_SCRIPT:
    case REMASTER_EMERALD_STEP_EVENT_COORD_SCRIPT:
    {
        if (NativeResult.coord_event_index
            >= static_cast<size_t>(CurrentMap.CoordEvents.Num()))
        {
            return false;
        }

        const FRemasterCoordEventIR& Source =
            CurrentMap.CoordEvents[
                static_cast<int32>(NativeResult.coord_event_index)];

        OutResult.ScriptId = !Source.ScriptId.IsEmpty()
            ? Source.ScriptId
            : Source.Script;
        OutResult.NextCoordEventIndex =
            NativeResult.next_coord_event_index
                <= static_cast<size_t>(MAX_int32)
            ? static_cast<int32>(
                NativeResult.next_coord_event_index)
            : -1;
        OutResult.Kind =
            NativeResult.kind
                == REMASTER_EMERALD_STEP_EVENT_IMMEDIATE_SCRIPT
            ? ERemasterPlayerStepKind::ImmediateCoordScript
            : ERemasterPlayerStepKind::CoordScript;

        return PopulatePlayerStepSnapshot(
            Save,
            CurrentMap,
            OutResult);
    }

    case REMASTER_EMERALD_STEP_EVENT_WARP:
    {
        if (NativeResult.warp_index
            >= static_cast<size_t>(CurrentMap.WarpEvents.Num()))
        {
            return false;
        }

        const int32 SourceIndex =
            static_cast<int32>(NativeResult.warp_index);
        const FRemasterWarpEventIR& Source =
            CurrentMap.WarpEvents[SourceIndex];

        FRemasterResolvedWarp Warp{};
        Warp.SourceEventIndex = SourceIndex;
        Warp.DestGroupNum = Source.DestGroupNum;
        Warp.DestMapNum = Source.DestMapNum;
        Warp.DestWarpId = Source.DestWarpIdNum;
        Warp.DestMap = Source.DestMap;
        Warp.bDynamicTarget = Source.bDynamicTarget;

        if (!ApplyResolvedWarp(Warp))
            return false;

        OutResult.Kind = ERemasterPlayerStepKind::Warp;
        return PopulatePlayerStepSnapshot(
            Save,
            CurrentMap,
            OutResult);
    }

    default:
        return false;
    }
}

bool URemasterWorldGameplaySubsystem::ContinuePlayerStepEvents(
    int32 CoordStartIndex,
    FRemasterPlayerStepResult& OutResult)
{
    OutResult = FRemasterPlayerStepResult{};
    return ProcessCurrentStepEvents(CoordStartIndex, OutResult);
}


bool URemasterWorldGameplaySubsystem::StepPlayer(
    int32 Direction,
    FRemasterPlayerStepResult& OutResult)
{
    OutResult = FRemasterPlayerStepResult{};

    if (!bMapReady
        || !CurrentMap.IsValid()
        || Direction < REMASTER_EMERALD_DIR_SOUTH
        || Direction > REMASTER_EMERALD_DIR_EAST
        || !GetGameInstance())
    {
        return false;
    }

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

    RemasterEmeraldMapView MapView{};
    MapView.width = CurrentMap.Width;
    MapView.height = CurrentMap.Height;
    MapView.blocks = CurrentMap.RawBlocks.GetData();
    MapView.block_count =
        static_cast<size_t>(CurrentMap.RawBlocks.Num());
    MapView.border = CurrentMap.BorderActiveWords.GetData();
    MapView.border_count =
        static_cast<size_t>(CurrentMap.BorderActiveWords.Num());
    MapView.primary_attributes =
        CurrentMap.PrimaryMetatileAttributes.GetData();
    MapView.primary_attribute_count =
        static_cast<size_t>(CurrentMap.PrimaryMetatileAttributes.Num());
    MapView.secondary_attributes =
        CurrentMap.SecondaryMetatileAttributes.GetData();
    MapView.secondary_attribute_count =
        static_cast<size_t>(CurrentMap.SecondaryMetatileAttributes.Num());

    if (!NativeObjectRuntime || !SyncRuntimeObjectView())
        return false;

    TArray<RemasterEmeraldObjectCollider> ObjectColliders;
    ObjectColliders.SetNumUninitialized(
        REMASTER_EMERALD_RUNTIME_OBJECT_COUNT);

    size_t RuntimeColliderCount = 0;
    if (!remaster_emerald_object_runtime_build_colliders(
            static_cast<const RemasterEmeraldObjectRuntime*>(
                NativeObjectRuntime),
            ObjectColliders.GetData(),
            static_cast<size_t>(ObjectColliders.Num()),
            &RuntimeColliderCount))
    {
        return false;
    }

    ObjectColliders.SetNum(
        static_cast<int32>(RuntimeColliderCount),
        EAllowShrinking::No);

    TArray<RemasterEmeraldConnectionDef> NativeConnections;
    TArray<int32> ConnectionSourceIndices;
    NativeConnections.Reserve(CurrentMap.Connections.Num());
    ConnectionSourceIndices.Reserve(CurrentMap.Connections.Num());

    for (int32 Index = 0; Index < CurrentMap.Connections.Num(); ++Index)
    {
        const FRemasterConnectionIR& Source =
            CurrentMap.Connections[Index];
        const int32 NativeDirection =
            ConnectionDirectionFromString(Source.Direction);

        if (NativeDirection < REMASTER_EMERALD_DIR_SOUTH
            || NativeDirection > REMASTER_EMERALD_DIR_EAST
            || !FitsUInt8(Source.DestGroupNum)
            || !FitsUInt8(Source.DestMapNum))
        {
            continue;
        }

        FRemasterMapIR Target;
        FString Error;
        if (!CatalogSubsystem->LoadMap(
                Source.DestGroupNum,
                Source.DestMapNum,
                Target,
                Error))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Player-step connection target load failed for %d,%d: %s"),
                Source.DestGroupNum,
                Source.DestMapNum,
                *Error);
            continue;
        }

        if (Target.Width <= 0
            || Target.Width > MAX_int16
            || Target.Height <= 0
            || Target.Height > MAX_int16)
        {
            continue;
        }

        RemasterEmeraldConnectionDef Native{};
        Native.direction = static_cast<uint8>(NativeDirection);
        Native.offset = Source.Offset;
        Native.dest_map_group =
            static_cast<uint8>(Source.DestGroupNum);
        Native.dest_map_num =
            static_cast<uint8>(Source.DestMapNum);
        Native.dest_width = static_cast<int16>(Target.Width);
        Native.dest_height = static_cast<int16>(Target.Height);

        NativeConnections.Add(Native);
        ConnectionSourceIndices.Add(Index);
    }

    RemasterEmeraldMovementContext Movement{};
    Movement.map = &MapView;
    Movement.objects = ObjectColliders.IsEmpty()
        ? nullptr
        : ObjectColliders.GetData();
    Movement.object_count =
        static_cast<size_t>(ObjectColliders.Num());

    RemasterEmeraldPlayerStepResult NativeResult{};
    if (!remaster_emerald_player_step(
            Save,
            &Movement,
            NativeConnections.IsEmpty()
                ? nullptr
                : NativeConnections.GetData(),
            static_cast<size_t>(NativeConnections.Num()),
            nullptr,
            0u,
            static_cast<uint8>(Direction),
            &NativeResult))
    {
        return false;
    }

    OutResult.Collision =
        static_cast<int32>(NativeResult.collision);
    OutResult.PlayerX = NativeResult.x;
    OutResult.PlayerY = NativeResult.y;
    OutResult.Elevation = NativeResult.elevation;
    OutResult.MapId = CurrentMap.Id;
    OutResult.MapGroup = CurrentMap.GroupNum;
    OutResult.MapNum = CurrentMap.MapNum;

    switch (NativeResult.kind)
    {
    case REMASTER_EMERALD_PLAYER_STEP_BLOCKED:
        OutResult.Kind = ERemasterPlayerStepKind::Blocked;
        return true;

    case REMASTER_EMERALD_PLAYER_STEP_MOVED:
        if (!SyncRuntimeObjectView())
            return false;
        return ProcessCurrentStepEvents(0, OutResult);

    case REMASTER_EMERALD_PLAYER_STEP_WARP:
        /* Production passes no warps to player_step; step events own them. */
        return false;

    case REMASTER_EMERALD_PLAYER_STEP_CONNECTION:
    {
        if (NativeResult.connection_index
                >= static_cast<size_t>(
                    ConnectionSourceIndices.Num()))
        {
            return false;
        }

        const int32 SourceIndex =
            ConnectionSourceIndices[
                static_cast<int32>(
                    NativeResult.connection_index)];

        if (!CurrentMap.Connections.IsValidIndex(SourceIndex))
            return false;

        const FRemasterConnectionIR& Source =
            CurrentMap.Connections[SourceIndex];

        FRemasterResolvedConnection Connection{};
        Connection.SourceConnectionIndex = SourceIndex;
        Connection.Direction = Direction;
        Connection.Offset = Source.Offset;
        Connection.DestGroupNum = Source.DestGroupNum;
        Connection.DestMapNum = Source.DestMapNum;
        Connection.DestMap = Source.Map;

        if (!ApplyResolvedConnection(Connection))
            return false;

        OutResult.Kind = ERemasterPlayerStepKind::Connection;
        return PopulatePlayerStepSnapshot(
            Save,
            CurrentMap,
            OutResult);
    }

    case REMASTER_EMERALD_PLAYER_STEP_INVALID:
    default:
        return false;
    }
}


bool URemasterWorldGameplaySubsystem::ResolveConnection(
    int32 Direction,
    FRemasterResolvedConnection& OutConnection) const
{
    OutConnection = FRemasterResolvedConnection{};

    if (!bMapReady
        || Direction < REMASTER_EMERALD_DIR_SOUTH
        || Direction > REMASTER_EMERALD_DIR_EAST
        || !GetGameInstance()
        || CurrentMap.Width <= 0
        || CurrentMap.Width > MAX_int16
        || CurrentMap.Height <= 0
        || CurrentMap.Height > MAX_int16)
    {
        return false;
    }

    const URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    URemasterWorldCatalogSubsystem* CatalogSubsystem =
        GetGameInstance()->GetSubsystem<URemasterWorldCatalogSubsystem>();

    if (!SaveSubsystem
        || !CatalogSubsystem
        || !SaveSubsystem->HasUsableSave())
    {
        return false;
    }

    const RemasterEmeraldSave* Save =
        static_cast<const RemasterEmeraldSave*>(
            SaveSubsystem->GetNativeSaveHandle());

    if (!Save)
        return false;

    RemasterEmeraldOverworldState State{};
    if (!remaster_emerald_overworld_get(Save, &State))
        return false;

    TArray<RemasterEmeraldConnectionDef> NativeConnections;
    TArray<int32> SourceIndices;

    NativeConnections.Reserve(CurrentMap.Connections.Num());
    SourceIndices.Reserve(CurrentMap.Connections.Num());

    for (int32 Index = 0; Index < CurrentMap.Connections.Num(); ++Index)
    {
        const FRemasterConnectionIR& Source = CurrentMap.Connections[Index];
        const int32 NativeDirection =
            ConnectionDirectionFromString(Source.Direction);

        if (NativeDirection != Direction
            || !FitsUInt8(Source.DestGroupNum)
            || !FitsUInt8(Source.DestMapNum))
        {
            continue;
        }

        FRemasterMapIR Target;
        FString Error;
        if (!CatalogSubsystem->LoadMap(
                Source.DestGroupNum,
                Source.DestMapNum,
                Target,
                Error))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Connection target map load failed for %d,%d: %s"),
                Source.DestGroupNum,
                Source.DestMapNum,
                *Error);
            continue;
        }

        if (Target.Width <= 0
            || Target.Width > MAX_int16
            || Target.Height <= 0
            || Target.Height > MAX_int16)
        {
            continue;
        }

        RemasterEmeraldConnectionDef Native{};
        Native.direction = static_cast<uint8>(NativeDirection);
        Native.offset = Source.Offset;
        Native.dest_map_group = static_cast<uint8>(Source.DestGroupNum);
        Native.dest_map_num = static_cast<uint8>(Source.DestMapNum);
        Native.dest_width = static_cast<int16>(Target.Width);
        Native.dest_height = static_cast<int16>(Target.Height);

        NativeConnections.Add(Native);
        SourceIndices.Add(Index);
    }

    if (NativeConnections.IsEmpty())
        return false;

    size_t MatchIndex = 0;
    if (!remaster_emerald_find_incoming_connection(
            NativeConnections.GetData(),
            static_cast<size_t>(NativeConnections.Num()),
            static_cast<uint8>(Direction),
            State.player_x,
            State.player_y,
            static_cast<int16>(CurrentMap.Width),
            static_cast<int16>(CurrentMap.Height),
            &MatchIndex))
    {
        return false;
    }

    if (MatchIndex >= static_cast<size_t>(SourceIndices.Num()))
        return false;

    const int32 SourceIndex =
        SourceIndices[static_cast<int32>(MatchIndex)];

    if (!CurrentMap.Connections.IsValidIndex(SourceIndex))
        return false;

    const FRemasterConnectionIR& Source =
        CurrentMap.Connections[SourceIndex];

    OutConnection.SourceConnectionIndex = SourceIndex;
    OutConnection.Direction = Direction;
    OutConnection.Offset = Source.Offset;
    OutConnection.DestGroupNum = Source.DestGroupNum;
    OutConnection.DestMapNum = Source.DestMapNum;
    OutConnection.DestMap = Source.Map;
    return true;
}

bool URemasterWorldGameplaySubsystem::ApplyResolvedConnection(
    const FRemasterResolvedConnection& Connection)
{
    if (!bMapReady
        || !GetGameInstance()
        || Connection.Direction < REMASTER_EMERALD_DIR_SOUTH
        || Connection.Direction > REMASTER_EMERALD_DIR_EAST
        || !FitsUInt8(Connection.DestGroupNum)
        || !FitsUInt8(Connection.DestMapNum))
    {
        return false;
    }

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

    FRemasterMapIR TargetMap;
    FString Error;
    if (!CatalogSubsystem->LoadMap(
            Connection.DestGroupNum,
            Connection.DestMapNum,
            TargetMap,
            Error))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Connection target map load failed for %d,%d: %s"),
            Connection.DestGroupNum,
            Connection.DestMapNum,
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
        return false;
    }

    RemasterEmeraldConnectionDef Native{};
    Native.direction = static_cast<uint8>(Connection.Direction);
    Native.offset = Connection.Offset;
    Native.dest_map_group = static_cast<uint8>(Connection.DestGroupNum);
    Native.dest_map_num = static_cast<uint8>(Connection.DestMapNum);
    Native.dest_width = static_cast<int16>(TargetMap.Width);
    Native.dest_height = static_cast<int16>(TargetMap.Height);

    if (!remaster_emerald_apply_connection_transition(
            Save,
            &Native,
            static_cast<uint16>(TargetMap.LayoutNum),
            static_cast<uint8>(TargetMap.WeatherId),
            static_cast<uint8>(TargetMap.MapTypeId),
            TargetMap.bRequiresFlash ? 1 : 0))
    {
        return false;
    }

    if (!RefreshSavedObjectTemplateCache(TargetMap))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Failed to refresh object template cache after connection: %s"),
            *TargetMap.Id);
        return false;
    }

    CurrentMap = MoveTemp(TargetMap);

    if (!RebuildRuntimeObjectState())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Failed to rebuild runtime objects after connection: %s"),
            *CurrentMap.Id);
        bMapReady = false;
        return false;
    }

    bMapReady = true;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Vanilla+ connection applied: map=%s (%d,%d) dir=%d offset=%d"),
        *CurrentMap.Id,
        CurrentMap.GroupNum,
        CurrentMap.MapNum,
        Connection.Direction,
        Connection.Offset);

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

    if (!RefreshSavedObjectTemplateCache(TargetMap))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Failed to refresh object template cache after warp: %s"),
            *TargetMap.Id);
        return false;
    }

    CurrentMap = MoveTemp(TargetMap);

    if (!RebuildRuntimeObjectState())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Failed to rebuild runtime objects after warp: %s"),
            *CurrentMap.Id);
        bMapReady = false;
        return false;
    }

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
