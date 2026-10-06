#include "RemasterPlatformUnreal.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
#include "RemasterAtomicSave.h"

#include "Engine/GameInstance.h"
#include "RemasterFeedbackSubsystem.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FString FRemasterPlatformUnreal::SlotPath(const char* Slot)
{
    const FString SafeSlot = UTF8_TO_TCHAR(Slot ? Slot : "default");
    const FString FileName = FPaths::GetExtension(SafeSlot).IsEmpty()
        ? SafeSlot + TEXT(".bin")
        : SafeSlot;

    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Core"),
        FileName);
}

void FRemasterPlatformUnreal::Install(UGameInstance* GameInstance)
{
    RemasterPlatformVTable Platform{};
    Platform.userdata = GameInstance;
    Platform.monotonic_time_ns = &MonotonicTimeNs;
    Platform.read_wall_clock = &ReadWallClock;
    Platform.save_read = &SaveRead;
    Platform.save_write = &SaveWrite;
    Platform.log = &Log;
    Platform.emit_presentation_event = &EmitPresentationEvent;

    remaster_platform_install(&Platform);
}

void FRemasterPlatformUnreal::Uninstall()
{
    remaster_platform_install(nullptr);
}

uint64 FRemasterPlatformUnreal::MonotonicTimeNs(void*)
{
    return static_cast<uint64>(
        FPlatformTime::Seconds() * 1000000000.0);
}

int FRemasterPlatformUnreal::ReadWallClock(
    void*,
    RemasterWallClock* OutClock)
{
    if (!OutClock)
    {
        return 0;
    }

    const FDateTime Now = FDateTime::Now();

    OutClock->year = Now.GetYear();
    OutClock->month = Now.GetMonth();
    OutClock->day = Now.GetDay();
    OutClock->hour = Now.GetHour();
    OutClock->minute = Now.GetMinute();
    OutClock->second = Now.GetSecond();

    return 1;
}

int FRemasterPlatformUnreal::SaveRead(
    void*,
    const char* Slot,
    uint8* Buffer,
    size_t Capacity,
    size_t* OutSize)
{
    if (!Buffer || !OutSize)
    {
        return 0;
    }

    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *SlotPath(Slot)))
    {
        return 0;
    }

    if (static_cast<size_t>(Bytes.Num()) > Capacity)
    {
        return 0;
    }

    FMemory::Memcpy(
        Buffer,
        Bytes.GetData(),
        Bytes.Num());

    *OutSize = static_cast<size_t>(Bytes.Num());
    return 1;
}

int FRemasterPlatformUnreal::SaveWrite(
    void*,
    const char* Slot,
    const uint8* Buffer,
    size_t Size)
{
    if ((!Buffer && Size > 0) || Size > static_cast<size_t>(MAX_int32))
    {
        return 0;
    }

    const FString Path = SlotPath(Slot);
    const FString Directory = FPaths::GetPath(Path);
    if (!IFileManager::Get().MakeDirectory(*Directory, true))
    {
        return 0;
    }
    const FString TemporaryPath = FPaths::CreateTempFilename(
        *Directory, TEXT("remaster-save-"), TEXT(".tmp"));

    TArray<uint8> Bytes;
    Bytes.Append(Buffer, static_cast<int32>(Size));

    return RemasterAtomicSave::Commit(
        [&]() { return FFileHelper::SaveArrayToFile(Bytes, *TemporaryPath); },
        [&]() {
#if PLATFORM_WINDOWS
            return RemasterAtomicSave::Replace(*TemporaryPath, *Path);
#else
            const FTCHARToUTF8 TemporaryUtf8(*TemporaryPath);
            const FTCHARToUTF8 DestinationUtf8(*Path);
            return RemasterAtomicSave::Replace(TemporaryUtf8.Get(), DestinationUtf8.Get());
#endif
        },
        [&]() { IFileManager::Get().Delete(*TemporaryPath, false, true, true); }) ? 1 : 0;
}

void FRemasterPlatformUnreal::Log(
    void*,
    RemasterLogLevel Level,
    const char* Message)
{
    const TCHAR* Text = UTF8_TO_TCHAR(
        Message ? Message : "");

    switch (Level)
    {
    case REMASTER_LOG_ERROR:
        UE_LOG(LogTemp, Error, TEXT("%s"), Text);
        break;
    case REMASTER_LOG_WARNING:
        UE_LOG(LogTemp, Warning, TEXT("%s"), Text);
        break;
    case REMASTER_LOG_DEBUG:
        UE_LOG(LogTemp, Verbose, TEXT("%s"), Text);
        break;
    case REMASTER_LOG_INFO:
    default:
        UE_LOG(LogTemp, Display, TEXT("%s"), Text);
        break;
    }
}

void FRemasterPlatformUnreal::EmitPresentationEvent(
    void* Userdata,
    uint32 EventId,
    int64 Arg0,
    int64 Arg1)
{
    UE_LOG(
        LogTemp,
        Verbose,
        TEXT("Core presentation event id=%u arg0=%lld arg1=%lld"),
        EventId,
        static_cast<long long>(Arg0),
        static_cast<long long>(Arg1));

    UGameInstance* GameInstance =
        static_cast<UGameInstance*>(Userdata);

    if (!GameInstance)
    {
        return;
    }

    if (URemasterFeedbackSubsystem* Feedback =
            GameInstance->GetSubsystem<URemasterFeedbackSubsystem>())
    {
        Feedback->Emit(
            FName(*FString::Printf(TEXT("CoreEvent.%u"), EventId)),
            Arg0,
            Arg1);
    }
}
