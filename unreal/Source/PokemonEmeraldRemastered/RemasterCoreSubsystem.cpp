#include "RemasterCoreSubsystem.h"
#include "RemasterPlatformUnreal.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
constexpr uint8 SaveMagic[4] = { 'R', '0', 'U', '1' };
constexpr int32 HashBytes = 8;

void AppendUInt64LE(TArray<uint8>& Out, uint64 Value)
{
    for (int32 Index = 0; Index < HashBytes; ++Index)
    {
        Out.Add(static_cast<uint8>((Value >> (Index * 8)) & 0xffu));
    }
}

bool ReadUInt64LE(const TArray<uint8>& Bytes, int32 Offset, uint64& OutValue)
{
    if (Offset < 0 || Offset + HashBytes > Bytes.Num())
    {
        return false;
    }

    OutValue = 0;
    for (int32 Index = 0; Index < HashBytes; ++Index)
    {
        OutValue |= static_cast<uint64>(Bytes[Offset + Index]) << (Index * 8);
    }
    return true;
}
}

void URemasterCoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FRemasterPlatformUnreal::Install();
    Core.Reset();
    LoadPersistentState();

    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(
        this,
        &URemasterCoreSubsystem::HandleWillEnterBackground);

    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(
        this,
        &URemasterCoreSubsystem::HandleEnteredForeground);

    UE_LOG(
        LogTemp,
        Display,
        TEXT("R0 Unreal core initialized hash=%llu"),
        static_cast<unsigned long long>(Core.StateHash()));
}

void URemasterCoreSubsystem::Deinitialize()
{
    SavePersistentState();

    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);
    FRemasterPlatformUnreal::Uninstall();

    Super::Deinitialize();
}

void URemasterCoreSubsystem::ResetCore()
{
    Core.Reset();
}

uint32 URemasterCoreSubsystem::Step(ERemasterAction Action)
{
    const uint32 Events = Core.Step(Action);

    UE_LOG(
        LogTemp,
        Verbose,
        TEXT("R0 Unreal step action=%d events=0x%X hash=%llu"),
        static_cast<int32>(Action),
        Events,
        static_cast<unsigned long long>(Core.StateHash()));

    return Events;
}

FRemasterSnapshot URemasterCoreSubsystem::Snapshot() const
{
    return Core.Snapshot();
}

uint64 URemasterCoreSubsystem::StateHash() const
{
    return Core.StateHash();
}

FString URemasterCoreSubsystem::SavePath() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("r0_unreal_state.bin"));
}

bool URemasterCoreSubsystem::SavePersistentState()
{
    const TArray<uint8> Payload = Core.SaveState();
    if (Payload.IsEmpty())
    {
        return false;
    }

    TArray<uint8> FileBytes;
    FileBytes.Reserve(UE_ARRAY_COUNT(SaveMagic) + Payload.Num() + HashBytes);
    FileBytes.Append(SaveMagic, UE_ARRAY_COUNT(SaveMagic));
    FileBytes.Append(Payload);
    AppendUInt64LE(FileBytes, Core.StateHash());

    const FString Path = SavePath();
    const bool bSaved = FFileHelper::SaveArrayToFile(FileBytes, *Path);

    UE_LOG(
        LogTemp,
        Display,
        TEXT("R0 Unreal persistent save %s hash=%llu path=%s"),
        bSaved ? TEXT("complete") : TEXT("failed"),
        static_cast<unsigned long long>(Core.StateHash()),
        *Path);

    return bSaved;
}

bool URemasterCoreSubsystem::LoadPersistentState()
{
    TArray<uint8> FileBytes;
    const FString Path = SavePath();

    if (!FFileHelper::LoadFileToArray(FileBytes, *Path))
    {
        return false;
    }

    const int32 PayloadSize = static_cast<int32>(remaster_core_state_size());
    const int32 ExpectedSize =
        UE_ARRAY_COUNT(SaveMagic) + PayloadSize + HashBytes;

    if (FileBytes.Num() != ExpectedSize ||
        FMemory::Memcmp(FileBytes.GetData(), SaveMagic, UE_ARRAY_COUNT(SaveMagic)) != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("R0 Unreal persistent save rejected: format"));
        return false;
    }

    TArray<uint8> Payload;
    Payload.Append(
        FileBytes.GetData() + UE_ARRAY_COUNT(SaveMagic),
        PayloadSize);

    uint64 ExpectedHash = 0;
    if (!ReadUInt64LE(
            FileBytes,
            UE_ARRAY_COUNT(SaveMagic) + PayloadSize,
            ExpectedHash))
    {
        return false;
    }

    Core.Reset();
    if (!Core.LoadState(Payload))
    {
        return false;
    }

    const uint64 ActualHash = Core.StateHash();
    if (ActualHash != ExpectedHash)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("R0 Unreal persistent save rejected: expected=%llu actual=%llu"),
            static_cast<unsigned long long>(ExpectedHash),
            static_cast<unsigned long long>(ActualHash));
        Core.Reset();
        return false;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("R0 Unreal persistent load complete hash=%llu"),
        static_cast<unsigned long long>(ActualHash));

    return true;
}

void URemasterCoreSubsystem::HandleWillEnterBackground()
{
    UE_LOG(LogTemp, Display, TEXT("R0 Unreal lifecycle: WILL_ENTER_BACKGROUND"));
    SavePersistentState();
}

void URemasterCoreSubsystem::HandleEnteredForeground()
{
    UE_LOG(
        LogTemp,
        Display,
        TEXT("R0 Unreal lifecycle: ENTERED_FOREGROUND hash=%llu"),
        static_cast<unsigned long long>(Core.StateHash()));
}
