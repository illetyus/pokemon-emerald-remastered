#pragma once

#include "CoreMinimal.h"

extern "C"
{
#include "remaster/core.h"
}

enum class ERemasterAction : uint8
{
    None,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Interact
};

struct FRemasterSnapshot
{
    int32 TileX = 0;
    int32 TileY = 0;
    uint32 StepCount = 0;
    uint32 InteractionCount = 0;
    uint32 EventFlags = 0;
    bool bEncounterPending = false;
};

class FRemasterCoreAdapter
{
public:
    FRemasterCoreAdapter();

    void Reset();
    uint32 Step(ERemasterAction Action);
    FRemasterSnapshot Snapshot() const;

    TArray<uint8> SaveState() const;
    bool LoadState(const TArray<uint8>& Bytes);

    uint64 StateHash() const;

private:
    static RemasterInput ToCoreInput(ERemasterAction Action);

    RemasterState State{};
};
