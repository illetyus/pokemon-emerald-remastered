#include <cstdint>
#include <cstdio>

extern "C"
{
#include "remaster/core.h"
#include "remaster/emerald_object_state.h"
#include "remaster/emerald_quest.h"
#include "remaster/emerald_encounter.h"
#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_items.h"
#include "remaster/emerald_state.h"
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

    RemasterEmeraldSave save{};
    RemasterEmeraldObjectTemplate object_template{};
    RemasterEmeraldObjectTemplate observed_template{};

    object_template.local_id = 7;
    object_template.graphics_id = 42;
    object_template.x = 12;
    object_template.y = 9;
    object_template.elevation = 3;
    object_template.movement_type = 5;
    object_template.flag_id = 0x123;

    if (!remaster_emerald_object_template_set(&save, 0, &object_template))
        return fail("object-template write failed");
    if (!remaster_emerald_object_template_get(&save, 0, &observed_template))
        return fail("object-template read failed");
    if (observed_template.local_id != 7
        || observed_template.graphics_id != 42
        || observed_template.x != 12
        || observed_template.y != 9
        || observed_template.movement_type != 5
        || observed_template.flag_id != 0x123)
        return fail("object-template state mismatch");

    if (!remaster_emerald_flag_set(&save, 0x0860, 1))
        return fail("R10 starter flag write failed");

    const RemasterEmeraldQuestObjective* quest =
        remaster_emerald_quest_active(&save);
    if (quest == nullptr
        || quest->id != REMASTER_EMERALD_QUEST_MEET_RIVAL_ROUTE103)
        return fail("R10 quest resolver embed mismatch");

    const RemasterEmeraldSpeciesInfo* treecko =
        remaster_emerald_species_info(277);
    if (treecko == nullptr
        || treecko->base_hp != 40
        || treecko->base_speed != 70)
        return fail("R11 species catalog embed mismatch");

    if (!remaster_emerald_bag_add(&save, 13, 1)
        || remaster_emerald_bag_count(&save, 13) != 1)
        return fail("R11 bag core embed mismatch");

    RemasterEmeraldEncounterRng encounter_rng{};
    remaster_emerald_encounter_rng_seed(
        &encounter_rng,
        UINT32_C(0x1234));
    if (remaster_emerald_encounter_random(&encounter_rng) != 0x4DCB
        || remaster_emerald_encounter_map_count() != 124)
        return fail("R12 encounter C++ embed mismatch");

    const RemasterEmeraldEncounterMapInfo* route101 =
        remaster_emerald_encounter_map_find(0, 16);
    if (route101 == nullptr
        || route101->land.encounter_rate != 20
        || route101->land.anchor_count != 12)
        return fail("R12 Route101 catalog embed mismatch");

    const std::uint64_t hash = remaster_core_state_hash(&state);
    if (hash != UINT64_C(7218695048241891488))
        return fail("canonical state hash mismatch");

    std::printf(
        "Unreal C++ embed preflight passed. state_hash=%llu\n",
        static_cast<unsigned long long>(hash));

    return 0;
}
