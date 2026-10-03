#include "RemasterVanillaPlusSaveSubsystem.h"

#include "HAL/UnrealMemory.h"
#include "Misc/Paths.h"
#include "RemasterCoreSubsystem.h"

extern "C"
{
#include "remaster/emerald_rtc.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"
}

namespace
{
constexpr const char* LegacySlot = "vanillaplus.sav";

ERemasterLegacySaveStatus ToUnrealStatus(RemasterEmeraldSaveStatus Status)
{
    switch (Status)
    {
    case REMASTER_EMERALD_SAVE_OK:
        return ERemasterLegacySaveStatus::Ok;
    case REMASTER_EMERALD_SAVE_DEGRADED:
        return ERemasterLegacySaveStatus::Degraded;
    case REMASTER_EMERALD_SAVE_CORRUPT:
        return ERemasterLegacySaveStatus::Corrupt;
    case REMASTER_EMERALD_SAVE_EMPTY:
    default:
        return ERemasterLegacySaveStatus::Empty;
    }
}
}

void URemasterVanillaPlusSaveSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<URemasterCoreSubsystem>();
    Super::Initialize(Collection);

    NativeSave = FMemory::Malloc(
        sizeof(RemasterEmeraldSave),
        alignof(RemasterEmeraldSave));

    if (NativeSave)
    {
        FMemory::Memzero(NativeSave, sizeof(RemasterEmeraldSave));
    }

    ScratchImage.SetNumUninitialized(
        REMASTER_EMERALD_SAVE_IMAGE_BYTES);

    LoadLegacySave();
}

void URemasterVanillaPlusSaveSubsystem::Deinitialize()
{
    if (NativeSave)
    {
        FMemory::Free(NativeSave);
        NativeSave = nullptr;
    }

    ScratchImage.Reset();
    Status = ERemasterLegacySaveStatus::Empty;

    Super::Deinitialize();
}

ERemasterLegacySaveStatus
URemasterVanillaPlusSaveSubsystem::LoadLegacySave()
{
    if (!NativeSave
        || ScratchImage.Num() != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
    {
        Status = ERemasterLegacySaveStatus::Corrupt;
        return Status;
    }

    RemasterEmeraldSave* Save =
        static_cast<RemasterEmeraldSave*>(NativeSave);

    const RemasterEmeraldSaveStatus NativeStatus =
        remaster_emerald_save_load_platform(
            LegacySlot,
            ScratchImage.GetData(),
            static_cast<size_t>(ScratchImage.Num()),
            Save);

    Status = ToUnrealStatus(NativeStatus);

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Vanilla+ save load status=%d path=%s counter=%u slot=%u"),
        static_cast<int32>(Status),
        *GetLegacySavePath(),
        Save->counter,
        Save->selected_slot);

    return Status;
}

bool URemasterVanillaPlusSaveSubsystem::StoreLegacySave()
{
    if (!HasUsableSave() || !NativeSave
        || ScratchImage.Num() != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
    {
        return false;
    }

    RemasterEmeraldSave* Save =
        static_cast<RemasterEmeraldSave*>(NativeSave);

    if (!remaster_emerald_save_store_platform(
            LegacySlot,
            ScratchImage.GetData(),
            static_cast<size_t>(ScratchImage.Num()),
            Save))
    {
        return false;
    }

    Status = ERemasterLegacySaveStatus::Ok;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Vanilla+ save stored counter=%u slot=%u"),
        Save->counter,
        Save->selected_slot);

    return true;
}

bool URemasterVanillaPlusSaveSubsystem::GetOverworldSnapshot(
    FRemasterLegacyOverworldSnapshot& OutSnapshot) const
{
    if (!HasUsableSave() || !NativeSave)
        return false;

    RemasterEmeraldOverworldState Native{};
    if (!remaster_emerald_overworld_get(
            static_cast<const RemasterEmeraldSave*>(NativeSave),
            &Native))
    {
        return false;
    }

    OutSnapshot.PlayerX = Native.player_x;
    OutSnapshot.PlayerY = Native.player_y;
    OutSnapshot.MapGroup = Native.map_group;
    OutSnapshot.MapNum = Native.map_num;
    OutSnapshot.WarpId = Native.warp_id;
    OutSnapshot.WarpX = Native.warp_x;
    OutSnapshot.WarpY = Native.warp_y;
    OutSnapshot.MapLayoutId = Native.map_layout_id;
    OutSnapshot.SavedMusic = Native.saved_music;
    OutSnapshot.Weather = Native.weather;
    OutSnapshot.WeatherCycleStage = Native.weather_cycle_stage;
    OutSnapshot.FlashLevel = Native.flash_level;
    OutSnapshot.PartyCount = Native.party_count;
    OutSnapshot.Money = static_cast<int64>(Native.money);
    OutSnapshot.Coins = Native.coins;
    OutSnapshot.RegisteredItem = Native.registered_item;

    return true;
}

bool URemasterVanillaPlusSaveSubsystem::GetPersistentFlag(
    int32 FlagId,
    bool& OutValue) const
{
    if (!HasUsableSave() || !NativeSave
        || FlagId < 0 || FlagId > MAX_uint16)
        return false;

    int NativeValue = 0;
    if (!remaster_emerald_flag_get(
            static_cast<const RemasterEmeraldSave*>(NativeSave),
            static_cast<uint16>(FlagId),
            &NativeValue))
    {
        return false;
    }

    OutValue = NativeValue != 0;
    return true;
}

bool URemasterVanillaPlusSaveSubsystem::SetPersistentFlag(
    int32 FlagId,
    bool bValue)
{
    if (!HasUsableSave() || !NativeSave
        || FlagId < 0 || FlagId > MAX_uint16)
        return false;

    return remaster_emerald_flag_set(
        static_cast<RemasterEmeraldSave*>(NativeSave),
        static_cast<uint16>(FlagId),
        bValue ? 1 : 0) != 0;
}

bool URemasterVanillaPlusSaveSubsystem::GetPersistentVar(
    int32 VarId,
    int32& OutValue) const
{
    if (!HasUsableSave() || !NativeSave
        || VarId < 0 || VarId > MAX_uint16)
        return false;

    uint16 NativeValue = 0;
    if (!remaster_emerald_var_get(
            static_cast<const RemasterEmeraldSave*>(NativeSave),
            static_cast<uint16>(VarId),
            &NativeValue))
    {
        return false;
    }

    OutValue = NativeValue;
    return true;
}

bool URemasterVanillaPlusSaveSubsystem::SetPersistentVar(
    int32 VarId,
    int32 Value)
{
    if (!HasUsableSave() || !NativeSave
        || VarId < 0 || VarId > MAX_uint16
        || Value < 0 || Value > MAX_uint16)
        return false;

    return remaster_emerald_var_set(
        static_cast<RemasterEmeraldSave*>(NativeSave),
        static_cast<uint16>(VarId),
        static_cast<uint16>(Value)) != 0;
}

bool URemasterVanillaPlusSaveSubsystem::GetLocalRtcNow(
    FRemasterLegacyRtcSnapshot& OutTime) const
{
    if (!HasUsableSave() || !NativeSave)
        return false;

    RemasterEmeraldTime Time{};
    if (!remaster_emerald_rtc_local_now(
            static_cast<const RemasterEmeraldSave*>(NativeSave),
            &Time))
    {
        return false;
    }

    OutTime.Days = Time.days;
    OutTime.Hours = Time.hours;
    OutTime.Minutes = Time.minutes;
    OutTime.Seconds = Time.seconds;
    return true;
}

bool URemasterVanillaPlusSaveSubsystem::RealignRtcNow(
    const FRemasterLegacyRtcSnapshot& DesiredLocalTime)
{
    if (!HasUsableSave() || !NativeSave)
        return false;

    RemasterEmeraldTime Time{};
    Time.days = static_cast<int16>(DesiredLocalTime.Days);
    Time.hours = static_cast<int8>(DesiredLocalTime.Hours);
    Time.minutes = static_cast<int8>(DesiredLocalTime.Minutes);
    Time.seconds = static_cast<int8>(DesiredLocalTime.Seconds);

    return remaster_emerald_rtc_realign_now(
        static_cast<RemasterEmeraldSave*>(NativeSave),
        Time) != 0;
}

FString URemasterVanillaPlusSaveSubsystem::GetLegacySavePath() const
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Core"),
        TEXT("vanillaplus.sav"));
}
