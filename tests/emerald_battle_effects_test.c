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
    uint16_t species_id,
    uint8_t level,
    const uint16_t moves[4])
{
    const RemasterEmeraldSpeciesInfo *species =
        remaster_emerald_species_info(species_id);
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[6] = {31, 31, 31, 31, 31, 31};
    uint8_t evs[6] = {0, 0, 0, 0, 0, 0};
    size_t i;

    if (species == 0)
        return 0;

    memset(mon, 0, sizeof(*mon));
    mon->box.personality = (uint32_t)species_id * 31u + 7u;
    mon->box.ot_id = 0x12345678u;

    if (!remaster_emerald_box_pokemon_set_species(&mon->box, species_id)
        || !remaster_emerald_box_pokemon_set_experience(
            &mon->box,
            remaster_emerald_experience_for_level(
                species->growth_rate,
                level)))
        return 0;

    for (i = 0; i < 6; ++i) {
        if (!remaster_emerald_box_pokemon_set_iv(&mon->box, i, ivs[i]))
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
            species_id,
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

static int start_fixture(
    RemasterEmeraldBattleState *battle,
    const uint16_t player_moves[4],
    const uint16_t foe_moves[4],
    uint32_t seed)
{
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foe;

    if (!make_mon(&player, 4, 30, player_moves)
        || !make_mon(&foe, 1, 30, foe_moves))
        return 0;

    remaster_emerald_battle_state_init(
        battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        seed);
    return remaster_emerald_battle_start(
        battle, &player, 1, &foe, 1);
}

int main(void)
{
    const uint16_t foe_moves[4] = {1, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldBattleAction actions[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint16_t hp_before;
    uint16_t received;

    {
        const uint16_t moves[4] = {203, 68, 254, 255};
        if (!check(start_fixture(&battle, moves, foe_moves, 0x1001u),
                "endure fixture"))
            return 1;

        battle.turn_number = 1;
        battle.battlers[0].pokemon.hp = 2;
        if (!check(remaster_emerald_battle_use_move(&battle, 0, 1, 0),
                "Endure executes")
            || !check(remaster_emerald_battle_use_move(&battle, 1, 0, 0),
                "foe attack executes")
            || !check(battle.battlers[0].pokemon.hp == 1,
                "Endure must leave user on 1 HP"))
            return 1;
    }

    {
        const uint16_t moves[4] = {68, 254, 255, 203};
        if (!check(start_fixture(&battle, moves, foe_moves, 0x2002u),
                "counter fixture"))
            return 1;

        battle.turn_number = 2;
        if (!check(remaster_emerald_battle_use_move(&battle, 1, 0, 0),
                "physical hit before Counter"))
            return 1;
        received = battle.battlers[0].last_damage;
        hp_before = battle.battlers[1].pokemon.hp;
        if (!check(received != 0, "Counter needs recorded damage")
            || !check(remaster_emerald_battle_use_move(&battle, 0, 1, 0),
                "Counter executes")
            || !check(
                battle.battlers[1].pokemon.hp
                    == (hp_before > received * 2u
                        ? hp_before - received * 2u
                        : 0),
                "Counter must reflect double physical damage"))
            return 1;
    }

    {
        const uint16_t moves[4] = {254, 255, 203, 68};
        if (!check(start_fixture(&battle, moves, foe_moves, 0x3003u),
                "stockpile fixture"))
            return 1;

        battle.turn_number = 1;
        if (!check(remaster_emerald_battle_use_move(&battle, 0, 1, 0),
                "Stockpile one")
            || !check(remaster_emerald_battle_use_move(&battle, 0, 1, 0),
                "Stockpile two")
            || !check(battle.battlers[0].stockpile == 2,
                "Stockpile count"))
            return 1;

        hp_before = battle.battlers[1].pokemon.hp;
        if (!check(remaster_emerald_battle_use_move(&battle, 0, 1, 1),
                "Spit Up executes")
            || !check(battle.battlers[1].pokemon.hp < hp_before,
                "Spit Up damages")
            || !check(battle.battlers[0].stockpile == 0,
                "Spit Up consumes Stockpile"))
            return 1;
    }

    {
        const uint16_t moves[4] = {229, 194, 281, 256};
        if (!check(start_fixture(&battle, moves, foe_moves, 0x4004u),
                "rapid spin fixture"))
            return 1;

        battle.turn_number = 1;
        battle.battlers[0].status2 |= REMASTER_EMERALD_STATUS2_WRAPPED;
        battle.battlers[0].trapped_turns = 3;
        battle.battlers[0].status3 |= REMASTER_EMERALD_STATUS3_LEECH_SEED;
        battle.spikes_layers[0] = 2;
        battle.side_status[0] |= REMASTER_EMERALD_SIDE_SPIKES;

        if (!check(remaster_emerald_battle_use_move(&battle, 0, 1, 0),
                "Rapid Spin executes")
            || !check(
                (battle.battlers[0].status2
                    & REMASTER_EMERALD_STATUS2_WRAPPED) == 0,
                "Rapid Spin clears wrap")
            || !check(
                (battle.battlers[0].status3
                    & REMASTER_EMERALD_STATUS3_LEECH_SEED) == 0,
                "Rapid Spin clears Leech Seed")
            || !check(
                battle.spikes_layers[0] == 0
                    && (battle.side_status[0]
                        & REMASTER_EMERALD_SIDE_SPIKES) == 0,
                "Rapid Spin clears Spikes"))
            return 1;
    }

    {
        const uint16_t moves[4] = {194, 281, 229, 256};
        if (!check(start_fixture(&battle, moves, foe_moves, 0x5005u),
                "destiny bond fixture"))
            return 1;

        battle.turn_number = 1;
        battle.battlers[0].pokemon.hp = 1;
        if (!check(remaster_emerald_battle_use_move(&battle, 0, 1, 0),
                "Destiny Bond executes")
            || !check(remaster_emerald_battle_use_move(&battle, 1, 0, 0),
                "lethal foe attack executes")
            || !check(
                battle.battlers[0].fainted
                    && battle.battlers[1].fainted,
                "Destiny Bond must faint attacker"))
            return 1;
    }

    {
        const uint16_t moves[4] = {281, 229, 194, 256};
        if (!check(start_fixture(&battle, moves, foe_moves, 0x6006u),
                "yawn fixture"))
            return 1;

        battle.turn_number = 0;
        if (!check(remaster_emerald_battle_use_move(&battle, 0, 1, 0),
                "Yawn executes"))
            return 1;

        memset(actions, 0, sizeof(actions));
        if (!check(remaster_emerald_battle_resolve_turn(&battle, actions),
                "Yawn countdown turn one")
            || !check(remaster_emerald_battle_resolve_turn(&battle, actions),
                "Yawn countdown turn two")
            || !check(
                (battle.battlers[1].pokemon.status
                    & REMASTER_EMERALD_STATUS1_SLEEP) != 0,
                "Yawn must apply delayed sleep"))
            return 1;
    }

    puts("r13 stateful move effects test passed");
    return 0;
}
