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
    mon->box.personality = (uint32_t)species * 31u + 11u;
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

static int start_trainer_fixture(
    RemasterEmeraldBattleState *battle,
    const uint16_t player_moves[4],
    const uint16_t foe_moves[4],
    uint32_t ai_flags,
    uint32_t seed)
{
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foe;

    if (!make_mon(&player, 1, 30, player_moves)
        || !make_mon(&foe, 4, 30, foe_moves))
        return 0;

    remaster_emerald_battle_state_init(
        battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        seed);
    if (!remaster_emerald_battle_start(
            battle, &player, 1, &foe, 1))
        return 0;

    battle->opponent_trainer_ai_flags = ai_flags;
    return 1;
}

int main(void)
{
    static const uint16_t player_moves[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldBattleAction action;

    {
        static const uint16_t foe_moves[4] = {33, 63, 0, 0};

        if (!check(
                start_trainer_fixture(
                    &battle,
                    player_moves,
                    foe_moves,
                    0,
                    1u),
                "zero-flag trainer AI fixture should start"))
            return 1;

        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "zero-flag trainer AI should choose a move"))
            return 1;

        if (!check(
                action.move_slot == 0,
                "Emerald zero-flag AI must tie at score 100 instead of "
                "preferring raw move power"))
            return 1;
    }

    {
        static const uint16_t foe_moves[4] = {63, 98, 0, 0};

        if (!check(
                start_trainer_fixture(
                    &battle,
                    player_moves,
                    foe_moves,
                    REMASTER_EMERALD_AI_TRY_TO_FAINT,
                    0x13579BDFu),
                "try-to-faint trainer AI fixture should start"))
            return 1;

        battle.battlers[0].pokemon.hp = 1;
        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "try-to-faint AI should choose a move"))
            return 1;

        if (!check(
                action.move_slot == 1,
                "TryToFaint must prefer a KO-capable Quick Attack over "
                "a slower KO-capable move"))
            return 1;
    }

    {
        static const uint16_t foe_moves[4] = {63, 252, 0, 0};

        if (!check(
                start_trainer_fixture(
                    &battle,
                    player_moves,
                    foe_moves,
                    REMASTER_EMERALD_AI_CHECK_VIABILITY,
                    0x2468ACE0u),
                "viability trainer AI fixture should start"))
            return 1;

        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "viability AI should choose a move"))
            return 1;

        if (!check(
                action.move_slot == 1,
                "CheckViability must apply Emerald's Fake Out +2 score"))
            return 1;
    }

    {
        static const uint16_t foe_moves[4] = {95, 63, 0, 0};

        if (!check(
                start_trainer_fixture(
                    &battle,
                    player_moves,
                    foe_moves,
                    REMASTER_EMERALD_AI_CHECK_BAD_MOVE,
                    0xABCDEF01u),
                "bad-move trainer AI fixture should start"))
            return 1;

        battle.battlers[0].pokemon.status =
            REMASTER_EMERALD_STATUS1_POISON;

        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "bad-move AI should choose a move"))
            return 1;

        if (!check(
                action.move_slot == 1,
                "CheckBadMove must penalize sleep against a statused target"))
            return 1;
    }

    {
        static const uint16_t player_left_moves[4] = {33, 0, 0, 0};
        static const uint16_t player_right_moves[4] = {33, 0, 0, 0};
        static const uint16_t foe_left_moves[4] = {52, 33, 0, 0};
        static const uint16_t foe_right_moves[4] = {33, 0, 0, 0};
        RemasterEmeraldPartyPokemon players[2];
        RemasterEmeraldPartyPokemon foes[2];

        if (!check(
                make_mon(&players[0], 1, 30, player_left_moves)
                    && make_mon(&players[1], 7, 30, player_right_moves)
                    && make_mon(&foes[0], 4, 30, foe_left_moves)
                    && make_mon(&foes[1], 37, 30, foe_right_moves),
                "doubles AI fixtures should build"))
            return 1;

        remaster_emerald_battle_state_init(
            &battle,
            REMASTER_EMERALD_BATTLE_TYPE_MASTER
                | REMASTER_EMERALD_BATTLE_TYPE_TRAINER
                | REMASTER_EMERALD_BATTLE_TYPE_DOUBLE,
            0x31415926u);
        if (!check(
                remaster_emerald_battle_start(
                    &battle, players, 2, foes, 2),
                "doubles trainer battle should start"))
            return 1;

        battle.opponent_trainer_ai_flags = 0;
        battle.battlers[3].ability = 18;
        battle.battlers[3].flash_fire = 0;

        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "doubles AI should choose an action"))
            return 1;

        if (!check(
                action.target == 3 && action.move_slot == 0,
                "Emerald doubles AI should deliberately trigger an "
                "unboosted Flash Fire partner with a Fire move"))
            return 1;
    }

    {
        static const uint16_t player_left_moves[4] = {33, 0, 0, 0};
        static const uint16_t player_right_moves[4] = {33, 0, 0, 0};
        static const uint16_t foe_left_moves[4] = {33, 0, 0, 0};
        static const uint16_t foe_right_moves[4] = {33, 0, 0, 0};
        RemasterEmeraldPartyPokemon players[2];
        RemasterEmeraldPartyPokemon foes[2];

        if (!check(
                make_mon(&players[0], 1, 30, player_left_moves)
                    && make_mon(&players[1], 7, 30, player_right_moves)
                    && make_mon(&foes[0], 4, 30, foe_left_moves)
                    && make_mon(&foes[1], 37, 30, foe_right_moves),
                "ordinary doubles AI fixtures should build"))
            return 1;

        remaster_emerald_battle_state_init(
            &battle,
            REMASTER_EMERALD_BATTLE_TYPE_MASTER
                | REMASTER_EMERALD_BATTLE_TYPE_TRAINER
                | REMASTER_EMERALD_BATTLE_TYPE_DOUBLE,
            0x27182818u);
        if (!check(
                remaster_emerald_battle_start(
                    &battle, players, 2, foes, 2),
                "ordinary doubles trainer battle should start"))
            return 1;

        battle.opponent_trainer_ai_flags = 0;
        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "ordinary doubles AI should choose an action"))
            return 1;

        if (!check(
                action.target == 0 || action.target == 2,
                "Emerald doubles AI must not use an ordinary damaging move "
                "against its partner"))
            return 1;
    }

    {
        static const uint16_t player_moves2[4] = {33, 0, 0, 0};
        static const uint16_t foe_active_moves[4] = {33, 0, 0, 0};
        static const uint16_t foe_reserve_moves[4] = {52, 0, 0, 0};
        RemasterEmeraldPartyPokemon player;
        RemasterEmeraldPartyPokemon foes[2];

        if (!check(
                make_mon(&player, 1, 30, player_moves2)
                    && make_mon(&foes[0], 4, 30, foe_active_moves)
                    && make_mon(&foes[1], 37, 30, foe_reserve_moves),
                "Wonder Guard switch fixtures should build"))
            return 1;

        remaster_emerald_battle_state_init(
            &battle,
            REMASTER_EMERALD_BATTLE_TYPE_MASTER
                | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
            2u);
        if (!check(
                remaster_emerald_battle_start(
                    &battle, &player, 1, foes, 2),
                "Wonder Guard switch battle should start"))
            return 1;

        battle.battlers[0].ability = 25;
        battle.opponent_trainer_ai_flags =
            REMASTER_EMERALD_AI_CHECK_BAD_MOVE;

        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "switch AI should choose an action"))
            return 1;

        if (!check(
                action.kind == REMASTER_EMERALD_BATTLE_ACTION_SWITCH
                    && action.party_slot == 1,
                "Wonder Guard AI should switch to a reserve with a "
                "super-effective move"))
            return 1;

        {
            RemasterEmeraldBattleAction turn_actions[
                REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
            turn_actions[1] = action;
            if (!check(
                    remaster_emerald_battle_resolve_turn(
                        &battle, turn_actions),
                    "AI switch action should resolve")
                || !check(
                    battle.battlers[1].party_slot == 1,
                    "AI switch should load the selected reserve"))
                return 1;
        }
    }

    {
        static const uint16_t player_moves2[4] = {33, 0, 0, 0};
        static const uint16_t foe_moves2[4] = {33, 0, 0, 0};
        RemasterEmeraldPartyPokemon player;
        RemasterEmeraldPartyPokemon foe;

        if (!check(
                make_mon(&player, 1, 30, player_moves2)
                    && make_mon(&foe, 4, 30, foe_moves2),
                "trainer item fixtures should build"))
            return 1;

        remaster_emerald_battle_state_init(
            &battle,
            REMASTER_EMERALD_BATTLE_TYPE_MASTER
                | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
            0x11223344u);
        if (!check(
                remaster_emerald_battle_start(
                    &battle, &player, 1, &foe, 1),
                "trainer item battle should start"))
            return 1;

        battle.opponent_trainer_items[0] = 19;
        battle.battlers[1].pokemon.hp = 1;
        battle.battlers[1].pokemon.status =
            REMASTER_EMERALD_STATUS1_POISON;

        if (!check(
                remaster_emerald_battle_choose_ai_action(
                    &battle, 1, &action),
                "trainer item AI should choose an action"))
            return 1;

        if (!check(
                action.kind == REMASTER_EMERALD_BATTLE_ACTION_ITEM
                    && action.item_id == 19,
                "low-HP trainer AI should choose Full Restore"))
            return 1;

        {
            RemasterEmeraldBattleAction turn_actions[
                REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
            turn_actions[1] = action;
            if (!check(
                    remaster_emerald_battle_resolve_turn(
                        &battle, turn_actions),
                    "trainer item action should resolve")
                || !check(
                    battle.battlers[1].pokemon.hp
                        == battle.battlers[1].pokemon.max_hp,
                    "Full Restore should heal trainer Pokémon to full HP")
                || !check(
                    battle.battlers[1].pokemon.status == 0,
                    "Full Restore should cure trainer Pokémon status")
                || !check(
                    battle.opponent_trainer_items[0] == 0,
                    "trainer item should be consumed exactly once"))
                return 1;
        }
    }

    puts("r13 trainer AI parity test passed");
    return 0;
}
