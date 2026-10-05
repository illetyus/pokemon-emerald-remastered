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
    const RemasterEmeraldSpeciesInfo *info;
    size_t i;

    memset(mon, 0, sizeof(*mon));
    mon->box.personality = (uint32_t)species * 29u + 7u;
    mon->box.ot_id = 0x13572468u;
    info = remaster_emerald_species_info(species);
    if (info == 0)
        return 0;

    if (!remaster_emerald_box_pokemon_set_species(&mon->box, species)
        || !remaster_emerald_box_pokemon_set_experience(
            &mon->box,
            remaster_emerald_experience_for_level(
                info->growth_rate,
                level)))
        return 0;

    for (i = 0; i < 6; ++i) {
        if (!remaster_emerald_box_pokemon_set_iv(&mon->box, i, ivs[i]))
            return 0;
    }
    for (i = 0; i < 4; ++i) {
        const RemasterEmeraldMoveInfo *move =
            remaster_emerald_move_info(moves[i]);
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

static int test_pursuit(void)
{
    const uint16_t idle[4] = {0, 0, 0, 0};
    const uint16_t pursuit[4] = {228, 0, 0, 0};
    RemasterEmeraldPartyPokemon player[2];
    RemasterEmeraldPartyPokemon foe[1];
    RemasterEmeraldBattleState battle;
    RemasterEmeraldBattleState base;
    RemasterEmeraldBattleState boosted;
    RemasterEmeraldBattleDamageResult normal_damage;
    RemasterEmeraldBattleDamageResult pursuit_damage;
    RemasterEmeraldBattleAction actions[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    size_t move_event = SIZE_MAX;
    size_t switch_event = SIZE_MAX;
    size_t i;
    uint16_t outgoing_hp;

    if (!make_mon(&player[0], 1, 35, idle)
        || !make_mon(&player[1], 4, 35, idle)
        || !make_mon(&foe[0], 25, 35, pursuit))
        return 0;

    remaster_emerald_battle_state_init(
        &base, REMASTER_EMERALD_BATTLE_TYPE_MASTER, 0xA1B2C3D4u);
    remaster_emerald_battle_state_init(
        &boosted, REMASTER_EMERALD_BATTLE_TYPE_MASTER, 0xA1B2C3D4u);
    if (!remaster_emerald_battle_start(&base, player, 2, foe, 1)
        || !remaster_emerald_battle_start(&boosted, player, 2, foe, 1))
        return 0;

    boosted.pursuit_boost[1] = 1;
    if (!remaster_emerald_battle_calculate_damage(
            &base, 1, 0, 228, &normal_damage)
        || !remaster_emerald_battle_calculate_damage(
            &boosted, 1, 0, 228, &pursuit_damage))
        return 0;
    if (!check(
            normal_damage.hit && pursuit_damage.hit
                && pursuit_damage.damage > normal_damage.damage,
            "Pursuit switch interception must increase damage"))
        return 0;

    remaster_emerald_battle_state_init(
        &battle, REMASTER_EMERALD_BATTLE_TYPE_MASTER, 0x11223344u);
    if (!remaster_emerald_battle_start(&battle, player, 2, foe, 1))
        return 0;
    outgoing_hp = battle.battlers[0].pokemon.hp;
    remaster_emerald_battle_clear_events(&battle);
    memset(actions, 0, sizeof(actions));
    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_SWITCH;
    actions[0].party_slot = 1;
    actions[1].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[1].move_slot = 0;
    actions[1].target = 0;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return 0;

    for (i = 0; i < battle.event_count; ++i) {
        if (battle.events[i].kind == REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED
            && battle.events[i].battler == 1
            && battle.events[i].move_id == 228
            && move_event == SIZE_MAX)
            move_event = i;
        if (battle.events[i].kind == REMASTER_EMERALD_BATTLE_EVENT_SWITCH
            && battle.events[i].battler == 0
            && switch_event == SIZE_MAX)
            switch_event = i;
    }

    return check(
               move_event != SIZE_MAX
                   && switch_event != SIZE_MAX
                   && move_event < switch_event,
               "Pursuit must execute before the switch")
        && check(
               battle.parties[0][0].hp < outgoing_hp,
               "Pursuit must damage the outgoing Pokemon")
        && check(
               battle.battlers[0].party_slot == 1,
               "switch must continue after surviving Pursuit");
}

static int test_baton_pass(void)
{
    const uint16_t baton[4] = {226, 0, 0, 0};
    const uint16_t idle[4] = {0, 0, 0, 0};
    RemasterEmeraldPartyPokemon player[2];
    RemasterEmeraldPartyPokemon foe[1];
    RemasterEmeraldBattleState battle;
    RemasterEmeraldBattleAction actions[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];

    if (!make_mon(&player[0], 25, 30, baton)
        || !make_mon(&player[1], 1, 30, idle)
        || !make_mon(&foe[0], 129, 20, idle))
        return 0;

    remaster_emerald_battle_state_init(
        &battle, REMASTER_EMERALD_BATTLE_TYPE_MASTER, 0x55667788u);
    if (!remaster_emerald_battle_start(&battle, player, 2, foe, 1))
        return 0;

    battle.battlers[0].stat_stages[1] = 9;
    battle.battlers[0].status2 |=
        REMASTER_EMERALD_STATUS2_FOCUS_ENERGY
        | REMASTER_EMERALD_STATUS2_SUBSTITUTE
        | REMASTER_EMERALD_STATUS2_CURSED;
    battle.battlers[0].status3 |=
        REMASTER_EMERALD_STATUS3_ROOTED
        | REMASTER_EMERALD_STATUS3_PERISH_SONG;
    battle.battlers[0].substitute_hp = 11;
    battle.battlers[0].perish_count = 3;

    memset(actions, 0, sizeof(actions));
    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    actions[0].party_slot = 1;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return 0;

    return check(
               battle.battlers[0].party_slot == 1,
               "Baton Pass must switch to requested party slot")
        && check(
               battle.battlers[0].stat_stages[1] == 9,
               "Baton Pass must preserve stat stages")
        && check(
               (battle.battlers[0].status2
                    & REMASTER_EMERALD_STATUS2_FOCUS_ENERGY) != 0,
               "Baton Pass must preserve Focus Energy")
        && check(
               (battle.battlers[0].status2
                    & REMASTER_EMERALD_STATUS2_SUBSTITUTE) != 0
                   && battle.battlers[0].substitute_hp == 11,
               "Baton Pass must preserve Substitute")
        && check(
               (battle.battlers[0].status3
                    & REMASTER_EMERALD_STATUS3_ROOTED) != 0,
               "Baton Pass must preserve Ingrain")
        && check(
               (battle.battlers[0].status3
                    & REMASTER_EMERALD_STATUS3_PERISH_SONG) != 0,
               "Baton Pass must preserve Perish Song state");
}

static int test_beat_up(void)
{
    const uint16_t beat_up[4] = {251, 0, 0, 0};
    const uint16_t idle[4] = {0, 0, 0, 0};
    RemasterEmeraldPartyPokemon player[3];
    RemasterEmeraldPartyPokemon foe[1];
    RemasterEmeraldBattleState battle;
    size_t i;
    unsigned damage_events = 0;

    if (!make_mon(&player[0], 25, 30, beat_up)
        || !make_mon(&player[1], 4, 30, idle)
        || !make_mon(&player[2], 7, 30, idle)
        || !make_mon(&foe[0], 143, 50, idle))
        return 0;

    player[2].status = REMASTER_EMERALD_STATUS1_POISON;

    remaster_emerald_battle_state_init(
        &battle, REMASTER_EMERALD_BATTLE_TYPE_MASTER, 0xCAFED00Du);
    if (!remaster_emerald_battle_start(&battle, player, 3, foe, 1))
        return 0;

    remaster_emerald_battle_clear_events(&battle);
    if (!remaster_emerald_battle_use_move(&battle, 0, 1, 0))
        return 0;

    for (i = 0; i < battle.event_count; ++i) {
        if (battle.events[i].kind == REMASTER_EMERALD_BATTLE_EVENT_DAMAGE
            && battle.events[i].move_id == 251
            && battle.events[i].target == 1)
            damage_events++;
    }

    return check(
        damage_events == 2,
        "Beat Up must hit once per healthy status-free party member");
}

int main(void)
{
    if (!test_pursuit())
        return 1;
    if (!test_baton_pass())
        return 1;
    if (!test_beat_up())
        return 1;

    puts("r13 move effects test passed");
    return 0;
}
