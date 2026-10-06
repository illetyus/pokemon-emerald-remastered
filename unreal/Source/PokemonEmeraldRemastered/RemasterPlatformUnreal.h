#pragma once

#include "CoreMinimal.h"

extern "C"
{
#include "remaster/platform.h"
}

class UGameInstance;

class FRemasterPlatformUnreal
{
public:
    static void Install(UGameInstance* GameInstance);
    static void Uninstall();

private:
    static uint64 MonotonicTimeNs(void* Userdata);
    static int ReadWallClock(
        void* Userdata,
        RemasterWallClock* OutClock);

    static int SaveRead(
        void* Userdata,
        const char* Slot,
        uint8* Buffer,
        size_t Capacity,
        size_t* OutSize);

    static RemasterSaveReadResult SaveReadResult(
        void* Userdata,
        const char* Slot,
        uint8* Buffer,
        size_t Capacity,
        size_t* OutSize);

    static int SaveWrite(
        void* Userdata,
        const char* Slot,
        const uint8* Buffer,
        size_t Size);

    static void Log(
        void* Userdata,
        RemasterLogLevel Level,
        const char* Message);

    static void EmitPresentationEvent(
        void* Userdata,
        uint32 EventId,
        int64 Arg0,
        int64 Arg1);

    static FString SlotPath(const char* Slot);
};
