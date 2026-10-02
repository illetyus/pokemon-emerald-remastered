#include <cassert>
#include <cstdint>
#include <cstdio>

extern "C"
{
#include "../core/src/core.c"
}

int main()
{
    RemasterState state{};
    remaster_core_init(&state);

    std::uint32_t events = remaster_core_step(&state, REMASTER_INPUT_MOVE_UP);
    assert(events == REMASTER_EVENT_BLOCKED);

    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);

    events = remaster_core_step(&state, REMASTER_INPUT_INTERACT);
    assert(events == (REMASTER_EVENT_INTERACTED | REMASTER_EVENT_FLAG_SET));

    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);

    events = remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    assert(events == (REMASTER_EVENT_MOVED | REMASTER_EVENT_ENCOUNTER));

    assert(state.tile_x == 3);
    assert(state.tile_y == 4);
    assert(state.step_count == 5);
    assert(state.event_flags == 1u);
    assert(state.encounter_pending == 1u);

    const std::uint64_t hash = remaster_core_state_hash(&state);
    assert(hash == UINT64_C(7218695048241891488));

    std::printf(
        "Unreal C++ embed preflight passed. state_hash=%llu\n",
        static_cast<unsigned long long>(hash));

    return 0;
}
