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
    mon->box.personality =
        UINT32_C(0x10203040) + (uint32_t)species * 17u + level;
    mon->box.ot_id = UINT32_C(0x55667788);

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

static int run_wild_replay(
    uint32_t seed,
    RemasterEmeraldBattleState *out)
{
    static const uint16_t player_moves[4] = {98, 0, 0, 0};
    static const uint16_t wild_moves[4] = {33, 0, 0, 0};
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon wild;
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};

    if (!make_mon(&player, 4, 20, player_moves)
        || !make_mon(&wild, 129, 5, wild_moves))
        return 0;

    wild.hp = 1;
    wild.box.checksum =
        remaster_emerald_box_pokemon_checksum(&wild.box);

    remaster_emerald_battle_state_init(
        out,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        seed);
    if (!remaster_emerald_battle_start(
            out, &player, 1, &wild, 1))
        return 0;

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;

    return remaster_emerald_battle_resolve_turn(out, actions)
        && out->ended
        && out->outcome == REMASTER_EMERALD_BATTLE_OUTCOME_WON;
}

static int run_trainer_replay(
    uint32_t seed,
    RemasterEmeraldBattleState *out)
{
    static const uint16_t player_moves[4] = {98, 0, 0, 0};
    static const uint16_t foe_moves[4] = {33, 0, 0, 0};
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foes[2];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};

    if (!make_mon(&player, 1, 60, player_moves)
        || !make_mon(&foes[0], 4, 5, foe_moves)
        || !make_mon(&foes[1], 7, 10, foe_moves))
        return 0;

    foes[0].hp = 1;
    foes[1].hp = 1;

    remaster_emerald_battle_state_init(
        out,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        seed);
    if (!remaster_emerald_battle_start(
            out, &player, 1, foes, 2))
        return 0;

    out->opponent_trainer_id = 1;
    out->opponent_trainer_ai_flags =
        REMASTER_EMERALD_AI_CHECK_BAD_MOVE
        | REMASTER_EMERALD_AI_TRY_TO_FAINT
        | REMASTER_EMERALD_AI_CHECK_VIABILITY;

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    if (!remaster_emerald_battle_resolve_turn(out, actions)
        || !remaster_emerald_battle_needs_replacement(out, 1))
        return 0;

    memset(actions, 0, sizeof(actions));
    if (!remaster_emerald_battle_resolve_turn(out, actions)
        || out->battlers[1].party_slot != 1)
        return 0;

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    return remaster_emerald_battle_resolve_turn(out, actions)
        && out->ended
        && out->outcome == REMASTER_EMERALD_BATTLE_OUTCOME_WON;
}

static int run_loss_replay(
    uint32_t seed,
    RemasterEmeraldBattleState *out)
{
    static const uint16_t player_moves[4] = {33, 0, 0, 0};
    static const uint16_t foe_moves[4] = {98, 0, 0, 0};
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foe;
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};

    if (!make_mon(&player, 1, 5, player_moves)
        || !make_mon(&foe, 4, 60, foe_moves))
        return 0;

    player.hp = 1;

    remaster_emerald_battle_state_init(
        out,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        seed);
    if (!remaster_emerald_battle_start(
            out, &player, 1, &foe, 1))
        return 0;

    out->opponent_trainer_id = 1;
    out->opponent_trainer_ai_flags = 0;

    return remaster_emerald_battle_resolve_turn(out, actions)
        && out->ended
        && out->outcome == REMASTER_EMERALD_BATTLE_OUTCOME_LOST;
}

int main(void)
{
    RemasterEmeraldBattleState a;
    RemasterEmeraldBattleState b;
    RemasterEmeraldSave save;
    int whiteout = 0;

    if (!check(
            run_wild_replay(UINT32_C(0x11112222), &a)
                && run_wild_replay(UINT32_C(0x11112222), &b),
            "wild replay fixtures should complete"))
        return 1;
    if (!check(
            memcmp(&a, &b, sizeof(a)) == 0,
            "fixed-seed wild battle replay must be byte-deterministic"))
        return 1;

    if (!check(
            run_trainer_replay(UINT32_C(0x33334444), &a)
                && run_trainer_replay(UINT32_C(0x33334444), &b),
            "trainer replay fixtures should complete"))
        return 1;
    if (!check(
            memcmp(&a, &b, sizeof(a)) == 0,
            "fixed-seed trainer battle with replacement must replay exactly"))
        return 1;

    if (!check(
            run_loss_replay(UINT32_C(0x55556666), &a)
                && run_loss_replay(UINT32_C(0x55556666), &b),
            "loss replay fixtures should complete"))
        return 1;
    if (!check(
            memcmp(&a, &b, sizeof(a)) == 0,
            "fixed-seed trainer loss must replay exactly"))
        return 1;

    memset(&save, 0, sizeof(save));
    if (!check(
            remaster_emerald_battle_finalize_trainer(
                &a, &save, &whiteout),
            "deterministic trainer loss should finalize"))
        return 1;
    if (!check(
            whiteout
                && remaster_emerald_party_count(&save) == 1,
            "loss finalization must request whiteout and commit party"))
        return 1;

    puts("r13 battle replay acceptance passed");
    return 0;
}
