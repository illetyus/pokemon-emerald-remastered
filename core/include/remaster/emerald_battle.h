#ifndef REMASTER_EMERALD_BATTLE_H
#define REMASTER_EMERALD_BATTLE_H

#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_BATTLE_MAX_BATTLERS = 4,
    REMASTER_EMERALD_BATTLE_PARTY_SIZE = 6,
    REMASTER_EMERALD_BATTLE_STAT_COUNT = 8,
    REMASTER_EMERALD_BATTLE_EVENT_CAPACITY = 256
};

enum {
    REMASTER_EMERALD_BATTLE_TYPE_DOUBLE = 1u << 0,
    REMASTER_EMERALD_BATTLE_TYPE_LINK = 1u << 1,
    REMASTER_EMERALD_BATTLE_TYPE_MASTER = 1u << 2,
    REMASTER_EMERALD_BATTLE_TYPE_TRAINER = 1u << 3,
    REMASTER_EMERALD_BATTLE_TYPE_FIRST = 1u << 4,
    REMASTER_EMERALD_BATTLE_TYPE_MULTI = 1u << 6,
    REMASTER_EMERALD_BATTLE_TYPE_SAFARI = 1u << 7,
    REMASTER_EMERALD_BATTLE_TYPE_BATTLE_TOWER = 1u << 8,
    REMASTER_EMERALD_BATTLE_TYPE_WALLY = 1u << 9,
    REMASTER_EMERALD_BATTLE_TYPE_ROAMER = 1u << 10,
    REMASTER_EMERALD_BATTLE_TYPE_EREADER_TRAINER = 1u << 11,
    REMASTER_EMERALD_BATTLE_TYPE_LEGENDARY = 1u << 13,
    REMASTER_EMERALD_BATTLE_TYPE_DOME = 1u << 16,
    REMASTER_EMERALD_BATTLE_TYPE_PALACE = 1u << 17,
    REMASTER_EMERALD_BATTLE_TYPE_ARENA = 1u << 18,
    REMASTER_EMERALD_BATTLE_TYPE_FACTORY = 1u << 19,
    REMASTER_EMERALD_BATTLE_TYPE_PIKE = 1u << 20,
    REMASTER_EMERALD_BATTLE_TYPE_PYRAMID = 1u << 21,
    REMASTER_EMERALD_BATTLE_TYPE_INGAME_PARTNER = 1u << 22,
    REMASTER_EMERALD_BATTLE_TYPE_RECORDED_LINK = 1u << 25,
    REMASTER_EMERALD_BATTLE_TYPE_SECRET_BASE = 1u << 27
};

enum {
    REMASTER_EMERALD_STATUS1_SLEEP = 0x00000007u,
    REMASTER_EMERALD_STATUS1_POISON = 0x00000008u,
    REMASTER_EMERALD_STATUS1_BURN = 0x00000010u,
    REMASTER_EMERALD_STATUS1_FREEZE = 0x00000020u,
    REMASTER_EMERALD_STATUS1_PARALYSIS = 0x00000040u,
    REMASTER_EMERALD_STATUS1_TOXIC = 0x00000080u,
    REMASTER_EMERALD_STATUS1_TOXIC_COUNTER = 0x00000F00u
};

#define REMASTER_EMERALD_STATUS2_CONFUSION UINT32_C(0x00000007)
#define REMASTER_EMERALD_STATUS2_FLINCHED UINT32_C(0x00000008)
#define REMASTER_EMERALD_STATUS2_UPROAR UINT32_C(0x00000070)
#define REMASTER_EMERALD_STATUS2_BIDE UINT32_C(0x00000300)
#define REMASTER_EMERALD_STATUS2_LOCK_CONFUSE UINT32_C(0x00000C00)
#define REMASTER_EMERALD_STATUS2_MULTIPLETURNS UINT32_C(0x00001000)
#define REMASTER_EMERALD_STATUS2_WRAPPED UINT32_C(0x0000E000)
#define REMASTER_EMERALD_STATUS2_INFATUATION UINT32_C(0x000F0000)
#define REMASTER_EMERALD_STATUS2_FOCUS_ENERGY UINT32_C(0x00100000)
#define REMASTER_EMERALD_STATUS2_TRANSFORMED UINT32_C(0x00200000)
#define REMASTER_EMERALD_STATUS2_RECHARGE UINT32_C(0x00400000)
#define REMASTER_EMERALD_STATUS2_RAGE UINT32_C(0x00800000)
#define REMASTER_EMERALD_STATUS2_SUBSTITUTE UINT32_C(0x01000000)
#define REMASTER_EMERALD_STATUS2_DESTINY_BOND UINT32_C(0x02000000)
#define REMASTER_EMERALD_STATUS2_ESCAPE_PREVENTION UINT32_C(0x04000000)
#define REMASTER_EMERALD_STATUS2_NIGHTMARE UINT32_C(0x08000000)
#define REMASTER_EMERALD_STATUS2_CURSED UINT32_C(0x10000000)
#define REMASTER_EMERALD_STATUS2_FORESIGHT UINT32_C(0x20000000)
#define REMASTER_EMERALD_STATUS2_DEFENSE_CURL UINT32_C(0x40000000)
#define REMASTER_EMERALD_STATUS2_TORMENT UINT32_C(0x80000000)

enum {
    REMASTER_EMERALD_STATUS3_LEECH_SEED = 1u << 2,
    REMASTER_EMERALD_STATUS3_ALWAYS_HITS = (1u << 3) | (1u << 4),
    REMASTER_EMERALD_STATUS3_PERISH_SONG = 1u << 5,
    REMASTER_EMERALD_STATUS3_ON_AIR = 1u << 6,
    REMASTER_EMERALD_STATUS3_UNDERGROUND = 1u << 7,
    REMASTER_EMERALD_STATUS3_MINIMIZED = 1u << 8,
    REMASTER_EMERALD_STATUS3_CHARGED_UP = 1u << 9,
    REMASTER_EMERALD_STATUS3_ROOTED = 1u << 10,
    REMASTER_EMERALD_STATUS3_YAWN = (1u << 11) | (1u << 12),
    REMASTER_EMERALD_STATUS3_IMPRISONED_OTHERS = 1u << 13,
    REMASTER_EMERALD_STATUS3_GRUDGE = 1u << 14,
    REMASTER_EMERALD_STATUS3_CANT_SCORE_CRIT = 1u << 15,
    REMASTER_EMERALD_STATUS3_MUD_SPORT = 1u << 16,
    REMASTER_EMERALD_STATUS3_WATER_SPORT = 1u << 17,
    REMASTER_EMERALD_STATUS3_UNDERWATER = 1u << 18
};

enum {
    REMASTER_EMERALD_SIDE_REFLECT = 1u << 0,
    REMASTER_EMERALD_SIDE_LIGHT_SCREEN = 1u << 1,
    REMASTER_EMERALD_SIDE_SPIKES = 1u << 4,
    REMASTER_EMERALD_SIDE_SAFEGUARD = 1u << 5,
    REMASTER_EMERALD_SIDE_MIST = 1u << 8
};

typedef enum RemasterEmeraldBattleWeather {
    REMASTER_EMERALD_BATTLE_WEATHER_NONE = 0,
    REMASTER_EMERALD_BATTLE_WEATHER_RAIN = 1,
    REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM = 2,
    REMASTER_EMERALD_BATTLE_WEATHER_SUN = 3,
    REMASTER_EMERALD_BATTLE_WEATHER_HAIL = 4
} RemasterEmeraldBattleWeather;

typedef enum RemasterEmeraldBattleOutcome {
    REMASTER_EMERALD_BATTLE_OUTCOME_NONE = 0,
    REMASTER_EMERALD_BATTLE_OUTCOME_WON = 1,
    REMASTER_EMERALD_BATTLE_OUTCOME_LOST = 2,
    REMASTER_EMERALD_BATTLE_OUTCOME_DREW = 3,
    REMASTER_EMERALD_BATTLE_OUTCOME_RAN = 4,
    REMASTER_EMERALD_BATTLE_OUTCOME_PLAYER_TELEPORTED = 5,
    REMASTER_EMERALD_BATTLE_OUTCOME_MON_FLED = 6,
    REMASTER_EMERALD_BATTLE_OUTCOME_CAUGHT = 7,
    REMASTER_EMERALD_BATTLE_OUTCOME_FORFEITED = 9
} RemasterEmeraldBattleOutcome;

typedef enum RemasterEmeraldBattleActionKind {
    REMASTER_EMERALD_BATTLE_ACTION_NONE = 0,
    REMASTER_EMERALD_BATTLE_ACTION_MOVE = 1,
    REMASTER_EMERALD_BATTLE_ACTION_SWITCH = 2,
    REMASTER_EMERALD_BATTLE_ACTION_ITEM = 3,
    REMASTER_EMERALD_BATTLE_ACTION_RUN = 4
} RemasterEmeraldBattleActionKind;

typedef enum RemasterEmeraldBattleEventKind {
    REMASTER_EMERALD_BATTLE_EVENT_NONE = 0,
    REMASTER_EMERALD_BATTLE_EVENT_STARTED = 1,
    REMASTER_EMERALD_BATTLE_EVENT_TURN_STARTED = 2,
    REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED = 3,
    REMASTER_EMERALD_BATTLE_EVENT_MOVE_MISSED = 4,
    REMASTER_EMERALD_BATTLE_EVENT_DAMAGE = 5,
    REMASTER_EMERALD_BATTLE_EVENT_CRITICAL = 6,
    REMASTER_EMERALD_BATTLE_EVENT_EFFECTIVENESS = 7,
    REMASTER_EMERALD_BATTLE_EVENT_STATUS = 8,
    REMASTER_EMERALD_BATTLE_EVENT_STAT_STAGE = 9,
    REMASTER_EMERALD_BATTLE_EVENT_WEATHER = 10,
    REMASTER_EMERALD_BATTLE_EVENT_ABILITY = 11,
    REMASTER_EMERALD_BATTLE_EVENT_ITEM = 12,
    REMASTER_EMERALD_BATTLE_EVENT_SWITCH = 13,
    REMASTER_EMERALD_BATTLE_EVENT_FAINT = 14,
    REMASTER_EMERALD_BATTLE_EVENT_EXP = 15,
    REMASTER_EMERALD_BATTLE_EVENT_LEVEL_UP = 16,
    REMASTER_EMERALD_BATTLE_EVENT_CAPTURE_SHAKE = 17,
    REMASTER_EMERALD_BATTLE_EVENT_CAPTURED = 18,
    REMASTER_EMERALD_BATTLE_EVENT_RUN = 19,
    REMASTER_EMERALD_BATTLE_EVENT_ENDED = 20,
    REMASTER_EMERALD_BATTLE_EVENT_HEAL = 21,
    REMASTER_EMERALD_BATTLE_EVENT_PP = 22,
    REMASTER_EMERALD_BATTLE_EVENT_MESSAGE = 23
} RemasterEmeraldBattleEventKind;

typedef struct RemasterEmeraldBattleRng {
    uint32_t state;
    uint64_t calls;
} RemasterEmeraldBattleRng;

typedef struct RemasterEmeraldBattleEvent {
    uint16_t kind;
    uint8_t battler;
    uint8_t target;
    uint16_t move_id;
    int32_t value;
    uint32_t aux;
} RemasterEmeraldBattleEvent;

typedef struct RemasterEmeraldBattleAction {
    uint8_t kind;
    uint8_t move_slot;
    uint8_t target;
    uint8_t party_slot;
    uint16_t item_id;
} RemasterEmeraldBattleAction;

typedef struct RemasterEmeraldBattleMon {
    RemasterEmeraldPartyPokemon pokemon;
    uint16_t species;
    uint16_t held_item;
    uint16_t moves[4];
    uint8_t pp[4];
    uint8_t types[2];
    uint8_t ability;
    uint8_t stat_stages[REMASTER_EMERALD_BATTLE_STAT_COUNT];
    uint32_t status2;
    uint32_t status3;
    uint16_t substitute_hp;
    uint16_t last_move;
    uint16_t last_taken_move;
    uint16_t choice_locked_move;
    uint16_t last_damage;
    uint16_t bide_damage;
    uint16_t trapped_move;
    uint8_t last_damage_from;
    uint8_t last_damage_type;
    uint8_t last_damage_was_physical;
    uint8_t last_damage_turn;
    uint8_t side;
    uint8_t party_slot;
    uint8_t active;
    uint8_t fainted;
    uint8_t protected_turn;
    uint8_t endure_turn;
    uint8_t entered_turn;
    uint8_t protect_chain;
    uint8_t stockpile;
    uint8_t rollout_count;
    uint8_t fury_cutter_count;
    uint8_t trapped_turns;
    uint8_t rampage_turns;
    uint8_t bide_turns;
    uint8_t lock_on_turns;
    uint8_t lock_on_target;
    uint8_t perish_count;
    uint8_t toxic_counter;
    uint8_t flash_fire;
    uint8_t taunt_turns;
    uint8_t encore_turns;
    uint8_t encore_move_slot;
    uint8_t disable_turns;
    uint8_t disable_move_slot;
    uint8_t destiny_bond_turn;
    uint8_t grudge_turn;
    uint16_t charging_move;
    uint8_t charging_move_slot;
    uint8_t charging_target;
    uint8_t helping_hand;
    uint8_t magic_coat;
    uint8_t snatch;
    uint8_t imprison;
    uint16_t last_consumed_item;
} RemasterEmeraldBattleMon;

typedef struct RemasterEmeraldBattleState {
    uint32_t battle_type_flags;
    RemasterEmeraldBattleRng rng;
    RemasterEmeraldBattleMon battlers[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];

    RemasterEmeraldPartyPokemon parties[2][REMASTER_EMERALD_BATTLE_PARTY_SIZE];
    uint8_t party_count[2];
    uint8_t active_party_slot[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];

    uint32_t side_status[2];
    uint8_t spikes_layers[2];
    uint8_t weather;
    uint8_t weather_turns;
    uint8_t player_badge_mask;
    uint8_t terrain;
    uint8_t called_move_depth;
    uint8_t follow_me_target[2];
    uint8_t follow_me_turns[2];
    uint8_t wish_turns[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint16_t wish_amount[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint8_t future_turns[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint8_t future_attacker[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint8_t pursuit_boost[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint16_t future_move[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint16_t future_damage[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint32_t turn_number;
    uint8_t outcome;
    uint8_t ended;
    uint8_t caught_valid;
    RemasterEmeraldPartyPokemon caught_pokemon;

    uint32_t money_reward;
    uint32_t last_exp_gain;

    RemasterEmeraldBattleEvent events[REMASTER_EMERALD_BATTLE_EVENT_CAPACITY];
    size_t event_count;
} RemasterEmeraldBattleState;

typedef struct RemasterEmeraldBattleDamageResult {
    uint16_t damage;
    uint8_t hit;
    uint8_t critical;
    uint8_t effectiveness_tenths;
    uint8_t immune;
} RemasterEmeraldBattleDamageResult;

void remaster_emerald_battle_rng_seed(
    RemasterEmeraldBattleRng *rng,
    uint32_t seed);
uint16_t remaster_emerald_battle_random(
    RemasterEmeraldBattleRng *rng);
uint32_t remaster_emerald_battle_random32(
    RemasterEmeraldBattleRng *rng);

void remaster_emerald_battle_state_init(
    RemasterEmeraldBattleState *battle,
    uint32_t battle_type_flags,
    uint32_t seed);

int remaster_emerald_battle_start(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldPartyPokemon *player_party,
    uint8_t player_count,
    const RemasterEmeraldPartyPokemon *opponent_party,
    uint8_t opponent_count);

int remaster_emerald_battle_start_from_save(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldSave *save,
    const RemasterEmeraldPartyPokemon *opponent_party,
    uint8_t opponent_count);

int remaster_emerald_battle_calculate_damage(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker,
    uint8_t defender,
    uint16_t move_id,
    RemasterEmeraldBattleDamageResult *out_result);

int remaster_emerald_battle_use_move(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker,
    uint8_t target,
    uint8_t move_slot);

int remaster_emerald_battle_switch(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t party_slot);

int remaster_emerald_battle_choose_ai_action(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    RemasterEmeraldBattleAction *out_action);

int remaster_emerald_battle_resolve_turn(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS]);

int remaster_emerald_battle_try_run(
    RemasterEmeraldBattleState *battle,
    uint8_t battler);

int remaster_emerald_battle_throw_ball(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    uint16_t ball_item_id);

uint32_t remaster_emerald_battle_exp_gain(
    const RemasterEmeraldBattleMon *defeated,
    uint8_t participant_count,
    int trainer_battle);

int remaster_emerald_battle_commit_player_party(
    const RemasterEmeraldBattleState *battle,
    RemasterEmeraldSave *save);

int remaster_emerald_battle_store_caught(
    const RemasterEmeraldBattleState *battle,
    RemasterEmeraldSave *save,
    size_t *out_box,
    size_t *out_slot);

uint8_t remaster_emerald_battle_type_effectiveness(
    uint8_t attack_type,
    uint8_t defense_type);

void remaster_emerald_battle_clear_events(
    RemasterEmeraldBattleState *battle);

#ifdef __cplusplus
}
#endif

#endif
