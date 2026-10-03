#include "RemasterCoreAdapter.h"

FRemasterCoreAdapter::FRemasterCoreAdapter()
{
    Reset();
}

void FRemasterCoreAdapter::Reset()
{
    remaster_core_init(&State);
}

uint32 FRemasterCoreAdapter::Step(ERemasterAction Action)
{
    return remaster_core_step(&State, ToCoreInput(Action));
}

FRemasterSnapshot FRemasterCoreAdapter::Snapshot() const
{
    FRemasterSnapshot Out;
    Out.TileX = State.tile_x;
    Out.TileY = State.tile_y;
    Out.StepCount = State.step_count;
    Out.InteractionCount = State.interaction_count;
    Out.EventFlags = State.event_flags;
    Out.bEncounterPending = State.encounter_pending != 0;
    return Out;
}

TArray<uint8> FRemasterCoreAdapter::SaveState() const
{
    TArray<uint8> Bytes;
    Bytes.SetNumUninitialized(static_cast<int32>(remaster_core_state_size()));

    if (!remaster_core_save(
            &State,
            Bytes.GetData(),
            static_cast<size_t>(Bytes.Num())))
    {
        Bytes.Reset();
    }

    return Bytes;
}

bool FRemasterCoreAdapter::LoadState(const TArray<uint8>& Bytes)
{
    if (Bytes.Num() <= 0)
    {
        return false;
    }

    return remaster_core_load(
        &State,
        Bytes.GetData(),
        static_cast<size_t>(Bytes.Num())) != 0;
}

uint64 FRemasterCoreAdapter::StateHash() const
{
    return remaster_core_state_hash(&State);
}

RemasterInput FRemasterCoreAdapter::ToCoreInput(ERemasterAction Action)
{
    switch (Action)
    {
    case ERemasterAction::MoveUp:
        return REMASTER_INPUT_MOVE_UP;
    case ERemasterAction::MoveDown:
        return REMASTER_INPUT_MOVE_DOWN;
    case ERemasterAction::MoveLeft:
        return REMASTER_INPUT_MOVE_LEFT;
    case ERemasterAction::MoveRight:
        return REMASTER_INPUT_MOVE_RIGHT;
    case ERemasterAction::Interact:
        return REMASTER_INPUT_INTERACT;
    case ERemasterAction::None:
    default:
        return REMASTER_INPUT_NONE;
    }
}
