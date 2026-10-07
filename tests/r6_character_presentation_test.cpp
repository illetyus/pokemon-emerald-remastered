#include "RemasterCharacterPresentationRead.h"
#include "remaster/emerald_state.h"
#include <cstdio>
#include <cstring>

#define REQUIRE(condition) do { if (!(condition)) { std::fprintf(stderr, "R6 failed line %d: %s\n", __LINE__, #condition); return 1; } } while (0)

int main()
{
    using namespace RemasterCharacterPresentation;
    RemasterEmeraldSave Save{};
    RemasterEmeraldObjectRuntime Runtime{};
    RemasterEmeraldObjectEventDef Events[2]{};
    Events[0].local_id = 1; Events[0].flag_id = 0x123;
    Events[1].local_id = 2;
    Runtime.count = 2;
    Runtime.objects[0] = {1, 1, 10, 20, 9, 20, 4, 4, 3, 0};
    Runtime.objects[1] = {1, 2, 30, 40, 30, 39, 5, 5, 4, 0};
    const auto OriginalSave = Save;
    const auto OriginalRuntime = Runtime;
    Snapshot Out[16]{};
    size_t Count = 0;
    REQUIRE(ReadSnapshots(&Runtime, &Save, Events, 2, Out, 16, Count));
    REQUIRE(Count == 2 && Out[0].LocalId != Out[1].LocalId);
    REQUIRE(Out[0].Visible && Out[1].Visible);
    REQUIRE(Out[0].X == 10 && Out[0].Y == 20 && Out[0].Elevation == 3);
    REQUIRE(std::memcmp(&Save, &OriginalSave, sizeof Save) == 0);
    REQUIRE(std::memcmp(&Runtime, &OriginalRuntime, sizeof Runtime) == 0);
    REQUIRE(remaster_emerald_flag_set(&Save, 0x123, 1));
    REQUIRE(ReadSnapshots(&Runtime, &Save, Events, 2, Out, 16, Count));
    REQUIRE(!Out[0].Visible && Out[1].Visible);
    REQUIRE(remaster_emerald_object_runtime_set_active(&Runtime, 2, 0));
    REQUIRE(ReadSnapshots(&Runtime, &Save, Events, 2, Out, 16, Count));
    REQUIRE(!Out[1].Visible);
    REQUIRE(remaster_emerald_object_runtime_set_position(&Runtime, 1, 77, 88, 6));
    REQUIRE(ReadSnapshots(&Runtime, &Save, Events, 2, Out, 16, Count));
    REQUIRE(Out[0].X == 77 && Out[0].Y == 88 && Out[0].Elevation == 6);
    REQUIRE(!ReadSnapshots(&Runtime, &Save, Events, 2, Out, 1, Count) && Count == 0);
    Events[1].local_id = 1;
    REQUIRE(!ReadSnapshots(&Runtime, &Save, Events, 2, Out, 16, Count));
    Events[1].local_id = 2;
    Runtime.count = 17;
    REQUIRE(!ReadSnapshots(&Runtime, &Save, Events, 2, Out, 16, Count));
    LoadEpoch Epoch;
    const auto Old = Epoch.Value;
    Epoch.Cancel();
    REQUIRE(!Epoch.Accept(Old));
    REQUIRE(Epoch.Accept(Epoch.Value));
    Epoch.Cancel(); // map switch / destruction / identity change cancels old callbacks
    REQUIRE(!Epoch.Accept(Old));
    return 0;
}
