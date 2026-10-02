#include "remaster/core.h"

#include <stdint.h>
#include <stdio.h>

typedef struct ReplayStep {
    RemasterInput input;
    uint32_t expected_events;
    uint64_t expected_hash;
} ReplayStep;

static const ReplayStep kReplay[] = {
    { REMASTER_INPUT_MOVE_UP,
      REMASTER_EVENT_BLOCKED,
      UINT64_C(0xc12fd033cedac739) },

    { REMASTER_INPUT_MOVE_RIGHT,
      REMASTER_EVENT_MOVED,
      UINT64_C(0x29f69072eb5263f3) },

    { REMASTER_INPUT_MOVE_RIGHT,
      REMASTER_EVENT_MOVED,
      UINT64_C(0xce1a6093bb2ada79) },

    { REMASTER_INPUT_INTERACT,
      REMASTER_EVENT_INTERACTED | REMASTER_EVENT_FLAG_SET,
      UINT64_C(0x475eee60a3df4ec9) },

    { REMASTER_INPUT_MOVE_DOWN,
      REMASTER_EVENT_MOVED,
      UINT64_C(0xe66222ea2b7886f3) },

    { REMASTER_INPUT_MOVE_DOWN,
      REMASTER_EVENT_MOVED,
      UINT64_C(0x04f02b4616646535) },

    { REMASTER_INPUT_MOVE_DOWN,
      REMASTER_EVENT_MOVED | REMASTER_EVENT_ENCOUNTER,
      UINT64_C(0x642df4e66c5448a0) }
};

int main(void)
{
    RemasterState state;
    size_t i;

    remaster_core_init(&state);

    for (i = 0; i < sizeof(kReplay) / sizeof(kReplay[0]); ++i) {
        uint32_t events = remaster_core_step(
            &state,
            kReplay[i].input);

        uint64_t hash = remaster_core_state_hash(
            &state);

        if (events != kReplay[i].expected_events) {
            fprintf(
                stderr,
                "Replay event mismatch at step %zu: got 0x%X expected 0x%X\n",
                i,
                events,
                kReplay[i].expected_events);
            return 1;
        }

        if (hash != kReplay[i].expected_hash) {
            fprintf(
                stderr,
                "Replay hash mismatch at step %zu: got %llu expected %llu\n",
                i,
                (unsigned long long)hash,
                (unsigned long long)kReplay[i].expected_hash);
            return 1;
        }
    }

    puts("Deterministic replay regression passed.");
    return 0;
}
