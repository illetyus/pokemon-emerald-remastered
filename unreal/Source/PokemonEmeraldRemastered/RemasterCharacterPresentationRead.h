#pragma once

extern "C"
{
#include "remaster/emerald_object_state.h"
}

namespace RemasterCharacterPresentation
{
struct Snapshot
{
    uint16_t LocalId = 0;
    int32_t X = 0;
    int32_t Y = 0;
    uint8_t Elevation = 0;
    bool Visible = false;
};

// No mutation, simulation, coordinate inference or dependency on model assets.
inline bool ReadSnapshots(
    const RemasterEmeraldObjectRuntime* Runtime,
    const RemasterEmeraldSave* Save,
    const RemasterEmeraldObjectEventDef* Events,
    size_t EventCount,
    Snapshot* Out,
    size_t Capacity,
    size_t& OutCount)
{
    OutCount = 0;
    if (!Runtime || !Save || (!Events && EventCount) || !Out
        || Runtime->count > REMASTER_EMERALD_RUNTIME_OBJECT_COUNT
        || Capacity < Runtime->count)
        return false;
    for (size_t I = 0; I < EventCount; ++I)
        for (size_t J = 0; J < I; ++J)
            if (Events[I].local_id == Events[J].local_id)
                return false;
    for (size_t I = 0; I < Runtime->count; ++I)
    {
        RemasterEmeraldRuntimeObject Object{};
        if (!remaster_emerald_object_runtime_get(Runtime, I, &Object))
            return false;
        const RemasterEmeraldObjectEventDef* Event = nullptr;
        for (size_t J = 0; J < EventCount; ++J)
            if (Events[J].local_id == Object.local_id)
            {
                Event = &Events[J];
                break;
            }
        if (!Event)
            continue;
        Out[OutCount++] = {Object.local_id,
            Object.current_x, Object.current_y, Object.elevation,
            Object.active != 0 && remaster_emerald_object_event_visible(Save, Event) != 0};
    }
    return true;
}

struct LoadEpoch
{
    uint64_t Value = 0;
    void Cancel() { ++Value; }
    bool Accept(uint64_t Captured) const { return Captured == Value; }
};
}
