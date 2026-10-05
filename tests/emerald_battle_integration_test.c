#include "remaster/emerald_battle.h"
#include "remaster/emerald_encounter.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

static int make_player(RemasterEmeraldPartyPokemon *mon)
{
    const uint16_t moves[4] = {52, 86, 240, 1};
    RemasterEmeraldCalculatedStats stats;
    const RemasterEmeraldSpeciesInfo *species;
    uint8_t ivs[6] = {31, 31, 31, 31, 31, 31};
    uint8_t evs[6] = {0, 0, 0, 0, 0, 0};
    size_t i;

    memset(mon, 0, sizeof(*mon));
    mon->box.personality = 3;
    mon->box.ot_id = 0x10203040u;
    species = remaster_emerald_species_info(4);
    if (species == 0)
        return 0;

    if (!remaster_emerald_box_pokemon_set_species(&mon->box, 4)
        || !remaster_emerald_box_pokemon_set_experience(
            &mon->box,
            remaster_emerald_experience_for_level(
                species->growth_rate,
                20)))
        return 0;

    for (i = 0; i < 6; ++i) {
        if (!remaster_emerald_box_pokemon_set_iv(&mon->box, i, ivs[i]))
            return 0;
    }

    for (i = 0; i < 4; ++i) {
        const RemasterEmeraldMoveInfo *move =
            remaster_emerald_move_info(moves[i]);
        if (move == 0
            || !remaster_emerald_box_pokemon_set_move(
                &mon->box, i, moves[i], move->pp))
            return 0;
    }

    if (!remaster_emerald_calculate_stats(
            4,
            20,
            remaster_emerald_box_pokemon_nature(&mon->box),
            ivs,
            evs,
            &stats))
        return 0;

    mon->level = 20;
    mon->max_hp = stats.hp;
    mon->hp = stats.hp;
    mon->attack = stats.attack;
    mon->defense = stats.defense;
    mon->speed = stats.speed;
    mon->sp_attack = stats.sp_attack;
    mon->sp_defense = stats.sp_defense;
    mon->box.checksum = remaster_emerald_box_pokemon_checksum(&mon->box);
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon wild;
    RemasterEmeraldPartyPokemon stored;
    RemasterEmeraldEncounterRuntime encounter;
    RemasterEmeraldBattleState battle;
    uint8_t count;

    memset(&save, 0, sizeof(save));

    if (!make_player(&player))
        return 1;

    if (!check(remaster_emerald_party_set_count(&save, 1), "set party count")
        || !check(remaster_emerald_party_set(&save, 0, &player), "store player"))
        return 1;

    remaster_emerald_encounter_runtime_init(&encounter, 0xCAFEBABEu);
    if (!check(
            remaster_emerald_encounter_create_wild(
                &encounter,
                &save,
                129,
                5,
                1,
                &wild),
            "R12 should create a wild Pokemon"))
        return 1;

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        0x2468ACE0u);

    if (!check(
            remaster_emerald_battle_start_from_save(
                &battle,
                &save,
                &wild,
                1),
            "R13 should start from R12 wild Pokemon"))
        return 1;

    if (!check(
            remaster_emerald_battle_use_move(&battle, 0, 1, 1),
            "Thunder Wave should execute"))
        return 1;
    if (!check(
            (battle.battlers[1].pokemon.status
                & REMASTER_EMERALD_STATUS1_PARALYSIS) != 0,
            "status should be owned by battle core"))
        return 1;

    if (!check(
            remaster_emerald_battle_use_move(&battle, 0, 1, 2),
            "Rain Dance should execute"))
        return 1;
    if (!check(
            battle.weather == REMASTER_EMERALD_BATTLE_WEATHER_RAIN,
            "weather should be owned by battle core"))
        return 1;

    if (!check(
            remaster_emerald_battle_throw_ball(&battle, 0, 1, 1),
            "Master Ball must capture"))
        return 1;
    if (!check(
            battle.outcome == REMASTER_EMERALD_BATTLE_OUTCOME_CAUGHT
                && battle.caught_valid,
            "capture outcome should be authoritative"))
        return 1;

    if (!check(
            remaster_emerald_battle_store_caught(
                &battle,
                &save,
                0,
                0),
            "caught Pokemon should return to R11 save domain"))
        return 1;

    count = remaster_emerald_party_count(&save);
    if (!check(count == 2, "caught Pokemon should fill open party slot"))
        return 1;
    if (!check(
            remaster_emerald_party_get(&save, 1, &stored, 0),
            "read stored caught Pokemon"))
        return 1;
    if (!check(
            remaster_emerald_box_pokemon_species(&stored.box) == 129,
            "stored species must match R12 encounter"))
        return 1;

    puts("r13 encounter-battle integration test passed");
    return 0;
}
