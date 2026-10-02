#include "RemasterPlatformUnreal.h"

#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FString FRemasterPlatformUnreal::SlotPath(const char* Slot)
{
    const FString SafeSlot = UTF8_TO_TCHAR(Slot ? Slot : "default");
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Core"),
        SafeSlot + TEXT(".bin"));
}

void FRemasterPlatformUnreal::Install()
{
    RemasterPlatformVTable Platform{};
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
    if (!Buffer && Size > 0)
    {
        return 0;
    }

    const FString Path = SlotPath(Slot);
    IFileManager::Get().MakeDirectory(
        *FPaths::GetPath(Path),
        true);

    TArray<uint8> Bytes;
    Bytes.Append(Buffer, static_cast<int32>(Size));

    return FFileHelper::SaveArrayToFile(Bytes, *Path) ? 1 : 0;
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
    void*,
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
}
