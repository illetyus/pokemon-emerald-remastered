#include "RemasterVanillaPlusSaveSubsystem.h"

#include "HAL/UnrealMemory.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"
#include "RemasterWorldCatalogSubsystem.h"
#include "RemasterSaveImport.h"
#include "RemasterCoreSubsystem.h"

extern "C"
{
#include "remaster/emerald_rtc.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"
}

namespace
{

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
    case REMASTER_EMERALD_SAVE_IO_ERROR:
        return ERemasterLegacySaveStatus::IoError;
    case REMASTER_EMERALD_SAVE_UNSUPPORTED:
        return ERemasterLegacySaveStatus::Unsupported;
    case REMASTER_EMERALD_SAVE_EMPTY:
        return ERemasterLegacySaveStatus::Empty;
    default:
        return ERemasterLegacySaveStatus::Corrupt;
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

    bSelectionValid=ReadSessionSelection();
    if(bSelectionValid)LoadLegacySave();
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
    if(!bSelectionValid)return Status;
    if (!NativeSave
        || ScratchImage.Num() != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
    {
        Status = ERemasterLegacySaveStatus::Corrupt;
        return Status;
    }

    RemasterEmeraldSave* Save =
        static_cast<RemasterEmeraldSave*>(NativeSave);

    const RemasterEmeraldSaveStatus NativeStatus =
        remaster_emerald_save_load_platform_format(
            TCHAR_TO_UTF8(*ActiveSlot),
            ScratchImage.GetData(),
            static_cast<size_t>(ScratchImage.Num()),
            static_cast<RemasterEmeraldSaveFormat>(ActiveFormat),
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
            TCHAR_TO_UTF8(*ActiveSlot),
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
        ActiveSlot);
}

namespace {
constexpr const char* SelectionSlot="session-selection.dat";
bool ValidSessionSlot(const FString& Slot) {
    if(Slot==TEXT("vanillaplus.sav"))return true;
    if(Slot.Len()!=43 || !Slot.StartsWith(TEXT("import-")) || !Slot.EndsWith(TEXT(".sav")))return false;
    for(int32 i=7;i<39;++i)if(!FChar::IsHexDigit(Slot[i]))return false;
    return true;
}
}

bool URemasterVanillaPlusSaveSubsystem::ReadSessionSelection()
{
    const auto* Platform=remaster_platform_get();
    uint8 Bytes[256]{};size_t Size=0;
    Status=ERemasterLegacySaveStatus::Unsupported;
    if(!Platform || !Platform->save_read_result){Status=ERemasterLegacySaveStatus::IoError;return false;}
    const auto Result=Platform->save_read_result(Platform->userdata,SelectionSlot,Bytes,255,&Size);
    if(Result==REMASTER_SAVE_READ_MISSING)return true;
    if(Result!=REMASTER_SAVE_READ_OK){Status=ERemasterLegacySaveStatus::IoError;return false;}
    if(Size==0 || Size>255)return false;
    for(size_t i=0;i<Size;++i)if(Bytes[i]==0 || Bytes[i]>127)return false;
    const FString Text=UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes));
    TArray<FString> Lines;Text.ParseIntoArrayLines(Lines,true);
    if(Lines.Num()!=3 || Lines[0]!=TEXT("AP04/1") || !ValidSessionSlot(Lines[1])
        || (Lines[2]!=TEXT("0") && Lines[2]!=TEXT("1")))return false;
    if(Text!=FString::Printf(TEXT("AP04/1\n%s\n%s\n"),*Lines[1],*Lines[2]))return false;
    ActiveSlot=Lines[1];ActiveFormat=Lines[2]==TEXT("0")?0:1;
    return true;
}
bool URemasterVanillaPlusSaveSubsystem::PersistSessionSelection(const FString& Slot,uint8 Format)
{
    if(!ValidSessionSlot(Slot) || Format>1)return false;
    const auto* Platform=remaster_platform_get();
    const FTCHARToUTF8 Text(*FString::Printf(TEXT("AP04/1\n%s\n%d\n"),*Slot,Format));
    return Platform && Platform->save_write && Platform->save_write(Platform->userdata,
        SelectionSlot,reinterpret_cast<const uint8*>(Text.Get()),static_cast<size_t>(Text.Length()))!=0;
}
bool URemasterVanillaPlusSaveSubsystem::CreateNewGame(const FString& Name,uint8 Avatar,
    uint32 Seed,uint16 TrainerLower,RemasterEmeraldNewGameRuntime& OutRuntime,FString& Error)
{
    if(!bSelectionValid || !NativeSave || Status!=ERemasterLegacySaveStatus::Empty) {
        Error=TEXT("Mevcut kayıt korunuyor. Yeni Oyun için boş kayıt alanı gerekiyor.");return false;
    }
    auto Probe=MakeUnique<RemasterEmeraldSave>();
    const auto ProbeSlot=[&](const FString& Slot,uint8 Format) {
        return remaster_emerald_save_load_platform_format(TCHAR_TO_UTF8(*Slot),ScratchImage.GetData(),
            static_cast<size_t>(ScratchImage.Num()),static_cast<RemasterEmeraldSaveFormat>(Format),Probe.Get());
    };
    if(ProbeSlot(ActiveSlot,ActiveFormat)!=REMASTER_EMERALD_SAVE_EMPTY) {
        Error=TEXT("Seçili kayıt alanı boş değil; Yeni Oyun başlatılmadı.");return false;
    }
    const FString Destination=TEXT("vanillaplus.sav");
    if(ActiveSlot!=Destination && ProbeSlot(Destination,1)!=REMASTER_EMERALD_SAVE_EMPTY) {
        Error=TEXT("Ana kayıt alanında bir kayıt var; önceki oturum seçimi korunuyor.");return false;
    }
    const FTCHARToUTF8 EncodedName(*Name);
    RemasterEmeraldNewGameInitRequest Request{};
    Request.identity.format=REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    Request.identity.avatar=Avatar;Request.identity.rng_state=Seed;Request.identity.trainer_id_lower=TrainerLower;
    Request.observed_save_status=REMASTER_EMERALD_SAVE_EMPTY;
    Request.ranking_id_policy=REMASTER_EMERALD_NEW_GAME_SOURCE_UBFIX_ZERO_ID;
    // GBA RTC reset date is 2000-01-01: source ConvertDateToDayCount returns 1.
    Request.rtc_after_reset.days=1;
    if(remaster_emerald_new_game_encode_name(EncodedName.Get(),static_cast<size_t>(EncodedName.Length()),Request.identity.player_name)
        !=REMASTER_EMERALD_NEW_GAME_PREPARED) {Error=TEXT("Ad 1–7 geçerli karakterden oluşmalı.");return false;}
    auto Work=MakeUnique<RemasterEmeraldSave>();auto Candidate=MakeUnique<RemasterEmeraldSave>();
    RemasterEmeraldNewGameRuntime Runtime{};
    if(remaster_emerald_new_game_initialize(&Request,Work.Get(),Candidate.Get(),&Runtime)
        !=REMASTER_EMERALD_NEW_GAME_INITIALIZED) {Error=TEXT("Başlangıç kaydı oluşturulamadı.");return false;}
    // Modern-host adaptation: emulate reset in the game's persistent local RTC
    // offset, never change the PC/device clock. Preserve source berry-update zero.
    const auto BerryTime=remaster_emerald_save_get_last_berry_update(Candidate.Get());
    if(!remaster_emerald_rtc_realign_now(Candidate.Get(),Request.rtc_after_reset)) {
        Error=TEXT("Oyun saati başlatılamadı.");return false;
    }
    remaster_emerald_save_set_last_berry_update(Candidate.Get(),BerryTime);
    auto* Catalog=GetGameInstance()->GetSubsystem<URemasterWorldCatalogSubsystem>();
    FRemasterMapIR Map;FString MapError;
    if(!Catalog || !Catalog->LoadMap(25,40,Map,MapError) || Map.Width<=2 || Map.Height<=2) {
        UE_LOG(LogTemp,Error,TEXT("AP04_NEW_GAME_MAP_ERROR %s"),*MapError);
        Error=TEXT("Başlangıç haritası açılamadı.");return false;
    }
    const FTCHARToUTF8 Slot(*Destination);
    if(!remaster_emerald_save_store_platform(Slot.Get(),ScratchImage.GetData(),
        static_cast<size_t>(ScratchImage.Num()),Candidate.Get())) {
        Error=TEXT("Başlangıç kaydı yazılamadı. Önceki dosyalar ve oturum seçimi korundu.");return false;
    }
    if(!PersistSessionSelection(Destination,1)) {
        Error=TEXT("Başlangıç kaydı vanillaplus.sav dosyasında korundu; oturum seçimi kaydedilemedi. Kayıt Aktar ile bu dosyayı açabilirsin.");return false;
    }
    ActiveSlot=Destination;ActiveFormat=1;
    *static_cast<RemasterEmeraldSave*>(NativeSave)=*Candidate;
    ActiveFormat=1;Status=ToUnrealStatus(Candidate->status);OutRuntime=Runtime;
    UE_LOG(LogTemp,Display,TEXT("AP04_NEW_GAME_CREATED avatar=%u rng=%u calls=%u clock_policy=game_local_reset"),Avatar,Runtime.rng_state,Runtime.rng_calls);
    return true;
}
bool URemasterVanillaPlusSaveSubsystem::ImportCopy(const FString& SourcePath,uint8 Format,FString& Error)
{
    if(SourcePath.IsEmpty() || SourcePath.Len()>4096 || Format>1 || !NativeSave)return false;
    const FString Guid=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Directory=IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(
        *FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Core")));
    if(!IFileManager::Get().MakeDirectory(*Directory,true)){Error=TEXT("Kopya klasörü oluşturulamadı.");return false;}
    const FString RawCopy=FPaths::Combine(Directory,TEXT("quarantine-import-")+Guid+TEXT(".sav"));
    const FString Slot=TEXT("import-")+Guid+TEXT(".sav");
    const FString WorkingCopy=FPaths::Combine(Directory,Slot);
    auto Candidate=MakeUnique<RemasterEmeraldSave>();
    *Candidate=*static_cast<const RemasterEmeraldSave*>(NativeSave);
#if PLATFORM_WINDOWS
    const auto Path=[](const FString& Value){return std::filesystem::path(*Value);};
#else
    const auto Path=[](const FString& Value){return std::filesystem::path(TCHAR_TO_UTF8(*Value));};
#endif
    const auto Result=RemasterSaveImport::CopyFirst(Path(SourcePath),Path(RawCopy),
        static_cast<RemasterEmeraldSaveFormat>(Format),*Candidate);
    if(Result.status!=REMASTER_EMERALD_SAVE_OK && Result.status!=REMASTER_EMERALD_SAVE_DEGRADED) {
        Error=Result.status==REMASTER_EMERALD_SAVE_UNSUPPORTED?TEXT("Seçilen kayıt biçimi desteklenmiyor."):
            Result.status==REMASTER_EMERALD_SAVE_CORRUPT?TEXT("Kayıt kopyası doğrulanamadı."):TEXT("Kayıt kopyası okunamadı veya oluşturulamadı.");
        return false;
    }
    TArray<uint8> Bytes;
    if(!FFileHelper::LoadFileToArray(Bytes,*RawCopy) || Bytes.Num()!=REMASTER_EMERALD_SAVE_IMAGE_BYTES
        || !RemasterSaveImport::WriteExclusive(Path(WorkingCopy),Bytes.GetData(),static_cast<size_t>(Bytes.Num()))
        || !PersistSessionSelection(Slot,Format)) {Error=TEXT("Çalışma kopyası seçilemedi; önceki kayıt korunuyor.");return false;}
    ActiveSlot=Slot;ActiveFormat=Format;bSelectionValid=true;Status=ToUnrealStatus(Result.status);
    *static_cast<RemasterEmeraldSave*>(NativeSave)=*Candidate;
    UE_LOG(LogTemp,Display,TEXT("AP04_IMPORT_COPY_SELECTED format=%u slot=%s"),Format,*Slot);
    return true;
}
