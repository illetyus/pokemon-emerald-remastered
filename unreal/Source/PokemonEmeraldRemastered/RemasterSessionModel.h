#pragma once
extern "C" {
#include "remaster/emerald_save.h"
}
#include <cstdint>
#include <string_view>
#include <limits>

// Host lifetime only. Persistent gameplay remains in RemasterEmeraldSave.
namespace RemasterSession {
inline bool IsTruckMap(std::string_view id) { return id=="MAP_INSIDE_OF_TRUCK"; }
enum class State { Idle, Loading, Playing, Error, Intro };
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
        if (state == State::Loading || state == State::Intro) return false;
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
    bool FinishIntro(std::uint64_t token,RemasterEmeraldSaveStatus status,bool ready) {
        if(state!=State::Loading || token!=generation)return false;
        lastSaveStatus=status;mapLoaded=ready&&CanContinue(status);
        state=mapLoaded?State::Intro:State::Error;return mapLoaded;
    }
    bool CompleteIntro(std::uint64_t token,bool ready) {
        if(state!=State::Intro || token!=generation || !mapLoaded || !CanContinue(lastSaveStatus))return false;
        state=ready?State::Playing:State::Error;mapLoaded=ready;return ready;
    }
    void Fail(std::uint64_t token) {
        if(token!=generation)return;
        state=State::Error;mapLoaded=false;
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