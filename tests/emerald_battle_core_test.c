#include "remaster/emerald_battle.h"

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

static int make_mon(
    RemasterEmeraldPartyPokemon *mon,
    uint16_t species,
    uint8_t level,
    const uint16_t moves[4])
{
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[6] = {31, 31, 31, 31, 31, 31};
    uint8_t evs[6] = {0, 0, 0, 0, 0, 0};
    size_t i;

    memset(mon, 0, sizeof(*mon));
    mon->box.personality = (uint32_t)species * 25u + 3u;
    mon->box.ot_id = 0x12345678u;

    if (!remaster_emerald_box_pokemon_set_species(&mon->box, species))
        return 0;
    if (!remaster_emerald_box_pokemon_set_experience(
            &mon->box,
            remaster_emerald_experience_for_level(
                remaster_emerald_species_info(species)->growth_rate,
                level)))
        return 0;
    for (i = 0; i < 6; ++i) {
        if (!remaster_emerald_box_pokemon_set_iv(&mon->box, i, ivs[i]))
            return 0;
    }
    for (i = 0; i < 4; ++i) {
        const RemasterEmeraldMoveInfo *move = remaster_emerald_move_info(moves[i]);
        if (!remaster_emerald_box_pokemon_set_move(
                &mon->box,
                i,
                moves[i],
                move != 0 ? move->pp : 0))
            return 0;
    }

    if (!remaster_emerald_calculate_stats(
            species,
            level,
            remaster_emerald_box_pokemon_nature(&mon->box),
            ivs,
            evs,
            &stats))
        return 0;

    mon->level = level;
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
    RemasterEmeraldPartyPokemon player[1];
    RemasterEmeraldPartyPokemon foe[1];
    RemasterEmeraldBattleState a;
    RemasterEmeraldBattleState b;
    RemasterEmeraldBattleAction actions[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    const uint16_t player_moves[4] = {52, 86, 240, 1};
    const uint16_t foe_moves[4] = {1, 45, 0, 0};
    uint16_t hp_before;

    if (!check(
            remaster_emerald_battle_type_effectiveness(10, 12) == 20,
            "Fire must be super-effective against Grass"))
        return 1;
    if (!check(
            remaster_emerald_battle_type_effectiveness(13, 4) == 0,
            "Electric must not affect Ground"))
        return 1;
    if (!check(
            remaster_emerald_battle_type_effectiveness(0, 7) == 0,
            "Normal must not affect Ghost"))
        return 1;

    if (!make_mon(player, 4, 20, player_moves)
        || !make_mon(foe, 1, 20, foe_moves)) {
        fprintf(stderr, "FAIL: fixture construction\n");
        return 1;
    }

    remaster_emerald_battle_state_init(
        &a,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        0x13579BDFu);
    remaster_emerald_battle_state_init(
        &b,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        0x13579BDFu);

    if (!check(
            remaster_emerald_battle_start(&a, player, 1, foe, 1),
            "battle A should start")
        || !check(
            remaster_emerald_battle_start(&b, player, 1, foe, 1),
            "battle B should start"))
        return 1;

    if (!check(
            a.event_count != 0
                && a.events[0].kind == REMASTER_EMERALD_BATTLE_EVENT_STARTED,
            "battle must emit started event"))
        return 1;

    memset(actions, 0, sizeof(actions));
    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    if (!remaster_emerald_battle_choose_ai_action(&a, 1, &actions[1]))
        return 1;

    hp_before = a.battlers[1].pokemon.hp;
    if (!check(
            remaster_emerald_battle_resolve_turn(&a, actions),
            "turn A should resolve"))
        return 1;
    if (!check(
            a.battlers[1].pokemon.hp < hp_before,
            "damaging move must lower HP"))
        return 1;
    if (!check(
            a.battlers[0].pp[0]
                == (uint8_t)(remaster_emerald_move_info(52)->pp - 1u),
            "move use must consume PP"))
        return 1;

    memset(actions, 0, sizeof(actions));
    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    if (!remaster_emerald_battle_choose_ai_action(&b, 1, &actions[1]))
        return 1;
    if (!check(
            remaster_emerald_battle_resolve_turn(&b, actions),
            "turn B should resolve"))
        return 1;
    if (!check(
            a.battlers[0].pokemon.hp == b.battlers[0].pokemon.hp
                && a.battlers[1].pokemon.hp == b.battlers[1].pokemon.hp
                && a.rng.state == b.rng.state
                && a.rng.calls == b.rng.calls,
            "fixed-seed replay must be deterministic"))
        return 1;

    puts("r13 battle core test passed");
    return 0;
}
