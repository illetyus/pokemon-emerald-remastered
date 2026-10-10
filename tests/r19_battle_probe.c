#include "remaster/emerald_battle.h"
#include "remaster/emerald_qol.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hex(const uint8_t *bytes, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) printf("%02x", (unsigned)bytes[i]);
}

static int number(const char *s, uint32_t *value)
{
    const char *p;
    unsigned long long parsed;
    if (*s == '\0' || strlen(s) > 10) return 0;
    for (p = s; *p; ++p) if (*p < '0' || *p > '9') return 0;
    parsed = strtoull(s, NULL, 10);
    if (parsed > UINT32_MAX) return 0;
    *value = (uint32_t)parsed;
    return 1;
}

static int write_pokemon(const RemasterEmeraldPartyPokemon *mon)
{
    RemasterEmeraldSave codec;
    memset(&codec, 0, sizeof(codec));
    if (!remaster_emerald_party_set(&codec, 0, mon)) return 0;
    putchar('"');
    hex(codec.save_block1 + 0x238, REMASTER_EMERALD_PARTY_POKEMON_BYTES);
    putchar('"');
    return !ferror(stdout);
}
static int write_RemasterEmeraldBattleRng(const RemasterEmeraldBattleRng *v)
{
    putchar('{');
    fputs("\"state\":", stdout);
    printf("%lu", (unsigned long)v->state);
    putchar(',');
    fputs("\"calls\":", stdout);
    printf("%llu", (unsigned long long)v->calls);
    putchar('}');
    return !ferror(stdout);
}

static int write_RemasterEmeraldBattleEvent(const RemasterEmeraldBattleEvent *v)
{
    putchar('{');
    fputs("\"kind\":", stdout);
    printf("%u", (unsigned)v->kind);
    putchar(',');
    fputs("\"battler\":", stdout);
    printf("%u", (unsigned)v->battler);
    putchar(',');
    fputs("\"target\":", stdout);
    printf("%u", (unsigned)v->target);
    putchar(',');
    fputs("\"move_id\":", stdout);
    printf("%u", (unsigned)v->move_id);
    putchar(',');
    fputs("\"value\":", stdout);
    printf("%ld", (long)v->value);
    putchar(',');
    fputs("\"aux\":", stdout);
    printf("%lu", (unsigned long)v->aux);
    putchar('}');
    return !ferror(stdout);
}

static int write_RemasterEmeraldBattleMon(const RemasterEmeraldBattleMon *v)
{
    putchar('{');
    fputs("\"pokemon\":", stdout);
    if (!write_pokemon(&v->pokemon)) return 0;
    putchar(',');
    fputs("\"species\":", stdout);
    printf("%u", (unsigned)v->species);
    putchar(',');
    fputs("\"held_item\":", stdout);
    printf("%u", (unsigned)v->held_item);
    putchar(',');
    fputs("\"moves\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 4; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->moves[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"pp\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 4; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->pp[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"types\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 2; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->types[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"ability\":", stdout);
    printf("%u", (unsigned)v->ability);
    putchar(',');
    fputs("\"stat_stages\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_STAT_COUNT; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->stat_stages[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"status2\":", stdout);
    printf("%lu", (unsigned long)v->status2);
    putchar(',');
    fputs("\"status3\":", stdout);
    printf("%lu", (unsigned long)v->status3);
    putchar(',');
    fputs("\"substitute_hp\":", stdout);
    printf("%u", (unsigned)v->substitute_hp);
    putchar(',');
    fputs("\"last_move\":", stdout);
    printf("%u", (unsigned)v->last_move);
    putchar(',');
    fputs("\"last_taken_move\":", stdout);
    printf("%u", (unsigned)v->last_taken_move);
    putchar(',');
    fputs("\"choice_locked_move\":", stdout);
    printf("%u", (unsigned)v->choice_locked_move);
    putchar(',');
    fputs("\"last_damage\":", stdout);
    printf("%u", (unsigned)v->last_damage);
    putchar(',');
    fputs("\"bide_damage\":", stdout);
    printf("%u", (unsigned)v->bide_damage);
    putchar(',');
    fputs("\"trapped_move\":", stdout);
    printf("%u", (unsigned)v->trapped_move);
    putchar(',');
    fputs("\"last_damage_from\":", stdout);
    printf("%u", (unsigned)v->last_damage_from);
    putchar(',');
    fputs("\"last_damage_type\":", stdout);
    printf("%u", (unsigned)v->last_damage_type);
    putchar(',');
    fputs("\"last_damage_was_physical\":", stdout);
    printf("%u", (unsigned)v->last_damage_was_physical);
    putchar(',');
    fputs("\"last_damage_turn\":", stdout);
    printf("%u", (unsigned)v->last_damage_turn);
    putchar(',');
    fputs("\"side\":", stdout);
    printf("%u", (unsigned)v->side);
    putchar(',');
    fputs("\"party_slot\":", stdout);
    printf("%u", (unsigned)v->party_slot);
    putchar(',');
    fputs("\"active\":", stdout);
    printf("%u", (unsigned)v->active);
    putchar(',');
    fputs("\"fainted\":", stdout);
    printf("%u", (unsigned)v->fainted);
    putchar(',');
    fputs("\"protected_turn\":", stdout);
    printf("%u", (unsigned)v->protected_turn);
    putchar(',');
    fputs("\"endure_turn\":", stdout);
    printf("%u", (unsigned)v->endure_turn);
    putchar(',');
    fputs("\"entered_turn\":", stdout);
    printf("%u", (unsigned)v->entered_turn);
    putchar(',');
    fputs("\"protect_chain\":", stdout);
    printf("%u", (unsigned)v->protect_chain);
    putchar(',');
    fputs("\"stockpile\":", stdout);
    printf("%u", (unsigned)v->stockpile);
    putchar(',');
    fputs("\"rollout_count\":", stdout);
    printf("%u", (unsigned)v->rollout_count);
    putchar(',');
    fputs("\"fury_cutter_count\":", stdout);
    printf("%u", (unsigned)v->fury_cutter_count);
    putchar(',');
    fputs("\"trapped_turns\":", stdout);
    printf("%u", (unsigned)v->trapped_turns);
    putchar(',');
    fputs("\"rampage_turns\":", stdout);
    printf("%u", (unsigned)v->rampage_turns);
    putchar(',');
    fputs("\"uproar_turns\":", stdout);
    printf("%u", (unsigned)v->uproar_turns);
    putchar(',');
    fputs("\"bide_turns\":", stdout);
    printf("%u", (unsigned)v->bide_turns);
    putchar(',');
    fputs("\"lock_on_turns\":", stdout);
    printf("%u", (unsigned)v->lock_on_turns);
    putchar(',');
    fputs("\"lock_on_target\":", stdout);
    printf("%u", (unsigned)v->lock_on_target);
    putchar(',');
    fputs("\"perish_count\":", stdout);
    printf("%u", (unsigned)v->perish_count);
    putchar(',');
    fputs("\"toxic_counter\":", stdout);
    printf("%u", (unsigned)v->toxic_counter);
    putchar(',');
    fputs("\"flash_fire\":", stdout);
    printf("%u", (unsigned)v->flash_fire);
    putchar(',');
    fputs("\"taunt_turns\":", stdout);
    printf("%u", (unsigned)v->taunt_turns);
    putchar(',');
    fputs("\"encore_turns\":", stdout);
    printf("%u", (unsigned)v->encore_turns);
    putchar(',');
    fputs("\"encore_move_slot\":", stdout);
    printf("%u", (unsigned)v->encore_move_slot);
    putchar(',');
    fputs("\"disable_turns\":", stdout);
    printf("%u", (unsigned)v->disable_turns);
    putchar(',');
    fputs("\"disable_move_slot\":", stdout);
    printf("%u", (unsigned)v->disable_move_slot);
    putchar(',');
    fputs("\"destiny_bond_turn\":", stdout);
    printf("%u", (unsigned)v->destiny_bond_turn);
    putchar(',');
    fputs("\"grudge_turn\":", stdout);
    printf("%u", (unsigned)v->grudge_turn);
    putchar(',');
    fputs("\"charging_move\":", stdout);
    printf("%u", (unsigned)v->charging_move);
    putchar(',');
    fputs("\"charging_move_slot\":", stdout);
    printf("%u", (unsigned)v->charging_move_slot);
    putchar(',');
    fputs("\"charging_target\":", stdout);
    printf("%u", (unsigned)v->charging_target);
    putchar(',');
    fputs("\"helping_hand\":", stdout);
    printf("%u", (unsigned)v->helping_hand);
    putchar(',');
    fputs("\"magic_coat\":", stdout);
    printf("%u", (unsigned)v->magic_coat);
    putchar(',');
    fputs("\"snatch\":", stdout);
    printf("%u", (unsigned)v->snatch);
    putchar(',');
    fputs("\"imprison\":", stdout);
    printf("%u", (unsigned)v->imprison);
    putchar(',');
    fputs("\"temporary_move_mask\":", stdout);
    printf("%u", (unsigned)v->temporary_move_mask);
    putchar(',');
    fputs("\"last_consumed_item\":", stdout);
    printf("%u", (unsigned)v->last_consumed_item);
    putchar('}');
    return !ferror(stdout);
}

static int write_RemasterEmeraldBattleState(const RemasterEmeraldBattleState *v)
{
    if (v->event_count > REMASTER_EMERALD_BATTLE_EVENT_CAPACITY) return 0;
    fputs("{\"version\":1,", stdout);
    fputs("\"battle_type_flags\":", stdout);
    printf("%lu", (unsigned long)v->battle_type_flags);
    putchar(',');
    fputs("\"rng\":", stdout);
    if (!write_RemasterEmeraldBattleRng(&v->rng)) return 0;
    putchar(',');
    fputs("\"battlers\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        if (!write_RemasterEmeraldBattleMon(&v->battlers[i0])) return 0;
    }
    putchar(']');
    putchar(',');
    fputs("\"parties\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 2; ++i0) {
        if (i0) putchar(',');
        putchar('[');
        for (size_t i1 = 0; i1 < REMASTER_EMERALD_BATTLE_PARTY_SIZE; ++i1) {
            if (i1) putchar(',');
            if (!write_pokemon(&v->parties[i0][i1])) return 0;
        }
        putchar(']');
    }
    putchar(']');
    putchar(',');
    fputs("\"party_count\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 2; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->party_count[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"active_party_slot\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->active_party_slot[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"side_status\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 2; ++i0) {
        if (i0) putchar(',');
        printf("%lu", (unsigned long)v->side_status[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"spikes_layers\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 2; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->spikes_layers[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"weather\":", stdout);
    printf("%u", (unsigned)v->weather);
    putchar(',');
    fputs("\"weather_turns\":", stdout);
    printf("%u", (unsigned)v->weather_turns);
    putchar(',');
    fputs("\"player_badge_mask\":", stdout);
    printf("%u", (unsigned)v->player_badge_mask);
    putchar(',');
    fputs("\"terrain\":", stdout);
    printf("%u", (unsigned)v->terrain);
    putchar(',');
    fputs("\"called_move_depth\":", stdout);
    printf("%u", (unsigned)v->called_move_depth);
    putchar(',');
    fputs("\"follow_me_target\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 2; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->follow_me_target[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"follow_me_turns\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 2; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->follow_me_turns[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"wish_turns\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->wish_turns[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"wish_amount\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->wish_amount[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"future_turns\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->future_turns[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"future_attacker\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->future_attacker[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"pursuit_boost\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->pursuit_boost[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"future_move\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->future_move[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"future_damage\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->future_damage[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"turn_number\":", stdout);
    printf("%lu", (unsigned long)v->turn_number);
    putchar(',');
    fputs("\"run_tries\":", stdout);
    printf("%u", (unsigned)v->run_tries);
    putchar(',');
    fputs("\"pyramid_run_multiplier\":", stdout);
    printf("%u", (unsigned)v->pyramid_run_multiplier);
    putchar(',');
    fputs("\"outcome\":", stdout);
    printf("%u", (unsigned)v->outcome);
    putchar(',');
    fputs("\"ended\":", stdout);
    printf("%u", (unsigned)v->ended);
    putchar(',');
    fputs("\"caught_valid\":", stdout);
    printf("%u", (unsigned)v->caught_valid);
    putchar(',');
    fputs("\"caught_pokemon\":", stdout);
    if (!write_pokemon(&v->caught_pokemon)) return 0;
    putchar(',');
    fputs("\"money_reward\":", stdout);
    printf("%lu", (unsigned long)v->money_reward);
    putchar(',');
    fputs("\"last_exp_gain\":", stdout);
    printf("%lu", (unsigned long)v->last_exp_gain);
    putchar(',');
    fputs("\"money_multiplier\":", stdout);
    printf("%u", (unsigned)v->money_multiplier);
    putchar(',');
    fputs("\"leveled_up_party_mask\":", stdout);
    printf("%u", (unsigned)v->leveled_up_party_mask);
    putchar(',');
    fputs("\"opponent_trainer_id\":", stdout);
    printf("%u", (unsigned)v->opponent_trainer_id);
    putchar(',');
    fputs("\"opponent_trainer_items\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < 4; ++i0) {
        if (i0) putchar(',');
        printf("%u", (unsigned)v->opponent_trainer_items[i0]);
    }
    putchar(']');
    putchar(',');
    fputs("\"opponent_trainer_ai_flags\":", stdout);
    printf("%lu", (unsigned long)v->opponent_trainer_ai_flags);
    putchar(',');
    fputs("\"events\":", stdout);
    putchar('[');
    for (size_t i0 = 0; i0 < v->event_count; ++i0) {
        if (i0) putchar(',');
        if (!write_RemasterEmeraldBattleEvent(&v->events[i0])) return 0;
    }
    putchar(']');
    putchar(',');
    fputs("\"event_count\":", stdout);
    printf("%llu", (unsigned long long)v->event_count);
    putchar('}');
    return !ferror(stdout);
}
static int make_mon(RemasterEmeraldPartyPokemon *mon, uint16_t species, uint8_t level, uint16_t move_id)
{
    const RemasterEmeraldSpeciesInfo *info = remaster_emerald_species_info(species);
    const RemasterEmeraldMoveInfo *move = remaster_emerald_move_info(move_id);
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[6] = {31,31,31,31,31,31}, evs[6] = {0};
    size_t i;
    if (info == NULL || move == NULL) return 0;
    memset(mon, 0, sizeof(*mon));
    mon->box.personality = UINT32_C(0x10203040) + (uint32_t)species * 17u + level;
    mon->box.ot_id = UINT32_C(0x55667788);
    mon->box.header_flags = 2;
    if (!remaster_emerald_box_pokemon_set_species(&mon->box, species)
        || !remaster_emerald_box_pokemon_set_experience(&mon->box,
            remaster_emerald_experience_for_level(info->growth_rate, level))
        || !remaster_emerald_box_pokemon_set_move(&mon->box, 0, move_id, move->pp)) return 0;
    for (i = 0; i < 6; ++i)
        if (!remaster_emerald_box_pokemon_set_iv(&mon->box, i, ivs[i])) return 0;
    if (!remaster_emerald_calculate_stats(species, level,
        remaster_emerald_box_pokemon_nature(&mon->box), ivs, evs, &stats)) return 0;
    mon->level = level;
    mon->hp = mon->max_hp = stats.hp;
    mon->attack = stats.attack; mon->defense = stats.defense;
    mon->speed = stats.speed; mon->sp_attack = stats.sp_attack; mon->sp_defense = stats.sp_defense;
    mon->box.checksum = remaster_emerald_box_pokemon_checksum(&mon->box);
    return 1;
}

static int snapshot(const RemasterEmeraldSave *save, const RemasterEmeraldBattleState *battle,
    int attached, int committed, int whiteout, int qol_result)
{
    RemasterEmeraldOverworldState world;
    RemasterEmeraldPartyPokemon player, reserve;
    int valid = 0, defeated = 0;
    if (!remaster_emerald_overworld_get(save, &world)
        || !remaster_emerald_party_get(save, 0, &player, &valid) || !valid) return 0;
    memset(&reserve, 0, sizeof(reserve));
    if (remaster_emerald_party_count(save) > 1
        && (!remaster_emerald_party_get(save, 1, &reserve, &valid) || !valid)) return 0;
    if (!remaster_emerald_flag_get(save, 0x501, &defeated)) return 0;
    fputs("{\"version\":1,\"format\":\"vanillaplus\",\"domains\":{\"save_block2\":\"", stdout);
    hex(save->save_block2, sizeof(save->save_block2));
    fputs("\",\"save_block1\":\"", stdout);
    hex(save->save_block1, sizeof(save->save_block1));
    fputs("\",\"storage\":\"", stdout);
    hex(save->pokemon_storage, sizeof(save->pokemon_storage));
    fputs("\",\"script\":null,\"objects\":null,\"encounter\":null,\"battle\":", stdout);
    if (attached) {
        if (!write_RemasterEmeraldBattleState(battle)) return 0;
    } else fputs("null", stdout);
    fputs("},\"observations\":{", stdout);
    printf("\"attached\":%d,\"ended\":%u,\"outcome\":%u,\"turn\":%lu,\"rng_state\":%lu,\"rng_calls\":%llu,",
        attached, attached ? (unsigned)battle->ended : 0u, attached ? (unsigned)battle->outcome : 0u,
        attached ? (unsigned long)battle->turn_number : 0ul, attached ? (unsigned long)battle->rng.state : 0ul,
        attached ? (unsigned long long)battle->rng.calls : 0ull);
    printf("\"event_count\":%llu,\"money_multiplier\":%u,\"player_hp\":%u,\"foe_hp\":%u,\"foe_slot\":%u,",
        attached ? (unsigned long long)battle->event_count : 0ull, attached ? (unsigned)battle->money_multiplier : 0u,
        attached ? (unsigned)battle->battlers[0].pokemon.hp : 0u,
        attached ? (unsigned)battle->battlers[1].pokemon.hp : 0u,
        attached ? (unsigned)battle->battlers[1].party_slot : 0u);
    printf("\"money\":%lu,\"trainer_defeated\":%d,\"held0\":%u,\"held1\":%u,\"save_hp\":%u,\"save_level\":%u,",
        (unsigned long)world.money, !!defeated,
        (unsigned)remaster_emerald_box_pokemon_held_item(&player.box),
        (unsigned)remaster_emerald_box_pokemon_held_item(&reserve.box),
        (unsigned)player.hp, (unsigned)player.level);
    printf("\"whiteout\":%d,\"qol_result\":%d,\"committed\":%d}}\n", whiteout, qol_result, committed);
    return !ferror(stdout);
}

int main(int argc, char **argv)
{
    RemasterEmeraldSave save;
    RemasterEmeraldBattleState battle;
    RemasterEmeraldPartyPokemon player, reserve, foes[2];
    RemasterEmeraldOverworldState world;
    uint32_t seed, flags = REMASTER_EMERALD_BATTLE_TYPE_MASTER;
    unsigned recipe, foe_count = 1, index = 0;
    int attached = 0, committed = 0, whiteout = 0, qol_result = -1, runtime_noise = 0;
    char line[512];
    if ((argc != 3 && argc != 4) || !number(argv[2], &seed)) return 1;
    if (strcmp(argv[1], "wild-win-v1") == 0) recipe = 0;
    else if (strcmp(argv[1], "trainer-replacement-v1") == 0) recipe = 1;
    else if (strcmp(argv[1], "trainer-loss-v1") == 0) recipe = 2;
    else if (strcmp(argv[1], "qol-held-reward-v1") == 0) recipe = 3;
    else return 1;
    memset(&save, 0, sizeof(save)); memset(&battle, 0, sizeof(battle));
    memset(foes, 0, sizeof(foes)); memset(&reserve, 0, sizeof(reserve));
    if (!make_mon(&player, recipe == 0 ? 4 : 1, recipe == 2 ? 5 : recipe == 0 ? 20 : 60,
        recipe == 2 ? 33 : 98)) return 1;
    if (recipe == 0) {
        if (!make_mon(&foes[0], 129, 5, 33)) return 1;
        foes[0].hp = 1;
    } else {
        flags |= REMASTER_EMERALD_BATTLE_TYPE_TRAINER;
        if (!make_mon(&foes[0], 4, recipe == 2 ? 60 : 5, recipe == 2 ? 98 : 33)) return 1;
        if (recipe == 2) player.hp = 1;
        else {
            foe_count = 2; foes[0].hp = 1;
            if (!make_mon(&foes[1], 7, 10, 33)) return 1;
            foes[1].hp = 1;
        }
    }
    if (!remaster_emerald_party_set_count(&save, recipe == 3 ? 2 : 1)
        || !remaster_emerald_party_set(&save, 0, &player)) return 1;
    if (recipe == 3) {
        if (!make_mon(&reserve, 4, 60, 98)
            || !remaster_emerald_box_pokemon_set_held_item(&reserve.box, 189)
            || !remaster_emerald_party_set(&save, 1, &reserve)) return 1;
    }
    if (!remaster_emerald_overworld_get(&save, &world)) return 1;
    world.money = 1000;
    if (!remaster_emerald_overworld_set(&save, &world)) return 1;
    if (argc == 4) {
        if (strcmp(argv[3], "--transport-noise") == 0) {
            save.counter = 777; save.selected_slot = 1; save.last_written_sector = 13;
        } else if (strcmp(argv[3], "--runtime-noise") == 0) runtime_noise = 1;
        else return 1;
    }
    if (!snapshot(&save, &battle, attached, committed, whiteout, qol_result)) return 1;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        char *tokens[24], *token;
        size_t count = 0;
        if (++index > 4096 || strchr(line, '\n') == NULL) goto rejected;
        token = strtok(line, " \t\r\n");
        while (token != NULL) {
            if (count >= sizeof(tokens) / sizeof(tokens[0])) goto rejected;
            tokens[count++] = token; token = strtok(NULL, " \t\r\n");
        }
        if (!count) goto rejected;
        if (strcmp(tokens[0], "qol_swap_held") == 0 && count == 3 && recipe == 3 && !attached) {
            uint32_t a, b;
            int valid_a = 0, valid_b = 0;
            RemasterEmeraldPartyPokemon first, second;
            if (!number(tokens[1], &a) || !number(tokens[2], &b) || a == b
                || a >= remaster_emerald_party_count(&save) || b >= remaster_emerald_party_count(&save)
                || !remaster_emerald_party_get(&save, a, &first, &valid_a) || !valid_a
                || !remaster_emerald_party_get(&save, b, &second, &valid_b) || !valid_b) goto rejected;
            qol_result = !!remaster_emerald_qol_swap_held_items(&first.box, &second.box);
            if (qol_result && (!remaster_emerald_party_set(&save, a, &first)
                || !remaster_emerald_party_set(&save, b, &second))) goto rejected;
        } else if (strcmp(tokens[0], "battle_start") == 0 && count == 1 && !attached) {
            remaster_emerald_battle_state_init(&battle, flags, seed);
            if (!remaster_emerald_battle_start_from_save(&battle, &save, foes, (uint8_t)foe_count)) goto rejected;
            if (recipe != 0) {
                battle.opponent_trainer_id = 1;
                battle.opponent_trainer_ai_flags = recipe == 2 ? 0 :
                    REMASTER_EMERALD_AI_CHECK_BAD_MOVE | REMASTER_EMERALD_AI_TRY_TO_FAINT | REMASTER_EMERALD_AI_CHECK_VIABILITY;
            }
            if (runtime_noise) battle.future_damage[3] ^= 1;
            attached = 1;
        } else if (strcmp(tokens[0], "battle_turn") == 0 && count == 21 && attached && !battle.ended) {
            RemasterEmeraldBattleAction actions[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
            size_t i, j;
            for (i = 0; i < 4; ++i) {
                uint32_t values[5];
                const uint32_t max[5] = {4,3,3,5,65535};
                for (j = 0; j < 5; ++j)
                    if (!number(tokens[1 + i * 5 + j], &values[j]) || values[j] > max[j]) goto rejected;
                if (values[0] == 0 && (values[1] || values[2] || values[3] || values[4])) goto rejected;
                actions[i].kind = (uint8_t)values[0]; actions[i].move_slot = (uint8_t)values[1];
                actions[i].target = (uint8_t)values[2]; actions[i].party_slot = (uint8_t)values[3];
                actions[i].item_id = (uint16_t)values[4];
            }
            if (!remaster_emerald_battle_resolve_turn(&battle, actions)) goto rejected;
        } else if (strcmp(tokens[0], "battle_commit") == 0 && count == 1 && attached && battle.ended && !committed) {
            if (recipe == 0) {
                if (!remaster_emerald_battle_commit_player_party(&battle, &save)) goto rejected;
            } else if (!remaster_emerald_battle_finalize_trainer(&battle, &save, &whiteout)) goto rejected;
            committed = 1;
        } else goto rejected;
        if (!snapshot(&save, &battle, attached, committed, whiteout, qol_result)) goto rejected;
    }
    return ferror(stdin) || ferror(stdout) || index == 0 ? 1 : 0;
rejected:
    fprintf(stderr, "rejected battle command at index %u\n", index);
    return 1;
}
