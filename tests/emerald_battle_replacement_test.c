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
    static const uint16_t quick_attack[4] = {98, 0, 0, 0};
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foes[2];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint32_t turn_after_faint;

    if (!make_mon(&player, 1, 60, quick_attack)
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


static int test_double_replacement_uses_distinct_reserves(void)
{
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon players[2];
    RemasterEmeraldPartyPokemon foes[4];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint32_t turn_before;

    if (!make_mon(&players[0], 1, 30, tackle)
        || !make_mon(&players[1], 7, 30, tackle)
        || !make_mon(&foes[0], 4, 20, tackle)
        || !make_mon(&foes[1], 7, 20, tackle)
        || !make_mon(&foes[2], 25, 20, tackle)
        || !make_mon(&foes[3], 23, 20, tackle))
        return check(0, "double replacement fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER
            | REMASTER_EMERALD_BATTLE_TYPE_DOUBLE,
        0xD00B1E01u);
    if (!remaster_emerald_battle_start(
            &battle, players, 2, foes, 4))
        return check(0, "double replacement battle should start");

    battle.battlers[1].pokemon.hp = 0;
    battle.battlers[1].fainted = 1;
    battle.parties[1][0].hp = 0;
    battle.battlers[3].pokemon.hp = 0;
    battle.battlers[3].fainted = 1;
    battle.parties[1][1].hp = 0;
    turn_before = battle.turn_number;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "double replacement phase should resolve");

    return check(
        !battle.battlers[1].fainted
            && !battle.battlers[3].fainted
            && battle.battlers[1].party_slot >= 2
            && battle.battlers[3].party_slot >= 2
            && battle.battlers[1].party_slot
                != battle.battlers[3].party_slot
            && battle.turn_number == turn_before,
        "double replacement must reserve two distinct party slots "
        "without consuming a turn");
}

static int test_faint_clears_persistent_and_volatile_status(void)
{
    static const uint16_t quick_attack[4] = {98, 0, 0, 0};
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foe;
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};

    if (!make_mon(&player, 1, 60, quick_attack)
        || !make_mon(&foe, 4, 5, tackle))
        return check(0, "faint cleanup fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER,
        0xFA170001u);
    if (!remaster_emerald_battle_start(
            &battle, &player, 1, &foe, 1))
        return check(0, "faint cleanup battle should start");

    battle.battlers[1].pokemon.hp = 1;
    battle.parties[1][0].hp = 1;
    battle.battlers[1].pokemon.status =
        REMASTER_EMERALD_STATUS1_POISON;
    battle.battlers[1].status2 =
        REMASTER_EMERALD_STATUS2_CONFUSION
        | REMASTER_EMERALD_STATUS2_ESCAPE_PREVENTION;
    battle.battlers[1].status3 =
        REMASTER_EMERALD_STATUS3_ROOTED
        | REMASTER_EMERALD_STATUS3_GRUDGE;

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "faint cleanup KO turn should resolve");

    return check(
        battle.battlers[1].fainted
            && battle.battlers[1].pokemon.status == 0
            && battle.battlers[1].status2 == 0
            && battle.battlers[1].status3 == 0
            && battle.parties[1][0].status == 0,
        "faint cleanup must clear status1 and volatile battle state "
        "before syncing the party");
}

static int test_spikes_ko_blocks_entry_ability(void)
{
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foes[2];

    if (!make_mon(&player, 1, 30, tackle)
        || !make_mon(&foes[0], 4, 30, tackle)
        || !make_mon(&foes[1], 23, 30, tackle))
        return check(0, "Spikes KO fixtures should build");

    if (!check(
            remaster_emerald_species_ability(23, 0) == 22,
            "Ekans fixture should expose Intimidate as ability 0"))
        return 0;

    foes[1].hp = 1;
    foes[1].box.checksum =
        remaster_emerald_box_pokemon_checksum(&foes[1].box);

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        0x5A1CE001u);
    if (!remaster_emerald_battle_start(
            &battle, &player, 1, foes, 2))
        return check(0, "Spikes KO battle should start");

    battle.side_status[1] |= REMASTER_EMERALD_SIDE_SPIKES;
    battle.spikes_layers[1] = 1;

    if (!check(
            remaster_emerald_battle_switch(&battle, 1, 1),
            "Spikes KO switch should resolve"))
        return 0;

    return check(
        battle.battlers[1].fainted
            && battle.battlers[0].stat_stages[1] == 6,
        "a Pokémon fainting to Spikes must not activate its "
        "switch-in ability");
}

static int test_doubles_item_switch_order_matches_emerald(void)
{
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon players[2];
    RemasterEmeraldPartyPokemon foes[3];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    size_t item_event = SIZE_MAX;
    size_t switch_event = SIZE_MAX;
    size_t i;

    if (!make_mon(&players[0], 1, 30, tackle)
        || !make_mon(&players[1], 7, 30, tackle)
        || !make_mon(&foes[0], 4, 30, tackle)
        || !make_mon(&foes[1], 25, 30, tackle)
        || !make_mon(&foes[2], 23, 30, tackle))
        return check(0, "item/switch ordering fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER
            | REMASTER_EMERALD_BATTLE_TYPE_DOUBLE,
        0x0DDE0001u);
    if (!remaster_emerald_battle_start(
            &battle, players, 2, foes, 3))
        return check(0, "item/switch ordering battle should start");

    battle.battlers[1].pokemon.hp = 1;
    battle.parties[1][0].hp = 1;
    remaster_emerald_battle_clear_events(&battle);

    actions[1].kind = REMASTER_EMERALD_BATTLE_ACTION_ITEM;
    actions[1].item_id = 19;
    actions[1].target = 1;
    actions[3].kind = REMASTER_EMERALD_BATTLE_ACTION_SWITCH;
    actions[3].party_slot = 2;
    actions[3].target = 3;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "item/switch ordering turn should resolve");

    for (i = 0; i < battle.event_count; ++i) {
        if (battle.events[i].kind == REMASTER_EMERALD_BATTLE_EVENT_ITEM
            && item_event == SIZE_MAX)
            item_event = i;
        if (battle.events[i].kind == REMASTER_EMERALD_BATTLE_EVENT_SWITCH
            && switch_event == SIZE_MAX)
            switch_event = i;
    }

    return check(
        item_event != SIZE_MAX
            && switch_event != SIZE_MAX
            && item_event < switch_event,
        "Emerald must keep item/switch actions in battler-index order "
        "instead of speed-sorting them");
}

static int test_pursuit_intercepts_switch(void)
{
    static const uint16_t pursuit[4] = {228, 0, 0, 0};
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player;
    RemasterEmeraldPartyPokemon foes[2];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint16_t old_hp;
    size_t move_event = SIZE_MAX;
    size_t switch_event = SIZE_MAX;
    size_t i;

    if (!make_mon(&player, 1, 10, pursuit)
        || !make_mon(&foes[0], 4, 40, tackle)
        || !make_mon(&foes[1], 7, 40, tackle))
        return check(0, "Pursuit switch fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER,
        0x22800001u);
    if (!remaster_emerald_battle_start(
            &battle, &player, 1, foes, 2))
        return check(0, "Pursuit switch battle should start");

    old_hp = battle.battlers[1].pokemon.hp;
    remaster_emerald_battle_clear_events(&battle);

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    actions[1].kind = REMASTER_EMERALD_BATTLE_ACTION_SWITCH;
    actions[1].party_slot = 1;
    actions[1].target = 1;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "Pursuit switch turn should resolve");

    for (i = 0; i < battle.event_count; ++i) {
        if (battle.events[i].kind == REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED
            && battle.events[i].battler == 0
            && move_event == SIZE_MAX)
            move_event = i;
        if (battle.events[i].kind == REMASTER_EMERALD_BATTLE_EVENT_SWITCH
            && battle.events[i].battler == 1
            && switch_event == SIZE_MAX)
            switch_event = i;
    }

    return check(
        battle.battlers[1].party_slot == 1
            && battle.parties[1][0].hp < old_hp
            && move_event != SIZE_MAX
            && switch_event != SIZE_MAX
            && move_event < switch_event,
        "Pursuit must resolve against the outgoing Pokémon before switch");
}


static int test_fainted_action_is_cancelled_and_target_retargets(void)
{
    static const uint16_t quick_attack[4] = {98, 0, 0, 0};
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon players[2];
    RemasterEmeraldPartyPokemon foes[2];
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};
    uint8_t fainted_pp;
    uint16_t partner_hp;
    size_t retarget_event = SIZE_MAX;
    size_t fainted_move_event = SIZE_MAX;
    size_t i;

    if (!make_mon(&players[0], 1, 60, quick_attack)
        || !make_mon(&players[1], 7, 30, tackle)
        || !make_mon(&foes[0], 4, 20, tackle)
        || !make_mon(&foes[1], 25, 20, tackle))
        return check(0, "fainted-action retarget fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_DOUBLE,
        0xFA17A2E7u);
    if (!remaster_emerald_battle_start(
            &battle, players, 2, foes, 2))
        return check(0, "fainted-action retarget battle should start");

    battle.battlers[1].pokemon.hp = 1;
    battle.parties[1][0].hp = 1;
    fainted_pp = battle.battlers[1].pp[0];
    partner_hp = battle.battlers[3].pokemon.hp;
    remaster_emerald_battle_clear_events(&battle);

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[0].move_slot = 0;
    actions[0].target = 1;
    actions[1].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[1].move_slot = 0;
    actions[1].target = 0;
    actions[2].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    actions[2].move_slot = 0;
    actions[2].target = 1;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "fainted-action retarget turn should resolve");

    for (i = 0; i < battle.event_count; ++i) {
        if (battle.events[i].kind != REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED)
            continue;
        if (battle.events[i].battler == 1)
            fainted_move_event = i;
        if (battle.events[i].battler == 2
            && battle.events[i].target == 3)
            retarget_event = i;
    }

    return check(
        battle.battlers[1].fainted
            && fainted_move_event == SIZE_MAX
            && battle.battlers[1].pp[0] == fainted_pp
            && retarget_event != SIZE_MAX
            && battle.battlers[3].pokemon.hp < partner_hp,
        "a battler fainted earlier in the turn must lose its queued action "
        "and later moves must retarget to the surviving opposing partner");
}

static int test_arena_blocks_voluntary_switch(void)
{
    static const uint16_t tackle[4] = {33, 0, 0, 0};
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon players[2];
    RemasterEmeraldPartyPokemon foe;
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS] = {{0}};

    if (!make_mon(&players[0], 1, 40, tackle)
        || !make_mon(&players[1], 7, 40, tackle)
        || !make_mon(&foe, 4, 20, tackle))
        return check(0, "Arena switch fixtures should build");

    remaster_emerald_battle_state_init(
        &battle,
        REMASTER_EMERALD_BATTLE_TYPE_MASTER
            | REMASTER_EMERALD_BATTLE_TYPE_TRAINER
            | REMASTER_EMERALD_BATTLE_TYPE_ARENA,
        0xA2EAA001u);
    if (!remaster_emerald_battle_start(
            &battle, players, 2, &foe, 1))
        return check(0, "Arena switch battle should start");

    actions[0].kind = REMASTER_EMERALD_BATTLE_ACTION_SWITCH;
    actions[0].party_slot = 1;
    actions[0].target = 0;

    if (!remaster_emerald_battle_resolve_turn(&battle, actions))
        return check(0, "Arena switch turn should resolve");

    return check(
        battle.battlers[0].party_slot == 0,
        "Battle Arena must reject voluntary switching");
}

int main(void)
{
    if (!test_trainer_replacement())
        return 1;
    if (!test_player_replacement())
        return 1;
    if (!test_double_replacement_uses_distinct_reserves())
        return 1;
    if (!test_faint_clears_persistent_and_volatile_status())
        return 1;
    if (!test_spikes_ko_blocks_entry_ability())
        return 1;
    if (!test_doubles_item_switch_order_matches_emerald())
        return 1;
    if (!test_pursuit_intercepts_switch())
        return 1;
    if (!test_fainted_action_is_cancelled_and_target_retargets())
        return 1;
    if (!test_arena_blocks_voluntary_switch())
        return 1;

    puts("r13 faint replacement and lifecycle test passed");
    return 0;
}
