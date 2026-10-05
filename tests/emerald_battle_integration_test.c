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
    if (!check(
            remaster_emerald_box_pokemon_pokeball(&stored.box) == 1,
            "capture Ball must survive R11 storage round-trip"))
        return 1;

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        0x11223344u);
    if (!check(
            remaster_emerald_battle_start_trainer_from_save(
                &battle,
                &save,
                1),
            "catalog-backed Sawyer trainer battle should start"))
        return 1;
    if (!check(
            (battle.battle_type_flags
                & REMASTER_EMERALD_BATTLE_TYPE_TRAINER) != 0
                && battle.opponent_trainer_id == 1
                && battle.opponent_trainer_ai_flags
                    == (REMASTER_EMERALD_AI_CHECK_BAD_MOVE
                        | REMASTER_EMERALD_AI_TRY_TO_FAINT
                        | REMASTER_EMERALD_AI_CHECK_VIABILITY)
                && battle.party_count[1] == 1
                && battle.battlers[1].species == 74,
            "trainer metadata and generated opponent party must reach battle state"))
        return 1;

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        0x55667788u);
    if (!check(
            remaster_emerald_battle_start_trainer_from_save(
                &battle,
                &save,
                51),
            "catalog-backed Gabby & Ty double battle should start"))
        return 1;
    if (!check(
            (battle.battle_type_flags
                & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE) != 0
                && battle.party_count[1] == 2
                && battle.battlers[2].active
                && battle.battlers[3].active,
            "trainer double-battle metadata must activate both flanks"))
        return 1;

    battle.ended = 1;
    battle.outcome = REMASTER_EMERALD_BATTLE_OUTCOME_WON;
    if (!check(
            remaster_emerald_battle_finalize_trainer(
                &battle,
                &save,
                &whiteout),
            "won trainer battle should finalize into save state"))
        return 1;
    if (!check(
            !whiteout
                && remaster_emerald_flag_get(&save, 0x500u + 51u, &trainer_flag)
                && trainer_flag,
            "won trainer battle must set its trainer defeat flag"))
        return 1;

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        0x10203040u);
    if (!check(
            remaster_emerald_battle_start_trainer_from_save(
                &battle,
                &save,
                11),
            "catalog-backed Marcel trainer battle should start"))
        return 1;
    battle.ended = 1;
    battle.outcome = REMASTER_EMERALD_BATTLE_OUTCOME_LOST;
    whiteout = 0;
    trainer_flag = 0;
    if (!check(
            remaster_emerald_battle_finalize_trainer(
                &battle,
                &save,
                &whiteout),
            "lost trainer battle should finalize into save state"))
        return 1;
    if (!check(
            whiteout
                && remaster_emerald_flag_get(&save, 0x500u + 11u, &trainer_flag)
                && !trainer_flag,
            "lost trainer battle must request whiteout without defeat flag"))
        return 1;

    puts("r13 encounter-battle integration test passed");
    return 0;
}
