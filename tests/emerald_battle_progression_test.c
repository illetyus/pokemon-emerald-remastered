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
    mon->box.personality = 0x10203040u + species;
    mon->box.ot_id = 0x55667788u;

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

static int test_win_defers_evolution_until_after_battle(void)
{
    static const uint16_t player_moves[4] = {98, 33, 0, 0};
    static const uint16_t foe_moves[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foe;
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint32_t level7_exp;
    int saw_level = 0;
    int saw_learn = 0;
    int saw_evolution_check = 0;
    size_t exp_event = SIZE_MAX;
    size_t level_event = SIZE_MAX;
    size_t learn_event = SIZE_MAX;
    size_t ended_event = SIZE_MAX;
    size_t evolution_event = SIZE_MAX;
    size_t i;

    if (!check(
            make_mon(&player, 1, 6, player_moves)
                && make_mon(&foe, 4, 1, foe_moves),
            "progression fixtures should build"))
        return 1;

    level7_exp = remaster_emerald_experience_for_level(
        remaster_emerald_species_info(1)->growth_rate,
        7);
    if (!check(
            level7_exp > 1
                && remaster_emerald_box_pokemon_set_experience(
                    &player.box, level7_exp - 1u),
            "player fixture should sit one EXP below level 7"))
        return 1;
    player.box.checksum =
        remaster_emerald_box_pokemon_checksum(&player.box);

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        0xCAFEBABEu);
    if (!check(
            remaster_emerald_battle_start(
                &battle, &player, 1, &foe, 1),
            "progression battle should start"))
        return 1;

    battle.battlers[1].pokemon.hp = 1;
    battle.parties[1][0].hp = 1;

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;

    if (!check(
            remaster_emerald_battle_resolve_turn(
                &battle, actions),
            "level-up KO turn should resolve"))
        return 1;

    if (!check(
            battle.battlers[0].pokemon.level == 7,
            "battle EXP should level Bulbasaur from 6 to 7"))
        return 1;

    for (i = 0; i < battle.event_count; ++i) {
        const RemasterEmeraldBattleEvent *event = &battle.events[i];

        if (event->kind == REMASTER_EMERALD_BATTLE_EVENT_EXP
            && exp_event == SIZE_MAX)
            exp_event = i;
        if (event->kind == REMASTER_EMERALD_BATTLE_EVENT_LEVEL_UP
            && level_event == SIZE_MAX)
            level_event = i;
        if (event->kind == REMASTER_EMERALD_BATTLE_EVENT_MOVE_LEARN
            && learn_event == SIZE_MAX)
            learn_event = i;
        if (event->kind == REMASTER_EMERALD_BATTLE_EVENT_ENDED
            && ended_event == SIZE_MAX)
            ended_event = i;
        if (event->kind == REMASTER_EMERALD_BATTLE_EVENT_EVOLUTION_CHECK
            && evolution_event == SIZE_MAX)
            evolution_event = i;

        if (event->kind == REMASTER_EMERALD_BATTLE_EVENT_LEVEL_UP
            && event->battler == 0
            && event->value == 7)
            saw_level = 1;

        if (event->kind == REMASTER_EMERALD_BATTLE_EVENT_MOVE_LEARN
            && event->battler == 0
            && event->move_id == 73
            && event->value == 7
            && event->aux == 1)
            saw_learn = 1;

        if (event->kind
                == REMASTER_EMERALD_BATTLE_EVENT_EVOLUTION_CHECK
            && event->battler == 0
            && event->value == 7
            && event->aux == 1)
            saw_evolution_check = 1;
    }

    if (!check(saw_level, "level-up event should be emitted")
        || !check(
            saw_learn,
            "Bulbasaur level 7 Leech Seed should be handed off")
        || !check(
            saw_evolution_check,
            "level-up should hand off a normal evolution check"))
        return 1;

    if (!check(
            exp_event < level_event
                && level_event < learn_event
                && learn_event < ended_event
                && ended_event < evolution_event,
            "Emerald progression order must defer evolution until "
            "after the battle has ended with a win"))
        return 1;

    for (i = 0; i < REMASTER_EMERALD_MAX_MOVES; ++i) {
        if (!check(
                battle.battlers[0].moves[i] != 73,
                "battle core must not auto-accept a move-learning choice"))
            return 1;
    }

    if (!check(
            battle.battlers[0].species == 1,
            "battle core must not auto-evolve during handoff"))
        return 1;

    return 1;
}

static int test_loss_does_not_emit_evolution_handoff(void)
{
    static const uint16_t player_moves[4] = {98, 33, 0, 0};
    static const uint16_t weak_moves[4] = {33, 0, 0, 0};
    static const uint16_t strong_moves[4] = {98, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foes[2];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint32_t level7_exp;
    size_t i;

    if (!make_mon(&player, 1, 6, player_moves)
        || !make_mon(&foes[0], 4, 1, weak_moves)
        || !make_mon(&foes[1], 7, 60, strong_moves))
        return check(0, "loss progression fixtures should build");

    level7_exp = remaster_emerald_experience_for_level(
        remaster_emerald_species_info(1)->growth_rate,
        7);
    if (!remaster_emerald_box_pokemon_set_experience(
            &player.box, level7_exp - 1u))
        return check(0, "loss progression EXP fixture should build");
    player.box.checksum =
        remaster_emerald_box_pokemon_checksum(&player.box);
    foes[0].hp = 1;
    foes[0].box.checksum =
        remaster_emerald_box_pokemon_checksum(&foes[0].box);

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        0x10E50001u);
    if (!remaster_emerald_battle_start(
            &battle, &player, 1, foes, 2))
        return check(0, "loss progression battle should start");

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "loss progression first KO should resolve");

    for (i = 0; i < battle.event_count; ++i) {
        if (battle.events[i].kind
            == REMASTER_EMERALD_BATTLE_EVENT_EVOLUTION_CHECK)
            return check(
                0,
                "evolution handoff must not happen before battle outcome");
    }

    memset(actions, 0, sizeof(actions));
    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "loss progression replacement should resolve");

    battle.battlers[0].pokemon.hp = 1;
    battle.parties[0][0].hp = 1;
    memset(actions, 0, sizeof(actions));
    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "loss progression losing turn should resolve");

    if (!check(
            battle.ended
                && battle.outcome
                    == REMASTER_EMERALD_BATTLE_OUTCOME_LOST,
            "loss progression battle should end in defeat"))
        return 0;

    for (i = 0; i < battle.event_count; ++i) {
        if (battle.events[i].kind
            == REMASTER_EMERALD_BATTLE_EVENT_EVOLUTION_CHECK)
            return check(
                0,
                "a battle loss must not emit post-battle evolution");
    }

    return 1;
}

int main(void)
{
    if (!test_win_defers_evolution_until_after_battle())
        return 1;
    if (!test_loss_does_not_emit_evolution_handoff())
        return 1;

    puts("r13 progression handoff test passed");
    return 0;
}
