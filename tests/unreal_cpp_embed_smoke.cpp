#include <cstdint>
#include <cstdio>

extern "C"
{
#include "../core/src/platform.c"
#include "../core/src/mechanics.c"
#include "../core/src/core.c"
}

static int fail(const char* message)
{
    std::fprintf(stderr, "Unreal C++ embed preflight failed: %s\n", message);
    return 1;
}

int main()
{
    RemasterState state{};
    remaster_core_init(&state);

    std::uint32_t events = remaster_core_step(&state, REMASTER_INPUT_MOVE_UP);
    if (events != REMASTER_EVENT_BLOCKED)
        return fail("blocked-event mismatch");

    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);

    events = remaster_core_step(&state, REMASTER_INPUT_INTERACT);
    if (events != (REMASTER_EVENT_INTERACTED | REMASTER_EVENT_FLAG_SET))
        return fail("interaction-event mismatch");

    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);

    events = remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    if (events != (REMASTER_EVENT_MOVED | REMASTER_EVENT_ENCOUNTER))
        return fail("encounter-event mismatch");

    if (state.tile_x != 3 || state.tile_y != 4)
        return fail("final tile mismatch");
    if (state.step_count != 5)
        return fail("step-count mismatch");
    if (state.interaction_count != 1)
        return fail("interaction-count mismatch");
    if (state.event_flags != 1u)
        return fail("event-flags mismatch");
    if (state.encounter_pending != 1u)
        return fail("encounter state mismatch");

    const std::uint64_t hash = remaster_core_state_hash(&state);
    if (hash != UINT64_C(7218695048241891488))
        return fail("canonical state hash mismatch");

    std::printf(
        "Unreal C++ embed preflight passed. state_hash=%llu\n",
        static_cast<unsigned long long>(hash));

    return 0;
}
