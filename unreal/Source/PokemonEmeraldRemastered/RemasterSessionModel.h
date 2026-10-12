#pragma once
extern "C" {
#include "remaster/emerald_save.h"
}
#include <cstdint>
#include <limits>

// Host lifetime only. Persistent gameplay remains in RemasterEmeraldSave.
namespace RemasterSession {
enum class State { Idle, Loading, Playing, Error };
inline bool CanContinue(RemasterEmeraldSaveStatus status) {
    return status == REMASTER_EMERALD_SAVE_OK || status == REMASTER_EMERALD_SAVE_DEGRADED;
}
inline bool CanNewGame(RemasterEmeraldSaveStatus status) {
    return status == REMASTER_EMERALD_SAVE_EMPTY;
}
struct Lifecycle {
    State state = State::Idle;
    std::uint64_t generation = 0;
    RemasterEmeraldSaveStatus lastSaveStatus = REMASTER_EMERALD_SAVE_EMPTY;
    bool mapLoaded = false;
    bool Playing() const { return state == State::Playing; }
    bool Begin() {
        if (state == State::Loading) return false;
        if (generation == std::numeric_limits<std::uint64_t>::max()) {
            state = State::Error; mapLoaded = false; return false;
        }
        ++generation; state = State::Loading; mapLoaded = false; return true;
    }
    bool Finish(std::uint64_t token, RemasterEmeraldSaveStatus status, bool ready) {
        if (state != State::Loading || token != generation) return false;
        lastSaveStatus = status; mapLoaded = ready && CanContinue(status);
        state = mapLoaded ? State::Playing : State::Error;
        return true;
    }
    void Cancel() {
        mapLoaded = false;
        if (generation == std::numeric_limits<std::uint64_t>::max()) {
            state = State::Error; return;
        }
        ++generation; state = State::Idle;
    }
};
}