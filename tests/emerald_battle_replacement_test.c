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
    const RemasterEmeraldSpeciesInfo *info =
        remaster_emerald_species_info(species);
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[6] = {31, 31, 31, 31, 31, 31};
    uint8_t evs[6] = {0, 0, 0, 0, 0, 0};
    size_t i;

    if (info == 0)
        return 0;

    memset(mon, 0, sizeof(*mon));
    mon->box.personality = (uint32_t)species * 131u + level;
    mon->box.ot_id = 0x12345678u;

    if (!remaster_emerald_box_pokemon_set_species(&mon->box, species)
        || !remaster_emerald_box_pokemon_set_experience(
            &mon->box,
            remaster_emerald_experience_for_level(
                info->growth_rate, level)))
        return 0;

    for (i = 0; i < 6; ++i) {
        if (!remaster_emerald_box_pokemon_set_iv(
                &mon->box, i, ivs[i]))
            return 0;
    }

    for (i = 0; i < 4; ++i) {
        const RemasterEmeraldMoveInfo *move =
            remaster_emerald_move_info(moves[i]);
        if (moves[i] != 0
            && (move == 0
                || !remaster_emerald_box_pokemon_set_move(
                    &mon->box, i, moves[i], move->pp)))
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
    mon->hp = stats.hp;
    mon->max_hp = stats.hp;
    mon->attack = stats.attack;
    mon->defense = stats.defense;
    mon->speed = stats.speed;
    mon->sp_attack = stats.sp_attack;
    mon->sp_defense = stats.sp_defense;
    mon->box.checksum =
        remaster_emerald_box_pokemon_checksum(&mon->box);
    return 1;
}

static int test_trainer_replacement(void)
{
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foes[2];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint32_t turn_after_faint;

    if (!make_mon(&player, 1, 60, tackle)
        || !make_mon(&foes[0], 4, 5, tackle)
        || !make_mon(&foes[1], 7, 10, tackle))
        return check(0, "trainer replacement fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        0x1234u);
    if (!remaster_emerald_battle_start(
            &battle, &player, 1, foes, 2))
        return check(0, "trainer replacement battle should start");

    battle.battlers[1].pokemon.hp = 1;
    battle.parties[1][0].hp = 1;

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "KO turn should resolve");

    if (!check(
            battle.battlers[1].fainted
                && remaster_emerald_battle_needs_replacement(
                    &battle, 1),
            "trainer faint should enter replacement-pending state"))
        return 0;

    turn_after_faint = battle.turn_number;
    memset(actions, 0, sizeof(actions));
    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "trainer replacement phase should resolve");

    return check(
            !battle.battlers[1].fainted
                && battle.battlers[1].party_slot == 1
                && !remaster_emerald_battle_needs_replacement(
                    &battle, 1)
                && battle.turn_number == turn_after_faint,
            "trainer AI replacement must not consume a battle turn");
}

static int test_player_replacement(void)
{
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    static const uint16_t quick_attack[4] = {98, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon players[2];
    RemasterEmeraldPartyPokemon foe;
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint32_t turn_after_faint;

    if (!make_mon(&players[0], 1, 5, tackle)
        || !make_mon(&players[1], 7, 10, tackle)
        || !make_mon(&foe, 4, 60, quick_attack))
        return check(0, "player replacement fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        0x5678u);
    if (!remaster_emerald_battle_start(
            &battle, players, 2, &foe, 1))
        return check(0, "player replacement battle should start");

    battle.battlers[0].pokemon.hp = 1;
    battle.parties[0][0].hp = 1;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "player KO turn should resolve");

    if (!check(
            battle.battlers[0].fainted
                && remaster_emerald_battle_needs_replacement(
                    &battle, 0),
            "player faint should wait for replacement input"))
        return 0;

    turn_after_faint = battle.turn_number;
    memset(actions, 0, sizeof(actions));
    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_SWITCH;
    actions[0].party_slot = 1;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "player replacement action should resolve");

    return check(
            !battle.battlers[0].fainted
                && battle.battlers[0].party_slot == 1
                && !remaster_emerald_battle_needs_replacement(
                    &battle, 0)
                && battle.turn_number == turn_after_faint,
            "player-selected replacement must not consume a battle turn");
}

int main(void)
{
    if (!test_trainer_replacement())
        return 1;
    if (!test_player_replacement())
        return 1;

    puts("r13 faint replacement test passed");
    return 0;
}
