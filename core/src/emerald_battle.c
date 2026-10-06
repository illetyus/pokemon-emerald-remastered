#include "remaster/emerald_battle.h"

#include "remaster/emerald_items.h"
#include "remaster/emerald_state.h"

#include <limits.h>
#include <string.h>

#include "emerald_trainer_catalog.inc"

enum {
    TYPE_NORMAL = 0,
    TYPE_FIGHTING = 1,
    TYPE_FLYING = 2,
    TYPE_POISON = 3,
    TYPE_GROUND = 4,
    TYPE_ROCK = 5,
    TYPE_BUG = 6,
    TYPE_GHOST = 7,
    TYPE_STEEL = 8,
    TYPE_MYSTERY = 9,
    TYPE_FIRE = 10,
    TYPE_WATER = 11,
    TYPE_GRASS = 12,
    TYPE_ELECTRIC = 13,
    TYPE_PSYCHIC = 14,
    TYPE_ICE = 15,
    TYPE_DRAGON = 16,
    TYPE_DARK = 17,

    ABILITY_DRIZZLE = 2,
    ABILITY_BATTLE_ARMOR = 4,
    ABILITY_STURDY = 5,
    ABILITY_DAMP = 6,
    ABILITY_LIMBER = 7,
    ABILITY_SAND_VEIL = 8,
    ABILITY_STATIC = 9,
    ABILITY_VOLT_ABSORB = 10,
    ABILITY_WATER_ABSORB = 11,
    ABILITY_OBLIVIOUS = 12,
    ABILITY_CLOUD_NINE = 13,
    ABILITY_COMPOUND_EYES = 14,
    ABILITY_INSOMNIA = 15,
    ABILITY_IMMUNITY = 17,
    ABILITY_FLASH_FIRE = 18,
    ABILITY_OWN_TEMPO = 20,
    ABILITY_SUCTION_CUPS = 21,
    ABILITY_INTIMIDATE = 22,
    ABILITY_SHADOW_TAG = 23,
    ABILITY_ROUGH_SKIN = 24,
    ABILITY_WONDER_GUARD = 25,
    ABILITY_LEVITATE = 26,
    ABILITY_EFFECT_SPORE = 27,
    ABILITY_CLEAR_BODY = 29,
    ABILITY_NATURAL_CURE = 30,
    ABILITY_SERENE_GRACE = 32,
    ABILITY_SWIFT_SWIM = 33,
    ABILITY_CHLOROPHYLL = 34,
    ABILITY_HUGE_POWER = 37,
    ABILITY_POISON_POINT = 38,
    ABILITY_INNER_FOCUS = 39,
    ABILITY_MAGMA_ARMOR = 40,
    ABILITY_WATER_VEIL = 41,
    ABILITY_MAGNET_PULL = 42,
    ABILITY_SOUNDPROOF = 43,
    ABILITY_RAIN_DISH = 44,
    ABILITY_SAND_STREAM = 45,
    ABILITY_PRESSURE = 46,
    ABILITY_THICK_FAT = 47,
    ABILITY_EARLY_BIRD = 48,
    ABILITY_FLAME_BODY = 49,
    ABILITY_HYPER_CUTTER = 52,
    ABILITY_TRUANT = 54,
    ABILITY_HUSTLE = 55,
    ABILITY_PLUS = 57,
    ABILITY_MINUS = 58,
    ABILITY_SHED_SKIN = 61,
    ABILITY_GUTS = 62,
    ABILITY_MARVEL_SCALE = 63,
    ABILITY_OVERGROW = 65,
    ABILITY_BLAZE = 66,
    ABILITY_TORRENT = 67,
    ABILITY_SWARM = 68,
    ABILITY_ROCK_HEAD = 69,
    ABILITY_DROUGHT = 70,
    ABILITY_ARENA_TRAP = 71,
    ABILITY_VITAL_SPIRIT = 72,
    ABILITY_WHITE_SMOKE = 73,
    ABILITY_PURE_POWER = 74,
    ABILITY_SHELL_ARMOR = 75,
    ABILITY_AIR_LOCK = 77,

    HOLD_EFFECT_RESTORE_HP = 1,
    HOLD_EFFECT_CURE_PAR = 2,
    HOLD_EFFECT_CURE_SLP = 3,
    HOLD_EFFECT_CURE_PSN = 4,
    HOLD_EFFECT_CURE_BRN = 5,
    HOLD_EFFECT_CURE_FRZ = 6,
    HOLD_EFFECT_CURE_STATUS = 9,
    HOLD_EFFECT_EVASION_UP = 22,
    HOLD_EFFECT_MACHO_BRACE = 24,
    HOLD_EFFECT_EXP_SHARE = 25,
    HOLD_EFFECT_QUICK_CLAW = 26,
    HOLD_EFFECT_CHOICE_BAND = 29,
    HOLD_EFFECT_BUG_POWER = 31,
    HOLD_EFFECT_DOUBLE_PRIZE = 32,
    HOLD_EFFECT_SOUL_DEW = 34,
    HOLD_EFFECT_DEEP_SEA_TOOTH = 35,
    HOLD_EFFECT_DEEP_SEA_SCALE = 36,
    HOLD_EFFECT_FOCUS_BAND = 39,
    HOLD_EFFECT_LUCKY_EGG = 40,
    HOLD_EFFECT_SCOPE_LENS = 41,
    HOLD_EFFECT_STEEL_POWER = 42,
    HOLD_EFFECT_LEFTOVERS = 43,
    HOLD_EFFECT_LIGHT_BALL = 45,
    HOLD_EFFECT_GROUND_POWER = 46,
    HOLD_EFFECT_ROCK_POWER = 47,
    HOLD_EFFECT_GRASS_POWER = 48,
    HOLD_EFFECT_DARK_POWER = 49,
    HOLD_EFFECT_FIGHTING_POWER = 50,
    HOLD_EFFECT_ELECTRIC_POWER = 51,
    HOLD_EFFECT_WATER_POWER = 52,
    HOLD_EFFECT_FLYING_POWER = 53,
    HOLD_EFFECT_POISON_POWER = 54,
    HOLD_EFFECT_ICE_POWER = 55,
    HOLD_EFFECT_GHOST_POWER = 56,
    HOLD_EFFECT_PSYCHIC_POWER = 57,
    HOLD_EFFECT_FIRE_POWER = 58,
    HOLD_EFFECT_DRAGON_POWER = 59,
    HOLD_EFFECT_NORMAL_POWER = 60,
    HOLD_EFFECT_SHELL_BELL = 62,
    HOLD_EFFECT_LUCKY_PUNCH = 63,
    HOLD_EFFECT_METAL_POWDER = 64,
    HOLD_EFFECT_THICK_CLUB = 65,
    HOLD_EFFECT_STICK = 66,

    EFFECT_HIT = 0,
    EFFECT_SLEEP = 1,
    EFFECT_POISON_HIT = 2,
    EFFECT_ABSORB = 3,
    EFFECT_BURN_HIT = 4,
    EFFECT_FREEZE_HIT = 5,
    EFFECT_PARALYZE_HIT = 6,
    EFFECT_EXPLOSION = 7,
    EFFECT_DREAM_EATER = 8,
    EFFECT_MIRROR_MOVE = 9,
    EFFECT_ATTACK_UP = 10,
    EFFECT_DEFENSE_UP = 11,
    EFFECT_SPEED_UP = 12,
    EFFECT_SPECIAL_ATTACK_UP = 13,
    EFFECT_SPECIAL_DEFENSE_UP = 14,
    EFFECT_ACCURACY_UP = 15,
    EFFECT_EVASION_UP = 16,
    EFFECT_ALWAYS_HIT = 17,
    EFFECT_ATTACK_DOWN = 18,
    EFFECT_DEFENSE_DOWN = 19,
    EFFECT_SPEED_DOWN = 20,
    EFFECT_SPECIAL_ATTACK_DOWN = 21,
    EFFECT_SPECIAL_DEFENSE_DOWN = 22,
    EFFECT_ACCURACY_DOWN = 23,
    EFFECT_EVASION_DOWN = 24,
    EFFECT_HAZE = 25,
    EFFECT_BIDE = 26,
    EFFECT_RAMPAGE = 27,
    EFFECT_ROAR = 28,
    EFFECT_MULTI_HIT = 29,
    EFFECT_CONVERSION = 30,
    EFFECT_FLINCH_HIT = 31,
    EFFECT_RESTORE_HP = 32,
    EFFECT_TOXIC = 33,
    EFFECT_PAY_DAY = 34,
    EFFECT_LIGHT_SCREEN = 35,
    EFFECT_TRI_ATTACK = 36,
    EFFECT_REST = 37,
    EFFECT_OHKO = 38,
    EFFECT_RAZOR_WIND = 39,
    EFFECT_SUPER_FANG = 40,
    EFFECT_DRAGON_RAGE = 41,
    EFFECT_TRAP = 42,
    EFFECT_HIGH_CRITICAL = 43,
    EFFECT_DOUBLE_HIT = 44,
    EFFECT_RECOIL_IF_MISS = 45,
    EFFECT_MIST = 46,
    EFFECT_FOCUS_ENERGY = 47,
    EFFECT_RECOIL = 48,
    EFFECT_CONFUSE = 49,
    EFFECT_ATTACK_UP_2 = 50,
    EFFECT_DEFENSE_UP_2 = 51,
    EFFECT_SPEED_UP_2 = 52,
    EFFECT_SPECIAL_ATTACK_UP_2 = 53,
    EFFECT_SPECIAL_DEFENSE_UP_2 = 54,
    EFFECT_ACCURACY_UP_2 = 55,
    EFFECT_EVASION_UP_2 = 56,
    EFFECT_TRANSFORM = 57,
    EFFECT_ATTACK_DOWN_2 = 58,
    EFFECT_DEFENSE_DOWN_2 = 59,
    EFFECT_SPEED_DOWN_2 = 60,
    EFFECT_SPECIAL_ATTACK_DOWN_2 = 61,
    EFFECT_SPECIAL_DEFENSE_DOWN_2 = 62,
    EFFECT_ACCURACY_DOWN_2 = 63,
    EFFECT_EVASION_DOWN_2 = 64,
    EFFECT_REFLECT = 65,
    EFFECT_POISON = 66,
    EFFECT_PARALYZE = 67,
    EFFECT_ATTACK_DOWN_HIT = 68,
    EFFECT_DEFENSE_DOWN_HIT = 69,
    EFFECT_SPEED_DOWN_HIT = 70,
    EFFECT_SPECIAL_ATTACK_DOWN_HIT = 71,
    EFFECT_SPECIAL_DEFENSE_DOWN_HIT = 72,
    EFFECT_ACCURACY_DOWN_HIT = 73,
    EFFECT_EVASION_DOWN_HIT = 74,
    EFFECT_SKY_ATTACK = 75,
    EFFECT_CONFUSE_HIT = 76,
    EFFECT_TWINEEDLE = 77,
    EFFECT_VITAL_THROW = 78,
    EFFECT_SUBSTITUTE = 79,
    EFFECT_RECHARGE = 80,
    EFFECT_RAGE = 81,
    EFFECT_MIMIC = 82,
    EFFECT_METRONOME = 83,
    EFFECT_LEECH_SEED = 84,
    EFFECT_SPLASH = 85,
    EFFECT_DISABLE = 86,
    EFFECT_LEVEL_DAMAGE = 87,
    EFFECT_PSYWAVE = 88,
    EFFECT_COUNTER = 89,
    EFFECT_ENCORE = 90,
    EFFECT_PAIN_SPLIT = 91,
    EFFECT_SNORE = 92,
    EFFECT_CONVERSION_2 = 93,
    EFFECT_LOCK_ON = 94,
    EFFECT_SKETCH = 95,
    EFFECT_SLEEP_TALK = 97,
    EFFECT_DESTINY_BOND = 98,
    EFFECT_FLAIL = 99,
    EFFECT_SPITE = 100,
    EFFECT_FALSE_SWIPE = 101,
    EFFECT_HEAL_BELL = 102,
    EFFECT_QUICK_ATTACK = 103,
    EFFECT_TRIPLE_KICK = 104,
    EFFECT_THIEF = 105,
    EFFECT_MEAN_LOOK = 106,
    EFFECT_NIGHTMARE = 107,
    EFFECT_MINIMIZE = 108,
    EFFECT_CURSE = 109,
    EFFECT_PROTECT = 111,
    EFFECT_SPIKES = 112,
    EFFECT_FORESIGHT = 113,
    EFFECT_PERISH_SONG = 114,
    EFFECT_SANDSTORM = 115,
    EFFECT_ENDURE = 116,
    EFFECT_ROLLOUT = 117,
    EFFECT_SWAGGER = 118,
    EFFECT_FURY_CUTTER = 119,
    EFFECT_ATTRACT = 120,
    EFFECT_RETURN = 121,
    EFFECT_PRESENT = 122,
    EFFECT_FRUSTRATION = 123,
    EFFECT_SAFEGUARD = 124,
    EFFECT_THAW_HIT = 125,
    EFFECT_MAGNITUDE = 126,
    EFFECT_BATON_PASS = 127,
    EFFECT_PURSUIT = 128,
    EFFECT_RAPID_SPIN = 129,
    EFFECT_SONICBOOM = 130,
    EFFECT_MORNING_SUN = 132,
    EFFECT_SYNTHESIS = 133,
    EFFECT_MOONLIGHT = 134,
    EFFECT_HIDDEN_POWER = 135,
    EFFECT_RAIN_DANCE = 136,
    EFFECT_SUNNY_DAY = 137,
    EFFECT_DEFENSE_UP_HIT = 138,
    EFFECT_ATTACK_UP_HIT = 139,
    EFFECT_ALL_STATS_UP_HIT = 140,
    EFFECT_BELLY_DRUM = 142,
    EFFECT_PSYCH_UP = 143,
    EFFECT_MIRROR_COAT = 144,
    EFFECT_SKULL_BASH = 145,
    EFFECT_TWISTER = 146,
    EFFECT_EARTHQUAKE = 147,
    EFFECT_FUTURE_SIGHT = 148,
    EFFECT_GUST = 149,
    EFFECT_FLINCH_MINIMIZE_HIT = 150,
    EFFECT_SOLAR_BEAM = 151,
    EFFECT_THUNDER = 152,
    EFFECT_TELEPORT = 153,
    EFFECT_BEAT_UP = 154,
    EFFECT_SEMI_INVULNERABLE = 155,
    EFFECT_DEFENSE_CURL = 156,
    EFFECT_SOFTBOILED = 157,
    EFFECT_FAKE_OUT = 158,
    EFFECT_UPROAR = 159,
    EFFECT_STOCKPILE = 160,
    EFFECT_SPIT_UP = 161,
    EFFECT_SWALLOW = 162,
    EFFECT_HAIL = 164,
    EFFECT_TORMENT = 165,
    EFFECT_FLATTER = 166,
    EFFECT_WILL_O_WISP = 167,
    EFFECT_MEMENTO = 168,
    EFFECT_FACADE = 169,
    EFFECT_FOCUS_PUNCH = 170,
    EFFECT_SMELLINGSALT = 171,
    EFFECT_FOLLOW_ME = 172,
    EFFECT_NATURE_POWER = 173,
    EFFECT_CHARGE = 174,
    EFFECT_TAUNT = 175,
    EFFECT_HELPING_HAND = 176,
    EFFECT_TRICK = 177,
    EFFECT_ROLE_PLAY = 178,
    EFFECT_WISH = 179,
    EFFECT_ASSIST = 180,
    EFFECT_INGRAIN = 181,
    EFFECT_SUPERPOWER = 182,
    EFFECT_MAGIC_COAT = 183,
    EFFECT_RECYCLE = 184,
    EFFECT_REVENGE = 185,
    EFFECT_BRICK_BREAK = 186,
    EFFECT_YAWN = 187,
    EFFECT_KNOCK_OFF = 188,
    EFFECT_ENDEAVOR = 189,
    EFFECT_ERUPTION = 190,
    EFFECT_SKILL_SWAP = 191,
    EFFECT_IMPRISON = 192,
    EFFECT_REFRESH = 193,
    EFFECT_GRUDGE = 194,
    EFFECT_SNATCH = 195,
    EFFECT_LOW_KICK = 196,
    EFFECT_SECRET_POWER = 197,
    EFFECT_DOUBLE_EDGE = 198,
    EFFECT_TEETER_DANCE = 199,
    EFFECT_BLAZE_KICK = 200,
    EFFECT_MUD_SPORT = 201,
    EFFECT_POISON_FANG = 202,
    EFFECT_WEATHER_BALL = 203,
    EFFECT_OVERHEAT = 204,
    EFFECT_TICKLE = 205,
    EFFECT_COSMIC_POWER = 206,
    EFFECT_SKY_UPPERCUT = 207,
    EFFECT_BULK_UP = 208,
    EFFECT_POISON_TAIL = 209,
    EFFECT_WATER_SPORT = 210,
    EFFECT_CALM_MIND = 211,
    EFFECT_DRAGON_DANCE = 212,
    EFFECT_CAMOUFLAGE = 213,

    ITEM_MASTER_BALL = 1,
    ITEM_ULTRA_BALL = 2,
    ITEM_GREAT_BALL = 3,
    ITEM_POKE_BALL = 4,
    ITEM_SAFARI_BALL = 5,
    ITEM_NET_BALL = 6,
    ITEM_DIVE_BALL = 7,
    ITEM_NEST_BALL = 8,
    ITEM_REPEAT_BALL = 9,
    ITEM_TIMER_BALL = 10,
    ITEM_LUXURY_BALL = 11,
    ITEM_PREMIER_BALL = 12,
    ITEM_POTION = 13,
    ITEM_ANTIDOTE = 14,
    ITEM_BURN_HEAL = 15,
    ITEM_ICE_HEAL = 16,
    ITEM_AWAKENING = 17,
    ITEM_PARALYZE_HEAL = 18,
    ITEM_FULL_RESTORE = 19,
    ITEM_MAX_POTION = 20,
    ITEM_HYPER_POTION = 21,
    ITEM_SUPER_POTION = 22,
    ITEM_FULL_HEAL = 23,
    ITEM_GUARD_SPEC = 73,
    ITEM_DIRE_HIT = 74,
    ITEM_X_ATTACK = 75,
    ITEM_X_DEFEND = 76,
    ITEM_X_SPEED = 77,
    ITEM_X_ACCURACY = 78,
    ITEM_X_SPECIAL = 79,

    SPECIES_PIKACHU = 25,
    SPECIES_FARFETCHD = 83,
    SPECIES_CUBONE = 104,
    SPECIES_MAROWAK = 105,
    SPECIES_CHANSEY = 113,
    SPECIES_DITTO = 132,
    SPECIES_CLAMPERL = 373,
    SPECIES_LATIAS = 407,
    SPECIES_LATIOS = 408,

    MOVE_SWORDS_DANCE = 14,
    MOVE_GUST = 16,
    MOVE_FLY = 19,
    MOVE_HYDRO_PUMP = 56,
    MOVE_SURF = 57,
    MOVE_BUBBLE_BEAM = 61,
    MOVE_COUNTER = 68,
    MOVE_RAZOR_LEAF = 75,
    MOVE_STUN_SPORE = 78,
    MOVE_EARTHQUAKE = 89,
    MOVE_DIG = 91,
    MOVE_MIMIC = 102,
    MOVE_METRONOME = 118,
    MOVE_MIRROR_MOVE = 119,
    MOVE_SWIFT = 129,
    MOVE_ROCK_SLIDE = 157,
    MOVE_STRUGGLE = 165,
    MOVE_SKETCH = 166,
    MOVE_THIEF = 168,
    MOVE_PROTECT = 182,
    MOVE_DESTINY_BOND = 194,
    MOVE_BATON_PASS = 226,
    MOVE_DETECT = 197,
    MOVE_ENDURE = 203,
    MOVE_SLEEP_TALK = 214,
    MOVE_MIRROR_COAT = 243,
    MOVE_SHADOW_BALL = 247,
    MOVE_WHIRLPOOL = 250,
    MOVE_UPROAR = 253,
    MOVE_FOCUS_PUNCH = 264,
    MOVE_FOLLOW_ME = 266,
    MOVE_HELPING_HAND = 270,
    MOVE_TRICK = 271,
    MOVE_ASSIST = 274,
    MOVE_SNATCH = 289,
    MOVE_DIVE = 291,
    MOVE_BOUNCE = 340,
    MOVE_COVET = 343,
    MOVE_CALM_MIND = 347,
    MOVE_DRAGON_DANCE = 349,

    FLAG_BADGE01_GET = 0x867
};

static const uint8_t kTypeChart[18][18] = {
    {10,10,10,10,10,5,10,0,5,10,10,10,10,10,10,10,10,10},
    {20,10,5,5,10,20,5,0,20,10,10,10,10,10,5,20,10,20},
    {10,20,10,10,10,5,20,10,5,10,10,10,20,5,10,10,10,10},
    {10,10,10,5,5,5,10,5,0,10,10,10,20,10,10,10,10,10},
    {10,10,0,20,10,20,5,10,20,10,20,10,5,20,10,10,10,10},
    {10,5,20,10,5,10,20,10,5,10,20,10,10,10,10,20,10,10},
    {10,5,5,5,10,10,10,5,5,10,5,10,20,10,20,10,10,20},
    {0,10,10,10,10,10,10,20,5,10,10,10,10,10,20,10,10,5},
    {10,10,10,10,10,20,10,10,5,10,5,5,10,5,10,20,10,10},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {10,10,10,10,10,5,20,10,20,10,5,5,20,10,10,20,5,10},
    {10,10,10,10,20,20,10,10,10,10,20,5,5,10,10,10,5,10},
    {10,10,5,5,20,20,5,10,5,10,5,20,5,10,10,10,5,10},
    {10,10,20,10,0,10,10,10,10,10,10,20,5,5,10,10,5,10},
    {10,20,10,20,10,10,10,20,5,10,10,10,10,10,5,10,10,0},
    {10,10,20,10,20,10,10,10,5,10,5,5,20,10,10,5,20,10},
    {10,10,10,10,10,10,10,10,5,10,10,10,10,10,10,10,20,10},
    {10,5,10,10,10,10,10,20,5,10,10,10,10,10,20,10,10,5}
};

static void battle_event(
    RemasterEmeraldBattleState *battle,
    uint16_t kind,
    uint8_t battler,
    uint8_t target,
    uint16_t move_id,
    int32_t value,
    uint32_t aux)
{
    RemasterEmeraldBattleEvent *event;

    if (battle == 0
        || battle->event_count >= REMASTER_EMERALD_BATTLE_EVENT_CAPACITY)
        return;

    event = &battle->events[battle->event_count++];
    event->kind = kind;
    event->battler = battler;
    event->target = target;
    event->move_id = move_id;
    event->value = value;
    event->aux = aux;
}

void remaster_emerald_battle_clear_events(
    RemasterEmeraldBattleState *battle)
{
    if (battle != 0)
        battle->event_count = 0;
}

void remaster_emerald_battle_rng_seed(
    RemasterEmeraldBattleRng *rng,
    uint32_t seed)
{
    if (rng == 0)
        return;
    rng->state = seed;
    rng->calls = 0;
}

uint16_t remaster_emerald_battle_random(
    RemasterEmeraldBattleRng *rng)
{
    if (rng == 0)
        return 0;

    rng->state = rng->state * UINT32_C(1103515245) + UINT32_C(24691);
    rng->calls++;
    return (uint16_t)(rng->state >> 16u);
}

uint32_t remaster_emerald_battle_random32(
    RemasterEmeraldBattleRng *rng)
{
    uint32_t hi = remaster_emerald_battle_random(rng);
    uint32_t lo = remaster_emerald_battle_random(rng);
    return (hi << 16u) | lo;
}

uint8_t remaster_emerald_battle_type_effectiveness(
    uint8_t attack_type,
    uint8_t defense_type)
{
    if (attack_type >= 18 || defense_type >= 18)
        return 10;
    return kTypeChart[attack_type][defense_type];
}

static uint8_t battle_side(uint8_t battler)
{
    return (uint8_t)(battler & 1u);
}

static uint8_t battle_partner(uint8_t battler)
{
    return (uint8_t)(battler ^ 2u);
}

static int battle_valid_battler(uint8_t battler)
{
    return battler < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
}

static const RemasterEmeraldItemInfo *battle_item(
    const RemasterEmeraldBattleMon *mon)
{
    return mon != 0
        ? remaster_emerald_item_info(mon->held_item)
        : 0;
}

static uint8_t battle_hold_effect(const RemasterEmeraldBattleMon *mon)
{
    const RemasterEmeraldItemInfo *item = battle_item(mon);
    return item != 0 ? item->hold_effect : 0;
}

static uint8_t battle_hold_param(const RemasterEmeraldBattleMon *mon)
{
    const RemasterEmeraldItemInfo *item = battle_item(mon);
    return item != 0 ? item->hold_effect_param : 0;
}

static int battle_load_mon(
    RemasterEmeraldBattleMon *out,
    const RemasterEmeraldPartyPokemon *pokemon,
    uint8_t side,
    uint8_t party_slot)
{
    const RemasterEmeraldSpeciesInfo *species;
    uint8_t ability_num;

    if (out == 0 || pokemon == 0)
        return 0;

    memset(out, 0, sizeof(*out));
    out->pokemon = *pokemon;
    out->species = remaster_emerald_box_pokemon_species(&pokemon->box);
    species = remaster_emerald_species_info(out->species);
    if (species == 0 || out->species == 0)
        return 0;

    out->held_item = remaster_emerald_box_pokemon_held_item(&pokemon->box);
    remaster_emerald_box_pokemon_moves(
        &pokemon->box,
        out->moves,
        out->pp);
    out->types[0] = species->type1;
    out->types[1] = species->type2;
    ability_num = remaster_emerald_box_pokemon_ability_num(&pokemon->box);
    out->ability = remaster_emerald_species_ability(out->species, ability_num);
    memset(out->stat_stages, 6, sizeof(out->stat_stages));
    out->side = side;
    out->party_slot = party_slot;
    out->active = 1;
    out->fainted = pokemon->hp == 0 ? 1u : 0u;
    out->protected_turn = 0xFFu;
    out->endure_turn = 0xFFu;
    out->entered_turn = 1;
    out->last_damage_from = 0xFFu;
    out->lock_on_target = 0xFFu;
    out->charging_target = 0xFFu;
    return 1;
}

static void battle_sync_battler(
    RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    RemasterEmeraldBattleMon *mon;

    if (battle == 0 || !battle_valid_battler(battler))
        return;

    mon = &battle->battlers[battler];
    if (!mon->active
        || mon->side > 1
        || mon->party_slot >= battle->party_count[mon->side])
        return;

    remaster_emerald_box_pokemon_set_held_item(
        &mon->pokemon.box,
        mon->held_item);
    mon->pokemon.box.checksum =
        remaster_emerald_box_pokemon_checksum(&mon->pokemon.box);
    battle->parties[mon->side][mon->party_slot] = mon->pokemon;
}

static int battle_party_has_alive(
    const RemasterEmeraldBattleState *battle,
    uint8_t side)
{
    uint8_t i;

    if (battle == 0 || side > 1)
        return 0;

    for (i = 0; i < battle->party_count[side]; ++i) {
        if (remaster_emerald_box_pokemon_species(
                &battle->parties[side][i].box) != 0
            && battle->parties[side][i].hp != 0)
            return 1;
    }
    return 0;
}

static int battle_party_slot_active(
    const RemasterEmeraldBattleState *battle,
    uint8_t side,
    uint8_t party_slot)
{
    uint8_t battler;

    for (battler = 0;
         battler < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
         ++battler) {
        if (battle->battlers[battler].active
            && battle->battlers[battler].side == side
            && battle->battlers[battler].party_slot == party_slot
            && !battle->battlers[battler].fainted)
            return 1;
    }
    return 0;
}
static int battle_find_switch_slot(
    const RemasterEmeraldBattleState *battle,
    uint8_t side,
    uint8_t *out_slot)
{
    uint8_t i;

    if (battle == 0 || out_slot == 0 || side > 1)
        return 0;

    for (i = 0; i < battle->party_count[side]; ++i) {
        if (battle->parties[side][i].hp != 0
            && remaster_emerald_box_pokemon_species(
                &battle->parties[side][i].box) != 0
            && !battle_party_slot_active(battle, side, i)) {
            *out_slot = i;
            return 1;
        }
    }
    return 0;
}


static int battle_weather_has_effect(
    const RemasterEmeraldBattleState *battle)
{
    uint8_t i;

    if (battle == 0)
        return 0;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (!battle->battlers[i].active || battle->battlers[i].fainted)
            continue;
        if (battle->battlers[i].ability == ABILITY_CLOUD_NINE
            || battle->battlers[i].ability == ABILITY_AIR_LOCK)
            return 0;
    }
    return 1;
}

static int battle_ability_on_field(
    const RemasterEmeraldBattleState *battle,
    uint8_t ability)
{
    uint8_t i;

    if (battle == 0)
        return 0;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (battle->battlers[i].active
            && !battle->battlers[i].fainted
            && battle->battlers[i].ability == ability)
            return 1;
    }
    return 0;
}

static int battle_frontier_type(uint32_t flags)
{
    return (flags
        & (REMASTER_EMERALD_BATTLE_TYPE_BATTLE_TOWER
           | REMASTER_EMERALD_BATTLE_TYPE_DOME
           | REMASTER_EMERALD_BATTLE_TYPE_PALACE
           | REMASTER_EMERALD_BATTLE_TYPE_ARENA
           | REMASTER_EMERALD_BATTLE_TYPE_FACTORY
           | REMASTER_EMERALD_BATTLE_TYPE_PIKE
           | REMASTER_EMERALD_BATTLE_TYPE_PYRAMID)) != 0;
}

static int battle_badge_boost_allowed(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t badge_index)
{
    if (battle == 0 || battler >= REMASTER_EMERALD_BATTLE_MAX_BATTLERS)
        return 0;
    if (battle->battlers[battler].side != 0)
        return 0;
    if (battle->battle_type_flags
        & (REMASTER_EMERALD_BATTLE_TYPE_LINK
           | REMASTER_EMERALD_BATTLE_TYPE_EREADER_TRAINER
           | REMASTER_EMERALD_BATTLE_TYPE_RECORDED_LINK
           | REMASTER_EMERALD_BATTLE_TYPE_SECRET_BASE))
        return 0;
    if (battle_frontier_type(battle->battle_type_flags))
        return 0;
    return (battle->player_badge_mask & (1u << badge_index)) != 0;
}

static int battle_change_stage(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t stat,
    int delta,
    uint16_t move_id)
{
    RemasterEmeraldBattleMon *mon;
    int value;

    if (battle == 0
        || !battle_valid_battler(battler)
        || stat >= REMASTER_EMERALD_BATTLE_STAT_COUNT)
        return 0;

    mon = &battle->battlers[battler];
    if (delta < 0
        && (mon->ability == ABILITY_CLEAR_BODY
            || mon->ability == ABILITY_WHITE_SMOKE
            || (stat == 1 && mon->ability == ABILITY_HYPER_CUTTER))) {
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_ABILITY,
            battler,
            battler,
            move_id,
            mon->ability,
            0);
        return 0;
    }

    value = (int)mon->stat_stages[stat] + delta;
    if (value < 0)
        value = 0;
    if (value > 12)
        value = 12;
    if (value == mon->stat_stages[stat])
        return 0;

    mon->stat_stages[stat] = (uint8_t)value;
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_STAT_STAGE,
        battler,
        battler,
        move_id,
        delta,
        stat);
    return 1;
}

static uint32_t battle_apply_stage(
    uint32_t value,
    uint8_t stage)
{
    static const uint8_t numerators[13] = {
        2, 2, 2, 2, 2, 2, 2, 3, 4, 5, 6, 7, 8
    };
    static const uint8_t denominators[13] = {
        8, 7, 6, 5, 4, 3, 2, 2, 2, 2, 2, 2, 2
    };

    if (stage > 12)
        stage = 12;

    value = value * numerators[stage] / denominators[stage];
    return value != 0 ? value : 1;
}

static uint32_t battle_speed(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    const RemasterEmeraldBattleMon *mon;
    uint32_t speed;

    if (battle == 0 || !battle_valid_battler(battler))
        return 0;

    mon = &battle->battlers[battler];
    speed = battle_apply_stage(mon->pokemon.speed, mon->stat_stages[3]);

    if (battle_badge_boost_allowed(battle, battler, 2))
        speed = speed * 110u / 100u;

    if (mon->pokemon.status & REMASTER_EMERALD_STATUS1_PARALYSIS)
        speed /= 4u;

    if (battle_weather_has_effect(battle)) {
        if (mon->ability == ABILITY_SWIFT_SWIM
            && battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_RAIN)
            speed *= 2u;
        if (mon->ability == ABILITY_CHLOROPHYLL
            && battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SUN)
            speed *= 2u;
    }

    if (battle_hold_effect(mon) == HOLD_EFFECT_MACHO_BRACE)
        speed /= 2u;

    return speed != 0 ? speed : 1;
}

static uint8_t battle_alive_on_side(
    const RemasterEmeraldBattleState *battle,
    uint8_t side)
{
    uint8_t i;
    uint8_t count = 0;

    if (battle == 0 || side > 1)
        return 0;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (battle->battlers[i].active
            && !battle->battlers[i].fainted
            && battle->battlers[i].side == side)
            count++;
    }
    return count;
}

static int battle_field_has_status3(
    const RemasterEmeraldBattleState *battle,
    uint32_t mask)
{
    uint8_t i;

    if (battle == 0)
        return 0;
    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (battle->battlers[i].active
            && !battle->battlers[i].fainted
            && (battle->battlers[i].status3 & mask))
            return 1;
    }
    return 0;
}

static int battle_effect_is_plain_hit(uint8_t effect)
{
    switch (effect) {
    case EFFECT_HIT:
    case EFFECT_VITAL_THROW:
    case EFFECT_QUICK_ATTACK:
    case EFFECT_EARTHQUAKE:
        return 1;
    default:
        return 0;
    }
}

static int battle_is_two_turn_effect(
    const RemasterEmeraldMoveInfo *move)
{
    if (move == 0)
        return 0;
    return move->effect == EFFECT_RAZOR_WIND
        || move->effect == EFFECT_SKY_ATTACK
        || move->effect == EFFECT_SKULL_BASH
        || move->effect == EFFECT_SOLAR_BEAM
        || move->effect == EFFECT_SEMI_INVULNERABLE;
}

static uint32_t battle_semi_invulnerable_flag(uint16_t move_id)
{
    if (move_id == MOVE_FLY || move_id == MOVE_BOUNCE)
        return REMASTER_EMERALD_STATUS3_ON_AIR;
    if (move_id == MOVE_DIG)
        return REMASTER_EMERALD_STATUS3_UNDERGROUND;
    if (move_id == MOVE_DIVE)
        return REMASTER_EMERALD_STATUS3_UNDERWATER;
    return 0;
}

static int battle_move_can_hit_semi_invulnerable(
    const RemasterEmeraldBattleMon *defender,
    uint16_t move_id,
    uint8_t effect)
{
    if (defender == 0)
        return 0;

    if (defender->status3 & REMASTER_EMERALD_STATUS3_ON_AIR) {
        return effect == EFFECT_GUST
            || effect == EFFECT_TWISTER
            || effect == EFFECT_THUNDER
            || effect == EFFECT_SKY_UPPERCUT;
    }

    if (defender->status3 & REMASTER_EMERALD_STATUS3_UNDERGROUND) {
        return move_id == MOVE_EARTHQUAKE
            || effect == EFFECT_MAGNITUDE;
    }

    if (defender->status3 & REMASTER_EMERALD_STATUS3_UNDERWATER) {
        return move_id == MOVE_SURF
            || move_id == MOVE_WHIRLPOOL;
    }

    return 1;
}

static void battle_clear_charge(RemasterEmeraldBattleMon *mon)
{
    if (mon == 0)
        return;
    mon->charging_move = 0;
    mon->charging_move_slot = 0;
    mon->charging_target = 0xFFu;
    mon->status2 &= ~REMASTER_EMERALD_STATUS2_MULTIPLETURNS;
    mon->status3 &= ~(REMASTER_EMERALD_STATUS3_ON_AIR
        | REMASTER_EMERALD_STATUS3_UNDERGROUND
        | REMASTER_EMERALD_STATUS3_UNDERWATER);
}

static uint8_t battle_default_target(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler);

static int battle_move_invalid_for_sleep_assist(uint16_t move_id)
{
    const RemasterEmeraldMoveInfo *move =
        remaster_emerald_move_info(move_id);

    if (move == 0
        || move_id == 0
        || move_id == MOVE_SLEEP_TALK
        || move_id == MOVE_ASSIST
        || move_id == MOVE_MIRROR_MOVE
        || move_id == MOVE_METRONOME)
        return 1;

    if (move_id == MOVE_FOCUS_PUNCH
        || move_id == MOVE_UPROAR
        || battle_is_two_turn_effect(move)
        || move->effect == EFFECT_BIDE)
        return 1;

    return 0;
}

static int battle_move_forbidden_metronome(uint16_t move_id)
{
    switch (move_id) {
    case MOVE_METRONOME:
    case MOVE_STRUGGLE:
    case MOVE_SKETCH:
    case MOVE_MIMIC:
    case MOVE_COUNTER:
    case MOVE_MIRROR_COAT:
    case MOVE_PROTECT:
    case MOVE_DETECT:
    case MOVE_ENDURE:
    case MOVE_DESTINY_BOND:
    case MOVE_SLEEP_TALK:
    case MOVE_THIEF:
    case MOVE_FOLLOW_ME:
    case MOVE_SNATCH:
    case MOVE_HELPING_HAND:
    case MOVE_COVET:
    case MOVE_TRICK:
    case MOVE_FOCUS_PUNCH:
        return 1;
    default:
        return 0;
    }
}

static uint8_t battle_called_target(
    const RemasterEmeraldBattleState *battle,
    uint8_t attacker,
    uint8_t requested_target,
    uint16_t move_id)
{
    const RemasterEmeraldMoveInfo *move =
        remaster_emerald_move_info(move_id);

    if (move == 0)
        return requested_target;

    if (move->target & (1u << 4))
        return attacker;

    if (battle_valid_battler(requested_target)
        && battle->battlers[requested_target].active
        && !battle->battlers[requested_target].fainted)
        return requested_target;

    return battle_default_target(battle, attacker);
}

static int battle_call_move(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker,
    uint8_t target,
    uint8_t scratch_slot,
    uint16_t called_move)
{
    RemasterEmeraldBattleMon *mon;
    uint16_t saved_move;
    uint8_t saved_pp;
    uint8_t called_target;
    int result;

    if (battle == 0
        || !battle_valid_battler(attacker)
        || scratch_slot >= 4
        || remaster_emerald_move_info(called_move) == 0
        || called_move == 0
        || battle->called_move_depth >= 8)
        return 0;

    mon = &battle->battlers[attacker];
    saved_move = mon->moves[scratch_slot];
    saved_pp = mon->pp[scratch_slot];
    mon->moves[scratch_slot] = called_move;
    mon->pp[scratch_slot] = 1;
    called_target = battle_called_target(
        battle,
        attacker,
        target,
        called_move);

    battle->called_move_depth++;
    result = remaster_emerald_battle_use_move(
        battle,
        attacker,
        called_target,
        scratch_slot);
    battle->called_move_depth--;

    mon->moves[scratch_slot] = saved_move;
    mon->pp[scratch_slot] = saved_pp;
    return result;
}

static uint16_t battle_nature_power_move(uint8_t terrain)
{
    static const uint16_t moves[10] = {
        MOVE_STUN_SPORE,
        MOVE_RAZOR_LEAF,
        MOVE_EARTHQUAKE,
        MOVE_HYDRO_PUMP,
        MOVE_SURF,
        MOVE_BUBBLE_BEAM,
        MOVE_ROCK_SLIDE,
        MOVE_SHADOW_BALL,
        MOVE_SWIFT,
        MOVE_SWIFT
    };
    return terrain < 10
        ? moves[terrain]
        : (uint16_t)MOVE_SWIFT;
}

static uint8_t battle_gender(const RemasterEmeraldBattleMon *mon)
{
    const RemasterEmeraldSpeciesInfo *species;
    uint8_t ratio;
    uint8_t personality;

    if (mon == 0)
        return 2;

    species = remaster_emerald_species_info(mon->species);
    if (species == 0)
        return 2;

    ratio = species->gender_ratio;
    if (ratio == 255)
        return 2;
    if (ratio == 254)
        return 1;
    if (ratio == 0)
        return 0;

    personality = (uint8_t)(mon->pokemon.box.personality & 0xFFu);
    return personality < ratio ? 1u : 0u;
}

static uint8_t battle_default_target(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    uint8_t target = (uint8_t)(battler ^ 1u);
    uint8_t partner;

    if (target < REMASTER_EMERALD_BATTLE_MAX_BATTLERS
        && battle->battlers[target].active
        && !battle->battlers[target].fainted)
        return target;

    partner = battle_partner(target);
    if (partner < REMASTER_EMERALD_BATTLE_MAX_BATTLERS
        && battle->battlers[partner].active
        && !battle->battlers[partner].fainted)
        return partner;

    return target;
}

static void battle_set_weather(
    RemasterEmeraldBattleState *battle,
    uint8_t weather,
    uint8_t turns,
    uint8_t battler)
{
    if (battle == 0)
        return;
    battle->weather = weather;
    battle->weather_turns = turns;
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_WEATHER,
        battler,
        battler,
        0,
        weather,
        turns);
}

static void battle_entry_ability(
    RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    RemasterEmeraldBattleMon *mon;
    uint8_t target;

    if (battle == 0 || !battle_valid_battler(battler))
        return;

    mon = &battle->battlers[battler];
    if (!mon->active || mon->fainted)
        return;

    if (mon->side == 0
        && battle_hold_effect(mon) == HOLD_EFFECT_DOUBLE_PRIZE)
        battle->money_multiplier = 2;

    if (mon->ability == ABILITY_DRIZZLE) {
        battle_set_weather(
            battle,
            REMASTER_EMERALD_BATTLE_WEATHER_RAIN,
            0,
            battler);
    } else if (mon->ability == ABILITY_SAND_STREAM) {
        battle_set_weather(
            battle,
            REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM,
            0,
            battler);
    } else if (mon->ability == ABILITY_DROUGHT) {
        battle_set_weather(
            battle,
            REMASTER_EMERALD_BATTLE_WEATHER_SUN,
            0,
            battler);
    }

    if (mon->ability == ABILITY_INTIMIDATE) {
        for (target = 0;
             target < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
             ++target) {
            if (battle->battlers[target].active
                && !battle->battlers[target].fainted
                && battle->battlers[target].side != mon->side) {
                battle_change_stage(battle, target, 1, -1, 0);
            }
        }
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_ABILITY,
            battler,
            battler,
            0,
            mon->ability,
            0);
    }
}

size_t remaster_emerald_battle_trainer_count(void)
{
    return sizeof(kRemasterEmeraldTrainers)
        / sizeof(kRemasterEmeraldTrainers[0]);
}

const RemasterEmeraldTrainer *remaster_emerald_battle_trainer_find(
    uint16_t trainer_id)
{
    if ((size_t)trainer_id >= remaster_emerald_battle_trainer_count())
        return 0;
    return &kRemasterEmeraldTrainers[trainer_id];
}

uint32_t remaster_emerald_battle_trainer_reward(
    uint16_t trainer_id,
    uint8_t money_multiplier)
{
    const RemasterEmeraldTrainer *trainer =
        remaster_emerald_battle_trainer_find(trainer_id);
    uint32_t reward;
    uint8_t last_level;
    uint8_t class_value;

    if (trainer == 0
        || trainer->party_size == 0
        || money_multiplier == 0)
        return 0;

    last_level = trainer->party[trainer->party_size - 1u].level;
    class_value =
        kRemasterEmeraldTrainerMoneyValues[trainer->trainer_class];

    reward = 4u
        * (uint32_t)last_level
        * (uint32_t)money_multiplier
        * (uint32_t)class_value;
    if (trainer->double_battle)
        reward *= 2u;

    return reward;
}

int remaster_emerald_battle_trainer_validate(
    const RemasterEmeraldTrainer *trainer)
{
    uint8_t i;
    uint8_t j;

    if (trainer == 0
        || trainer->party_size == 0
        || trainer->party_size > REMASTER_EMERALD_BATTLE_PARTY_SIZE)
        return 0;

    if ((trainer->party_flags
            & ~(REMASTER_EMERALD_TRAINER_PARTY_CUSTOM_MOVESET
                | REMASTER_EMERALD_TRAINER_PARTY_HELD_ITEM))
        != 0)
        return 0;

    for (i = 0; i < trainer->party_size; ++i) {
        const RemasterEmeraldTrainerMon *mon = &trainer->party[i];

        if (mon->species == 0
            || mon->level == 0
            || mon->level > 100
            || remaster_emerald_species_info(mon->species) == 0)
            return 0;

        if (!(trainer->party_flags
                & REMASTER_EMERALD_TRAINER_PARTY_HELD_ITEM)
            && mon->held_item != 0)
            return 0;

        for (j = 0; j < REMASTER_EMERALD_MAX_MOVES; ++j) {
            if (!(trainer->party_flags
                    & REMASTER_EMERALD_TRAINER_PARTY_CUSTOM_MOVESET)
                && mon->moves[j] != 0)
                return 0;
            if (mon->moves[j] != 0
                && remaster_emerald_move_info(mon->moves[j]) == 0)
                return 0;
        }
    }

    return 1;
}

static uint16_t battle_trainer_shiny_value(
    uint32_t ot_id,
    uint32_t personality)
{
    return (uint16_t)(
        (ot_id >> 16u)
        ^ (ot_id & 0xFFFFu)
        ^ (personality >> 16u)
        ^ (personality & 0xFFFFu));
}

static int battle_build_trainer_mon(
    RemasterEmeraldBattleRng *rng,
    const RemasterEmeraldTrainer *trainer,
    const RemasterEmeraldTrainerMon *source,
    uint32_t personality,
    RemasterEmeraldPartyPokemon *out)
{
    const RemasterEmeraldSpeciesInfo *info;
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[REMASTER_EMERALD_STAT_COUNT];
    uint8_t evs[REMASTER_EMERALD_STAT_COUNT] = {0};
    uint8_t fixed_iv;
    uint8_t ability_num;
    uint8_t slot;
    uint32_t ot_id;

    if (rng == 0 || trainer == 0 || source == 0 || out == 0)
        return 0;

    info = remaster_emerald_species_info(source->species);
    if (info == 0)
        return 0;

    fixed_iv = (uint8_t)((uint32_t)source->iv * 31u / 255u);
    memset(ivs, fixed_iv, sizeof(ivs));
    memset(out, 0, sizeof(*out));

    do {
        ot_id = remaster_emerald_battle_random32(rng);
    } while (battle_trainer_shiny_value(ot_id, personality) < 8u);

    out->box.personality = personality;
    out->box.ot_id = ot_id;
    out->box.header_flags = 0x02u;

    if (!remaster_emerald_box_pokemon_set_species(
            &out->box, source->species)
        || !remaster_emerald_box_pokemon_set_experience(
            &out->box,
            remaster_emerald_experience_for_level(
                info->growth_rate,
                source->level))
        || !remaster_emerald_box_pokemon_set_friendship(
            &out->box,
            info->friendship))
        return 0;

    for (slot = 0; slot < REMASTER_EMERALD_STAT_COUNT; ++slot) {
        if (!remaster_emerald_box_pokemon_set_iv(
                &out->box, slot, fixed_iv))
            return 0;
    }

    ability_num =
        info->abilities[1] != 0
        ? (uint8_t)(personality & 1u)
        : 0u;
    if (!remaster_emerald_box_pokemon_set_ability_num(
            &out->box, ability_num))
        return 0;

    if (trainer->party_flags
        & REMASTER_EMERALD_TRAINER_PARTY_HELD_ITEM) {
        if (!remaster_emerald_box_pokemon_set_held_item(
                &out->box, source->held_item))
            return 0;
    }

    if (trainer->party_flags
        & REMASTER_EMERALD_TRAINER_PARTY_CUSTOM_MOVESET) {
        for (slot = 0; slot < REMASTER_EMERALD_MAX_MOVES; ++slot) {
            const RemasterEmeraldMoveInfo *move =
                remaster_emerald_move_info(source->moves[slot]);
            uint8_t pp = move != 0 ? move->pp : 0;
            if (!remaster_emerald_box_pokemon_set_move(
                    &out->box,
                    slot,
                    source->moves[slot],
                    pp))
                return 0;
        }
    } else if (!remaster_emerald_pokemon_apply_initial_moves(
            &out->box,
            source->species,
            source->level)) {
        return 0;
    }

    if (!remaster_emerald_calculate_stats(
            source->species,
            source->level,
            (uint8_t)(personality % 25u),
            ivs,
            evs,
            &stats))
        return 0;

    out->level = source->level;
    out->hp = stats.hp;
    out->max_hp = stats.hp;
    out->attack = stats.attack;
    out->defense = stats.defense;
    out->speed = stats.speed;
    out->sp_attack = stats.sp_attack;
    out->sp_defense = stats.sp_defense;
    out->box.checksum =
        remaster_emerald_box_pokemon_checksum(&out->box);
    return 1;
}

int remaster_emerald_battle_build_trainer_party(
    RemasterEmeraldBattleRng *rng,
    uint16_t trainer_id,
    RemasterEmeraldPartyPokemon out_party[
        REMASTER_EMERALD_BATTLE_PARTY_SIZE],
    uint8_t *out_count)
{
    const RemasterEmeraldTrainer *trainer =
        remaster_emerald_battle_trainer_find(trainer_id);
    uint32_t name_hash = 0;
    uint8_t i;

    if (rng == 0
        || out_party == 0
        || out_count == 0
        || trainer == 0
        || !remaster_emerald_battle_trainer_validate(trainer)
        || (size_t)trainer_id
            >= sizeof(kRemasterEmeraldTrainerNameHashes)
                / sizeof(kRemasterEmeraldTrainerNameHashes[0]))
        return 0;

    memset(
        out_party,
        0,
        sizeof(out_party[0]) * REMASTER_EMERALD_BATTLE_PARTY_SIZE);

    for (i = 0; i < trainer->party_size; ++i) {
        const RemasterEmeraldTrainerMon *source = &trainer->party[i];
        uint32_t personality;

        if ((size_t)source->species
            >= sizeof(kRemasterEmeraldSpeciesNameHashes)
                / sizeof(kRemasterEmeraldSpeciesNameHashes[0]))
            return 0;

        personality =
            trainer->double_battle
            ? UINT32_C(0x80)
            : ((trainer->encounter_music_gender & 0x80u)
                ? UINT32_C(0x78)
                : UINT32_C(0x88));

        name_hash += kRemasterEmeraldTrainerNameHashes[trainer_id];
        name_hash += kRemasterEmeraldSpeciesNameHashes[source->species];
        personality += name_hash << 8u;

        if (!battle_build_trainer_mon(
                rng,
                trainer,
                source,
                personality,
                &out_party[i]))
            return 0;
    }

    *out_count = trainer->party_size;
    return 1;
}

int remaster_emerald_battle_start_trainer_from_save(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldSave *save,
    uint16_t trainer_id)
{
    const RemasterEmeraldTrainer *trainer;
    RemasterEmeraldPartyPokemon opponent_party[
        REMASTER_EMERALD_BATTLE_PARTY_SIZE];
    uint8_t opponent_count;

    if (battle == 0 || save == 0)
        return 0;

    trainer = remaster_emerald_battle_trainer_find(trainer_id);
    if (trainer == 0
        || !remaster_emerald_battle_trainer_validate(trainer)
        || !remaster_emerald_battle_build_trainer_party(
            &battle->rng,
            trainer_id,
            opponent_party,
            &opponent_count))
        return 0;

    battle->battle_type_flags |= REMASTER_EMERALD_BATTLE_TYPE_TRAINER;
    if (trainer->double_battle)
        battle->battle_type_flags |= REMASTER_EMERALD_BATTLE_TYPE_DOUBLE;

    if (!remaster_emerald_battle_start_from_save(
            battle,
            save,
            opponent_party,
            opponent_count))
        return 0;

    battle->opponent_trainer_id = trainer_id;
    battle->opponent_trainer_ai_flags = trainer->ai_flags;
    memcpy(
        battle->opponent_trainer_items,
        trainer->items,
        sizeof(battle->opponent_trainer_items));
    return 1;
}

int remaster_emerald_battle_finalize_trainer(
    const RemasterEmeraldBattleState *battle,
    RemasterEmeraldSave *save,
    int *out_whiteout)
{
    enum { TRAINER_FLAGS_START = 0x500 };
    int whiteout = 0;

    if (battle == 0
        || save == 0
        || !battle->ended
        || !(battle->battle_type_flags
            & REMASTER_EMERALD_BATTLE_TYPE_TRAINER)
        || remaster_emerald_battle_trainer_find(
            battle->opponent_trainer_id) == 0)
        return 0;

    if (!remaster_emerald_battle_commit_player_party(battle, save))
        return 0;

    switch (battle->outcome) {
    case REMASTER_EMERALD_BATTLE_OUTCOME_WON:
    {
        enum { MAX_MONEY = 999999 };
        RemasterEmeraldOverworldState world;
        uint32_t trainer_reward;
        uint32_t total_reward;
        uint32_t next_money;

        if (!remaster_emerald_overworld_get(save, &world))
            return 0;

        trainer_reward = remaster_emerald_battle_trainer_reward(
            battle->opponent_trainer_id,
            battle->money_multiplier != 0 ? battle->money_multiplier : 1);
        total_reward = trainer_reward + battle->money_reward;
        if (total_reward < trainer_reward)
            total_reward = UINT32_MAX;

        next_money = world.money + total_reward;
        if (next_money > MAX_MONEY || next_money < world.money)
            next_money = MAX_MONEY;
        world.money = next_money;

        if (!remaster_emerald_flag_set(
                save,
                (uint16_t)(TRAINER_FLAGS_START
                    + battle->opponent_trainer_id),
                1)
            || !remaster_emerald_overworld_set(save, &world))
            return 0;
        break;
    }

    case REMASTER_EMERALD_BATTLE_OUTCOME_LOST:
    case REMASTER_EMERALD_BATTLE_OUTCOME_DREW:
        whiteout = 1;
        break;

    default:
        return 0;
    }

    if (out_whiteout != 0)
        *out_whiteout = whiteout;
    return 1;
}

void remaster_emerald_battle_state_init(
    RemasterEmeraldBattleState *battle,
    uint32_t battle_type_flags,
    uint32_t seed)
{
    if (battle == 0)
        return;

    memset(battle, 0, sizeof(*battle));
    battle->battle_type_flags = battle_type_flags;
    battle->money_multiplier = 1;
    battle->follow_me_target[0] = 0xFFu;
    battle->follow_me_target[1] = 0xFFu;
    memset(
        battle->future_attacker,
        0xFF,
        sizeof(battle->future_attacker));
    remaster_emerald_battle_rng_seed(&battle->rng, seed);
}

static int battle_first_party_slot(
    const RemasterEmeraldPartyPokemon *party,
    uint8_t count,
    uint8_t start,
    uint8_t *out_slot)
{
    uint8_t i;

    if (party == 0 || out_slot == 0)
        return 0;

    for (i = start; i < count; ++i) {
        if (party[i].hp != 0
            && remaster_emerald_box_pokemon_species(&party[i].box) != 0) {
            *out_slot = i;
            return 1;
        }
    }
    return 0;
}

int remaster_emerald_battle_start(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldPartyPokemon *player_party,
    uint8_t player_count,
    const RemasterEmeraldPartyPokemon *opponent_party,
    uint8_t opponent_count)
{
    uint8_t player_slot;
    uint8_t opponent_slot;
    uint8_t next_player;
    uint8_t next_opponent;

    if (battle == 0
        || player_party == 0
        || opponent_party == 0
        || player_count == 0
        || opponent_count == 0
        || player_count > REMASTER_EMERALD_BATTLE_PARTY_SIZE
        || opponent_count > REMASTER_EMERALD_BATTLE_PARTY_SIZE)
        return 0;

    memcpy(
        battle->parties[0],
        player_party,
        (size_t)player_count * sizeof(player_party[0]));
    memcpy(
        battle->parties[1],
        opponent_party,
        (size_t)opponent_count * sizeof(opponent_party[0]));
    battle->party_count[0] = player_count;
    battle->party_count[1] = opponent_count;

    if (!battle_first_party_slot(
            player_party,
            player_count,
            0,
            &player_slot)
        || !battle_first_party_slot(
            opponent_party,
            opponent_count,
            0,
            &opponent_slot))
        return 0;

    if (!battle_load_mon(
            &battle->battlers[0],
            &battle->parties[0][player_slot],
            0,
            player_slot)
        || !battle_load_mon(
            &battle->battlers[1],
            &battle->parties[1][opponent_slot],
            1,
            opponent_slot))
        return 0;

    battle->active_party_slot[0] = player_slot;
    battle->active_party_slot[1] = opponent_slot;

    if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE) {
        if (battle_first_party_slot(
                player_party,
                player_count,
                (uint8_t)(player_slot + 1u),
                &next_player)) {
            battle_load_mon(
                &battle->battlers[2],
                &battle->parties[0][next_player],
                0,
                next_player);
            battle->active_party_slot[2] = next_player;
        }
        if (battle_first_party_slot(
                opponent_party,
                opponent_count,
                (uint8_t)(opponent_slot + 1u),
                &next_opponent)) {
            battle_load_mon(
                &battle->battlers[3],
                &battle->parties[1][next_opponent],
                1,
                next_opponent);
            battle->active_party_slot[3] = next_opponent;
        }
    }

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_STARTED,
        0,
        1,
        0,
        0,
        battle->battle_type_flags);

    battle_entry_ability(battle, 0);
    battle_entry_ability(battle, 1);
    if (battle->battlers[2].active)
        battle_entry_ability(battle, 2);
    if (battle->battlers[3].active)
        battle_entry_ability(battle, 3);

    return 1;
}

int remaster_emerald_battle_start_from_save(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldSave *save,
    const RemasterEmeraldPartyPokemon *opponent_party,
    uint8_t opponent_count)
{
    RemasterEmeraldPartyPokemon party[REMASTER_EMERALD_BATTLE_PARTY_SIZE];
    uint8_t count;
    uint8_t i;

    if (battle == 0 || save == 0 || opponent_party == 0)
        return 0;

    count = remaster_emerald_party_count(save);
    if (count == 0)
        return 0;

    battle->player_badge_mask = 0;
    for (i = 0; i < 8; ++i) {
        int value = 0;
        if (remaster_emerald_flag_get(
                save,
                (uint16_t)(FLAG_BADGE01_GET + i),
                &value)
            && value)
            battle->player_badge_mask |= (uint8_t)(1u << i);
    }

    memset(party, 0, sizeof(party));
    for (i = 0; i < count; ++i) {
        if (!remaster_emerald_party_get(save, i, &party[i], 0))
            return 0;
    }

    return remaster_emerald_battle_start(
        battle,
        party,
        count,
        opponent_party,
        opponent_count);
}

static int battle_is_physical(uint8_t type)
{
    return type < TYPE_MYSTERY;
}

static uint8_t battle_type_multiplier(
    const RemasterEmeraldBattleMon *defender,
    uint8_t move_type)
{
    uint16_t value;

    if (defender == 0)
        return 10;

    if (move_type == TYPE_MYSTERY)
        return 0;

    if (defender->ability == ABILITY_LEVITATE
        && move_type == TYPE_GROUND)
        return 0;

    value = remaster_emerald_battle_type_effectiveness(
        move_type,
        defender->types[0]);
    if ((defender->status2 & REMASTER_EMERALD_STATUS2_FORESIGHT)
        && defender->types[0] == TYPE_GHOST
        && (move_type == TYPE_NORMAL || move_type == TYPE_FIGHTING))
        value = 10;

    if (defender->types[1] != defender->types[0]) {
        uint8_t second = remaster_emerald_battle_type_effectiveness(
            move_type,
            defender->types[1]);
        if ((defender->status2 & REMASTER_EMERALD_STATUS2_FORESIGHT)
            && defender->types[1] == TYPE_GHOST
            && (move_type == TYPE_NORMAL || move_type == TYPE_FIGHTING))
            second = 10;
        value = value * second / 10u;
    }

    if (value > 255)
        value = 255;
    return (uint8_t)value;
}

static int battle_type_power_hold_effect(
    uint8_t hold_effect,
    uint8_t type)
{
    static const uint8_t hold_types[] = {
        TYPE_BUG,
        TYPE_STEEL,
        TYPE_GROUND,
        TYPE_ROCK,
        TYPE_GRASS,
        TYPE_DARK,
        TYPE_FIGHTING,
        TYPE_ELECTRIC,
        TYPE_WATER,
        TYPE_FLYING,
        TYPE_POISON,
        TYPE_ICE,
        TYPE_GHOST,
        TYPE_PSYCHIC,
        TYPE_FIRE,
        TYPE_DRAGON,
        TYPE_NORMAL
    };
    static const uint8_t hold_effects[] = {
        HOLD_EFFECT_BUG_POWER,
        HOLD_EFFECT_STEEL_POWER,
        HOLD_EFFECT_GROUND_POWER,
        HOLD_EFFECT_ROCK_POWER,
        HOLD_EFFECT_GRASS_POWER,
        HOLD_EFFECT_DARK_POWER,
        HOLD_EFFECT_FIGHTING_POWER,
        HOLD_EFFECT_ELECTRIC_POWER,
        HOLD_EFFECT_WATER_POWER,
        HOLD_EFFECT_FLYING_POWER,
        HOLD_EFFECT_POISON_POWER,
        HOLD_EFFECT_ICE_POWER,
        HOLD_EFFECT_GHOST_POWER,
        HOLD_EFFECT_PSYCHIC_POWER,
        HOLD_EFFECT_FIRE_POWER,
        HOLD_EFFECT_DRAGON_POWER,
        HOLD_EFFECT_NORMAL_POWER
    };
    size_t i;

    for (i = 0; i < sizeof(hold_types); ++i) {
        if (hold_effect == hold_effects[i] && type == hold_types[i])
            return 1;
    }
    return 0;
}

static int battle_high_critical_effect(uint8_t effect)
{
    return effect == EFFECT_HIGH_CRITICAL
        || effect == 75
        || effect == EFFECT_BLAZE_KICK
        || effect == EFFECT_POISON_TAIL;
}

static int battle_critical(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleMon *attacker,
    const RemasterEmeraldBattleMon *defender,
    uint8_t effect)
{
    static const uint16_t denominators[5] = {16, 8, 4, 3, 2};
    unsigned stage = 0;

    if (defender->ability == ABILITY_BATTLE_ARMOR
        || defender->ability == ABILITY_SHELL_ARMOR)
        return 0;

    if (battle_high_critical_effect(effect))
        stage++;
    if (attacker->status2 & REMASTER_EMERALD_STATUS2_FOCUS_ENERGY)
        stage += 2u;
    if (battle_hold_effect(attacker) == HOLD_EFFECT_SCOPE_LENS)
        stage++;
    if (battle_hold_effect(attacker) == HOLD_EFFECT_LUCKY_PUNCH
        && attacker->species == SPECIES_CHANSEY)
        stage += 2u;
    if (battle_hold_effect(attacker) == HOLD_EFFECT_STICK
        && attacker->species == SPECIES_FARFETCHD)
        stage += 2u;

    if (stage > 4)
        stage = 4;
    return remaster_emerald_battle_random(&battle->rng)
        % denominators[stage] == 0;
}

static uint8_t battle_accuracy_percent(
    const RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleMon *attacker,
    const RemasterEmeraldBattleMon *defender,
    const RemasterEmeraldMoveInfo *move)
{
    static const uint8_t numerators[13] = {
        3,3,3,3,3,3,3,4,5,6,7,8,9
    };
    static const uint8_t denominators[13] = {
        9,8,7,6,5,4,3,3,3,3,3,3,3
    };
    int stage;
    uint32_t accuracy;

    if (move->accuracy == 0 || move->effect == EFFECT_ALWAYS_HIT)
        return 100;

    if (move->effect == EFFECT_THUNDER
        && battle_weather_has_effect(battle)) {
        if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_RAIN)
            return 100;
        if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SUN)
            return 50;
    }

    stage = (int)attacker->stat_stages[6]
        - (int)defender->stat_stages[7] + 6;
    if (stage < 0)
        stage = 0;
    if (stage > 12)
        stage = 12;

    accuracy = (uint32_t)move->accuracy
        * numerators[stage]
        / denominators[stage];

    if (attacker->ability == ABILITY_COMPOUND_EYES)
        accuracy = accuracy * 130u / 100u;

    if (attacker->ability == ABILITY_HUSTLE
        && battle_is_physical(move->type))
        accuracy = accuracy * 80u / 100u;

    if (defender->ability == ABILITY_SAND_VEIL
        && battle_weather_has_effect(battle)
        && battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM)
        accuracy = accuracy * 80u / 100u;

    if (battle_hold_effect(defender) == HOLD_EFFECT_EVASION_UP)
        accuracy = accuracy
            * (100u - battle_hold_param(defender))
            / 100u;

    if (accuracy > 100)
        accuracy = 100;
    return (uint8_t)accuracy;
}

static uint16_t battle_dynamic_power(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleMon *attacker,
    const RemasterEmeraldBattleMon *defender,
    const RemasterEmeraldMoveInfo *move,
    uint8_t *io_type)
{
    uint32_t hp_percent;
    uint8_t ivs[6];

    (void)defender;

    switch (move->effect) {
    case EFFECT_FLAIL:
        hp_percent = attacker->pokemon.max_hp != 0
            ? (uint32_t)attacker->pokemon.hp * 48u
                / attacker->pokemon.max_hp
            : 48u;
        if (hp_percent <= 1)
            return 200;
        if (hp_percent <= 4)
            return 150;
        if (hp_percent <= 9)
            return 100;
        if (hp_percent <= 16)
            return 80;
        if (hp_percent <= 32)
            return 40;
        return 20;
    case EFFECT_RETURN:
        return (uint16_t)(
            remaster_emerald_box_pokemon_friendship(&attacker->pokemon.box)
            * 10u / 25u);
    case EFFECT_FRUSTRATION:
        return (uint16_t)(
            (255u - remaster_emerald_box_pokemon_friendship(
                &attacker->pokemon.box))
            * 10u / 25u);
    case EFFECT_MAGNITUDE: {
        const uint16_t roll =
            remaster_emerald_battle_random(&battle->rng) % 100u;
        if (roll < 5)
            return 10;
        if (roll < 15)
            return 30;
        if (roll < 35)
            return 50;
        if (roll < 65)
            return 70;
        if (roll < 85)
            return 90;
        if (roll < 95)
            return 110;
        return 150;
    }
    case EFFECT_HIDDEN_POWER: {
        uint32_t bits = 0;
        uint32_t power_bits = 0;
        size_t i;

        remaster_emerald_box_pokemon_ivs(&attacker->pokemon.box, ivs);
        for (i = 0; i < 6; ++i) {
            bits |= (uint32_t)(ivs[i] & 1u) << i;
            power_bits |= (uint32_t)((ivs[i] >> 1u) & 1u) << i;
        }
        *io_type = (uint8_t)((bits * 15u) / 63u + 1u);
        if (*io_type >= TYPE_MYSTERY)
            (*io_type)++;
        return (uint16_t)((power_bits * 40u) / 63u + 30u);
    }
    case EFFECT_FACADE:
        return attacker->pokemon.status != 0
            ? (uint16_t)(move->power * 2u)
            : move->power;
    case EFFECT_ERUPTION:
        if (attacker->pokemon.max_hp == 0)
            return 1;
        return (uint16_t)(
            (uint32_t)move->power
            * attacker->pokemon.hp
            / attacker->pokemon.max_hp);
    case EFFECT_WEATHER_BALL:
        if (battle_weather_has_effect(battle)
            && battle->weather != REMASTER_EMERALD_BATTLE_WEATHER_NONE) {
            switch (battle->weather) {
            case REMASTER_EMERALD_BATTLE_WEATHER_RAIN:
                *io_type = TYPE_WATER;
                break;
            case REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM:
                *io_type = TYPE_ROCK;
                break;
            case REMASTER_EMERALD_BATTLE_WEATHER_SUN:
                *io_type = TYPE_FIRE;
                break;
            case REMASTER_EMERALD_BATTLE_WEATHER_HAIL:
                *io_type = TYPE_ICE;
                break;
            default:
                break;
            }
            return (uint16_t)(move->power * 2u);
        }
        return move->power;
    case EFFECT_LOW_KICK:
        return 60;
    case EFFECT_ROLLOUT: {
        uint32_t value = move->power;
        uint8_t count = attacker->rollout_count;
        if (count > 4)
            count = 4;
        value <<= count;
        if (attacker->status2 & REMASTER_EMERALD_STATUS2_DEFENSE_CURL)
            value *= 2u;
        return (uint16_t)(value > UINT16_MAX ? UINT16_MAX : value);
    }
    case EFFECT_FURY_CUTTER: {
        uint32_t value = move->power;
        uint8_t count = attacker->fury_cutter_count;
        if (count > 4)
            count = 4;
        value <<= count;
        return (uint16_t)(value > UINT16_MAX ? UINT16_MAX : value);
    }
    case EFFECT_REVENGE:
        return attacker->last_damage_turn
                == (uint8_t)(battle->turn_number & 0xFFu)
            ? (uint16_t)(move->power * 2u)
            : move->power;
    case EFFECT_SPIT_UP:
        if (attacker->stockpile == 0)
            return 0;
        return (uint16_t)(100u * attacker->stockpile);
    default:
        return move->power;
    }
}

static int battle_calculate_damage_internal(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker_id,
    uint8_t defender_id,
    uint16_t move_id,
    RemasterEmeraldBattleDamageResult *out_result,
    int skip_accuracy,
    int allow_critical)
{
    RemasterEmeraldBattleMon *attacker;
    RemasterEmeraldBattleMon *defender;
    const RemasterEmeraldMoveInfo *move;
    uint8_t move_type;
    uint16_t power;
    uint8_t type_multiplier;
    uint8_t accuracy;
    int physical;
    int critical;
    uint8_t atk_stage;
    uint8_t def_stage;
    uint32_t attack;
    uint32_t defense;
    uint32_t damage;
    uint8_t hold_effect;

    if (battle == 0
        || out_result == 0
        || !battle_valid_battler(attacker_id)
        || !battle_valid_battler(defender_id))
        return 0;

    memset(out_result, 0, sizeof(*out_result));
    attacker = &battle->battlers[attacker_id];
    defender = &battle->battlers[defender_id];
    move = remaster_emerald_move_info(move_id);
    if (move == 0 || !attacker->active || !defender->active)
        return 0;

    if ((defender->status3
            & (REMASTER_EMERALD_STATUS3_ON_AIR
               | REMASTER_EMERALD_STATUS3_UNDERGROUND
               | REMASTER_EMERALD_STATUS3_UNDERWATER))
        && !battle_move_can_hit_semi_invulnerable(
            defender,
            move_id,
            move->effect)) {
        out_result->hit = 0;
        return 1;
    }

    if (skip_accuracy) {
        accuracy = 100;
    } else if (attacker->lock_on_turns != 0
        && attacker->lock_on_target == defender_id) {
        accuracy = 100;
    } else {
        accuracy = battle_accuracy_percent(
            battle,
            attacker,
            defender,
            move);
    }
    if (!skip_accuracy
        && remaster_emerald_battle_random(&battle->rng) % 100u
            >= accuracy) {
        out_result->hit = 0;
        return 1;
    }
    out_result->hit = 1;

    move_type = move->type;
    power = battle_dynamic_power(
        battle,
        attacker,
        defender,
        move,
        &move_type);

    if (move->effect == EFFECT_PURSUIT
        && battle->pursuit_boost[attacker_id])
        power = (uint16_t)(power > UINT16_MAX / 2u
            ? UINT16_MAX
            : power * 2u);
    if (move->effect == EFFECT_PURSUIT
        && battle->pursuit_boost[attacker_id])
        power = (uint16_t)(move->power * 2u);

    if ((defender->status3 & REMASTER_EMERALD_STATUS3_ON_AIR)
        && (move->effect == EFFECT_GUST
            || move->effect == EFFECT_TWISTER))
        power = (uint16_t)(power > UINT16_MAX / 2u
            ? UINT16_MAX
            : power * 2u);

    if ((defender->status3 & REMASTER_EMERALD_STATUS3_UNDERGROUND)
        && (move_id == MOVE_EARTHQUAKE
            || move->effect == EFFECT_MAGNITUDE))
        power = (uint16_t)(power > UINT16_MAX / 2u
            ? UINT16_MAX
            : power * 2u);

    if ((defender->status3 & REMASTER_EMERALD_STATUS3_UNDERWATER)
        && (move_id == MOVE_SURF || move_id == MOVE_WHIRLPOOL))
        power = (uint16_t)(power > UINT16_MAX / 2u
            ? UINT16_MAX
            : power * 2u);

    if ((defender->status3 & REMASTER_EMERALD_STATUS3_MINIMIZED)
        && move->effect == EFFECT_FLINCH_MINIMIZE_HIT)
        power = (uint16_t)(power > UINT16_MAX / 2u
            ? UINT16_MAX
            : power * 2u);

    type_multiplier = move->power == 0
        ? 10u
        : battle_type_multiplier(defender, move_type);

    if (move->power != 0
        && defender->ability == ABILITY_VOLT_ABSORB
        && move_type == TYPE_ELECTRIC)
        type_multiplier = 0;
    if (move->power != 0
        && defender->ability == ABILITY_WATER_ABSORB
        && move_type == TYPE_WATER)
        type_multiplier = 0;
    if (move->power != 0
        && defender->ability == ABILITY_FLASH_FIRE
        && move_type == TYPE_FIRE)
        type_multiplier = 0;
    if (defender->ability == ABILITY_WONDER_GUARD
        && type_multiplier <= 10
        && move->power != 0)
        type_multiplier = 0;

    out_result->effectiveness_tenths = type_multiplier;
    if (type_multiplier == 0) {
        out_result->immune = 1;
        return 1;
    }

    if (move->power == 0 && power == 0)
        return 1;

    physical = battle_is_physical(move_type);
    critical = allow_critical
        ? battle_critical(
            battle,
            attacker,
            defender,
            move->effect)
        : 0;
    out_result->critical = critical ? 1u : 0u;

    atk_stage = physical ? attacker->stat_stages[1] : attacker->stat_stages[4];
    def_stage = physical ? defender->stat_stages[2] : defender->stat_stages[5];

    if (critical && atk_stage < 6)
        atk_stage = 6;
    if (critical && def_stage > 6)
        def_stage = 6;

    attack = physical ? attacker->pokemon.attack : attacker->pokemon.sp_attack;
    defense = physical ? defender->pokemon.defense : defender->pokemon.sp_defense;
    hold_effect = battle_hold_effect(attacker);

    if (physical
        && (attacker->ability == ABILITY_HUGE_POWER
            || attacker->ability == ABILITY_PURE_POWER))
        attack *= 2u;

    if (battle_badge_boost_allowed(
            battle,
            attacker_id,
            physical ? 0u : 6u))
        attack = attack * 110u / 100u;
    if (battle_badge_boost_allowed(
            battle,
            defender_id,
            physical ? 4u : 6u))
        defense = defense * 110u / 100u;

    if (battle_type_power_hold_effect(hold_effect, move_type))
        attack = attack
            * (100u + battle_hold_param(attacker))
            / 100u;

    if (physical && hold_effect == HOLD_EFFECT_CHOICE_BAND)
        attack = attack * 150u / 100u;

    if (!physical
        && hold_effect == HOLD_EFFECT_SOUL_DEW
        && !battle_frontier_type(battle->battle_type_flags)
        && (attacker->species == SPECIES_LATIAS
            || attacker->species == SPECIES_LATIOS))
        attack = attack * 150u / 100u;
    if (!physical
        && hold_effect == HOLD_EFFECT_DEEP_SEA_TOOTH
        && attacker->species == SPECIES_CLAMPERL)
        attack *= 2u;
    if (!physical
        && hold_effect == HOLD_EFFECT_LIGHT_BALL
        && attacker->species == SPECIES_PIKACHU)
        attack *= 2u;
    if (physical
        && hold_effect == HOLD_EFFECT_THICK_CLUB
        && (attacker->species == SPECIES_CUBONE
            || attacker->species == SPECIES_MAROWAK))
        attack *= 2u;

    if (!physical
        && battle_hold_effect(defender) == HOLD_EFFECT_SOUL_DEW
        && !battle_frontier_type(battle->battle_type_flags)
        && (defender->species == SPECIES_LATIAS
            || defender->species == SPECIES_LATIOS))
        defense = defense * 150u / 100u;
    if (!physical
        && battle_hold_effect(defender) == HOLD_EFFECT_DEEP_SEA_SCALE
        && defender->species == SPECIES_CLAMPERL)
        defense *= 2u;
    if (physical
        && battle_hold_effect(defender) == HOLD_EFFECT_METAL_POWDER
        && defender->species == SPECIES_DITTO)
        defense *= 2u;

    if (physical
        && attacker->ability == ABILITY_GUTS
        && attacker->pokemon.status != 0)
        attack = attack * 150u / 100u;
    if (physical && attacker->ability == ABILITY_HUSTLE)
        attack = attack * 150u / 100u;
    if (!physical
        && ((attacker->ability == ABILITY_PLUS
                && battle_ability_on_field(battle, ABILITY_MINUS))
            || (attacker->ability == ABILITY_MINUS
                && battle_ability_on_field(battle, ABILITY_PLUS))))
        attack = attack * 150u / 100u;
    if (physical
        && defender->ability == ABILITY_MARVEL_SCALE
        && defender->pokemon.status != 0)
        defense = defense * 150u / 100u;
    if (!physical
        && defender->ability == ABILITY_THICK_FAT
        && (move_type == TYPE_FIRE || move_type == TYPE_ICE))
        attack /= 2u;

    if (move->effect == EFFECT_EXPLOSION && physical)
        defense /= 2u;

    attack = battle_apply_stage(attack, atk_stage);
    defense = battle_apply_stage(defense, def_stage);

    if (attacker->pokemon.hp * 3u <= attacker->pokemon.max_hp) {
        if ((attacker->ability == ABILITY_OVERGROW && move_type == TYPE_GRASS)
            || (attacker->ability == ABILITY_BLAZE && move_type == TYPE_FIRE)
            || (attacker->ability == ABILITY_TORRENT && move_type == TYPE_WATER)
            || (attacker->ability == ABILITY_SWARM && move_type == TYPE_BUG))
            power = (uint16_t)((uint32_t)power * 150u / 100u);
    }

    if (move_type == TYPE_ELECTRIC
        && battle_field_has_status3(
            battle,
            REMASTER_EMERALD_STATUS3_MUD_SPORT))
        power /= 2u;
    if (move_type == TYPE_FIRE
        && battle_field_has_status3(
            battle,
            REMASTER_EMERALD_STATUS3_WATER_SPORT))
        power /= 2u;

    if (power == 0)
        power = 1;
    if (defense == 0)
        defense = 1;

    damage = ((2u * attacker->pokemon.level / 5u + 2u)
            * power
            * attack
            / defense)
        / 50u;

    if (physical
        && (attacker->pokemon.status & REMASTER_EMERALD_STATUS1_BURN)
        && attacker->ability != ABILITY_GUTS)
        damage /= 2u;

    if (!critical) {
        if (physical
            && (battle->side_status[defender->side]
                & REMASTER_EMERALD_SIDE_REFLECT)) {
            if ((battle->battle_type_flags
                    & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE)
                && battle_alive_on_side(battle, defender->side) == 2)
                damage = 2u * (damage / 3u);
            else
                damage /= 2u;
        }
        if (!physical
            && (battle->side_status[defender->side]
                & REMASTER_EMERALD_SIDE_LIGHT_SCREEN)) {
            if ((battle->battle_type_flags
                    & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE)
                && battle_alive_on_side(battle, defender->side) == 2)
                damage = 2u * (damage / 3u);
            else
                damage /= 2u;
        }
    }

    if ((battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE)
        && move->target == (1u << 3)
        && battle_alive_on_side(battle, defender->side) == 2)
        damage /= 2u;

    if (!physical && battle_weather_has_effect(battle)) {
        if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_RAIN) {
            if (move_type == TYPE_FIRE)
                damage /= 2u;
            else if (move_type == TYPE_WATER)
                damage = damage * 15u / 10u;
        }
        if ((battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_RAIN
                || battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM
                || battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_HAIL)
            && move->effect == EFFECT_SOLAR_BEAM)
            damage /= 2u;
        if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SUN) {
            if (move_type == TYPE_FIRE)
                damage = damage * 15u / 10u;
            else if (move_type == TYPE_WATER)
                damage /= 2u;
        }
    }

    if (!physical && attacker->flash_fire && move_type == TYPE_FIRE)
        damage = damage * 15u / 10u;

    if (move_type == TYPE_ELECTRIC
        && (attacker->status3 & REMASTER_EMERALD_STATUS3_CHARGED_UP))
        damage *= 2u;

    damage += 2u;

    if (critical)
        damage *= 2u;

    if (attacker->helping_hand)
        damage = damage * 15u / 10u;

    if (attacker->types[0] == move_type
        || attacker->types[1] == move_type)
        damage = damage * 15u / 10u;

    damage = damage * type_multiplier / 10u;
    if (damage == 0 && type_multiplier != 0)
        damage = 1;

    damage = damage
        * (85u + remaster_emerald_battle_random(&battle->rng) % 16u)
        / 100u;

    if (damage == 0)
        damage = 1;
    if (damage > UINT16_MAX)
        damage = UINT16_MAX;

    out_result->damage = (uint16_t)damage;
    return 1;
}

int remaster_emerald_battle_calculate_damage(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker_id,
    uint8_t defender_id,
    uint16_t move_id,
    RemasterEmeraldBattleDamageResult *out_result)
{
    return battle_calculate_damage_internal(
        battle,
        attacker_id,
        defender_id,
        move_id,
        out_result,
        0,
        1);
}

static int battle_status_allowed(
    const RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleMon *target,
    uint32_t status)
{
    uint8_t i;

    if (target == 0 || target->pokemon.status != 0)
        return 0;

    if (status & REMASTER_EMERALD_STATUS1_SLEEP) {
        if (target->ability == ABILITY_INSOMNIA
            || target->ability == ABILITY_VITAL_SPIRIT)
            return 0;
        if (battle != 0 && target->ability != ABILITY_SOUNDPROOF) {
            for (i = 0;
                 i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
                 ++i) {
                if (battle->battlers[i].active
                    && !battle->battlers[i].fainted
                    && (battle->battlers[i].status2
                        & REMASTER_EMERALD_STATUS2_UPROAR))
                    return 0;
            }
        }
    }
    if ((status
            & (REMASTER_EMERALD_STATUS1_POISON
               | REMASTER_EMERALD_STATUS1_TOXIC))
        && (target->ability == ABILITY_IMMUNITY
            || target->types[0] == TYPE_POISON
            || target->types[1] == TYPE_POISON
            || target->types[0] == TYPE_STEEL
            || target->types[1] == TYPE_STEEL))
        return 0;
    if ((status & REMASTER_EMERALD_STATUS1_BURN)
        && (target->ability == ABILITY_WATER_VEIL
            || target->types[0] == TYPE_FIRE
            || target->types[1] == TYPE_FIRE))
        return 0;
    if ((status & REMASTER_EMERALD_STATUS1_FREEZE)
        && (target->ability == ABILITY_MAGMA_ARMOR
            || target->types[0] == TYPE_ICE
            || target->types[1] == TYPE_ICE))
        return 0;
    if ((status & REMASTER_EMERALD_STATUS1_PARALYSIS)
        && target->ability == ABILITY_LIMBER)
        return 0;

    return 1;
}

static int battle_apply_status(
    RemasterEmeraldBattleState *battle,
    uint8_t source,
    uint8_t target_id,
    uint32_t status,
    uint16_t move_id)
{
    RemasterEmeraldBattleMon *target;

    if (battle == 0 || !battle_valid_battler(target_id))
        return 0;

    target = &battle->battlers[target_id];
    if (!battle_status_allowed(battle, target, status))
        return 0;

    if (battle->side_status[target->side] & REMASTER_EMERALD_SIDE_SAFEGUARD)
        return 0;

    if (status & REMASTER_EMERALD_STATUS1_SLEEP) {
        status = 1u + remaster_emerald_battle_random(&battle->rng) % 3u;
        if (target->ability == ABILITY_EARLY_BIRD && status > 1)
            status = (status + 1u) / 2u;
    }

    target->pokemon.status = status;
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_STATUS,
        source,
        target_id,
        move_id,
        (int32_t)status,
        0);
    return 1;
}

static int battle_apply_confusion(
    RemasterEmeraldBattleState *battle,
    uint8_t source,
    uint8_t target_id,
    uint16_t move_id)
{
    RemasterEmeraldBattleMon *target;
    uint32_t turns;

    if (battle == 0 || !battle_valid_battler(target_id))
        return 0;

    target = &battle->battlers[target_id];
    if (target->ability == ABILITY_OWN_TEMPO
        || (target->status2 & REMASTER_EMERALD_STATUS2_CONFUSION))
        return 0;

    turns = 2u + remaster_emerald_battle_random(&battle->rng) % 4u;
    target->status2 &= ~REMASTER_EMERALD_STATUS2_CONFUSION;
    target->status2 |= turns;
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_STATUS,
        source,
        target_id,
        move_id,
        (int32_t)target->status2,
        2);
    return 1;
}

static void battle_heal(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint16_t amount,
    uint16_t move_id)
{
    RemasterEmeraldBattleMon *mon;
    uint16_t missing;

    if (battle == 0 || !battle_valid_battler(battler))
        return;

    mon = &battle->battlers[battler];
    if (!mon->active || mon->fainted || amount == 0)
        return;

    missing = (uint16_t)(mon->pokemon.max_hp - mon->pokemon.hp);
    if (amount > missing)
        amount = missing;
    mon->pokemon.hp = (uint16_t)(mon->pokemon.hp + amount);
    if (amount != 0) {
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_HEAL,
            battler,
            battler,
            move_id,
            amount,
            0);
    }
}

static void battle_damage_direct(
    RemasterEmeraldBattleState *battle,
    uint8_t source,
    uint8_t target_id,
    uint16_t damage,
    uint16_t move_id,
    int false_swipe)
{
    RemasterEmeraldBattleMon *target;
    uint16_t actual;

    if (battle == 0
        || !battle_valid_battler(target_id)
        || damage == 0)
        return;

    target = &battle->battlers[target_id];
    if (!target->active || target->fainted)
        return;

    /*
     * Substitute absorbs direct opposing move damage, not residual/self
     * damage such as poison, burn, weather, Curse, Leech Seed or confusion.
     */
    if (target->substitute_hp != 0
        && move_id != 0
        && source != target_id) {
        actual = damage > target->substitute_hp
            ? target->substitute_hp
            : damage;
        target->substitute_hp = (uint16_t)(target->substitute_hp - actual);
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_DAMAGE,
            source,
            target_id,
            move_id,
            actual,
            1);
        return;
    }

    actual = damage > target->pokemon.hp
        ? target->pokemon.hp
        : damage;
    if ((false_swipe
            || target->endure_turn
                == (uint8_t)(battle->turn_number & 0xFFu))
        && actual >= target->pokemon.hp
        && target->pokemon.hp > 1)
        actual = (uint16_t)(target->pokemon.hp - 1u);
    else if (actual >= target->pokemon.hp
        && target->pokemon.hp > 1
        && battle_hold_effect(target) == HOLD_EFFECT_FOCUS_BAND
        && remaster_emerald_battle_random(&battle->rng) % 100u
            < battle_hold_param(target))
        actual = (uint16_t)(target->pokemon.hp - 1u);

    target->pokemon.hp = (uint16_t)(target->pokemon.hp - actual);
    target->last_damage = actual;
    if (move_id != 0 && source != target_id) {
        const RemasterEmeraldMoveInfo *source_move =
            remaster_emerald_move_info(move_id);
        target->last_damage_from = source;
        target->last_damage_turn =
            (uint8_t)(battle->turn_number & 0xFFu);
        if (source_move != 0) {
            target->last_damage_type = source_move->type;
            target->last_damage_was_physical =
                battle_is_physical(source_move->type) ? 1u : 0u;
        }
        if (target->bide_turns != 0) {
            uint32_t total = (uint32_t)target->bide_damage + actual;
            target->bide_damage =
                (uint16_t)(total > UINT16_MAX ? UINT16_MAX : total);
        }
    }

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_DAMAGE,
        source,
        target_id,
        move_id,
        actual,
        0);

    if (source != target_id
        && move_id != 0
        && (target->status2 & REMASTER_EMERALD_STATUS2_RAGE))
        battle_change_stage(
            battle, target_id, 1, 1, target->last_move);
}

static int battle_contact_move(const RemasterEmeraldMoveInfo *move)
{
    return move != 0 && (move->flags & 1u) != 0;
}

static void battle_contact_ability(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker_id,
    uint8_t defender_id,
    const RemasterEmeraldMoveInfo *move)
{
    RemasterEmeraldBattleMon *attacker;
    RemasterEmeraldBattleMon *defender;
    uint16_t roll;

    if (battle == 0
        || !battle_valid_battler(attacker_id)
        || !battle_valid_battler(defender_id)
        || !battle_contact_move(move))
        return;

    attacker = &battle->battlers[attacker_id];
    defender = &battle->battlers[defender_id];
    if (attacker->fainted || defender->fainted)
        return;

    if (defender->ability == ABILITY_ROUGH_SKIN) {
        uint16_t damage = attacker->pokemon.max_hp / 16u;
        if (damage == 0)
            damage = 1;
        battle_damage_direct(
            battle,
            defender_id,
            attacker_id,
            damage,
            move->move_id,
            0);
        return;
    }

    roll = remaster_emerald_battle_random(&battle->rng) % 100u;
    if (roll >= 30)
        return;

    if (defender->ability == ABILITY_STATIC) {
        battle_apply_status(
            battle,
            defender_id,
            attacker_id,
            REMASTER_EMERALD_STATUS1_PARALYSIS,
            move->move_id);
    } else if (defender->ability == ABILITY_FLAME_BODY) {
        battle_apply_status(
            battle,
            defender_id,
            attacker_id,
            REMASTER_EMERALD_STATUS1_BURN,
            move->move_id);
    } else if (defender->ability == ABILITY_POISON_POINT) {
        battle_apply_status(
            battle,
            defender_id,
            attacker_id,
            REMASTER_EMERALD_STATUS1_POISON,
            move->move_id);
    } else if (defender->ability == ABILITY_EFFECT_SPORE) {
        const uint16_t effect =
            remaster_emerald_battle_random(&battle->rng) % 3u;
        if (effect == 0) {
            battle_apply_status(
                battle,
                defender_id,
                attacker_id,
                REMASTER_EMERALD_STATUS1_SLEEP,
                move->move_id);
        } else if (effect == 1) {
            battle_apply_status(
                battle,
                defender_id,
                attacker_id,
                REMASTER_EMERALD_STATUS1_POISON,
                move->move_id);
        } else {
            battle_apply_status(
                battle,
                defender_id,
                attacker_id,
                REMASTER_EMERALD_STATUS1_PARALYSIS,
                move->move_id);
        }
    }
}

static void battle_secondary_effect(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker,
    uint8_t target,
    const RemasterEmeraldMoveInfo *move)
{
    uint32_t chance;

    if (move == 0 || move->secondary_effect_chance == 0)
        return;

    chance = move->secondary_effect_chance;
    if (battle->battlers[attacker].ability == ABILITY_SERENE_GRACE)
        chance *= 2u;
    if (chance > 100)
        chance = 100;

    if (remaster_emerald_battle_random(&battle->rng) % 100u >= chance)
        return;

    switch (move->effect) {
    case EFFECT_POISON_HIT:
        battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_POISON, move->move_id);
        break;
    case EFFECT_BURN_HIT:
        battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_BURN, move->move_id);
        break;
    case EFFECT_FREEZE_HIT:
        battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_FREEZE, move->move_id);
        break;
    case EFFECT_PARALYZE_HIT:
        battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_PARALYSIS, move->move_id);
        break;
    case EFFECT_FLINCH_HIT:
    case EFFECT_SKY_ATTACK:
    case EFFECT_TWISTER:
    case EFFECT_FLINCH_MINIMIZE_HIT:
        if (battle->battlers[target].ability != ABILITY_INNER_FOCUS)
            battle->battlers[target].status2 |=
                REMASTER_EMERALD_STATUS2_FLINCHED;
        break;
    case EFFECT_CONFUSE_HIT:
        battle_apply_confusion(battle, attacker, target, move->move_id);
        break;
    case EFFECT_ATTACK_DOWN_HIT:
        battle_change_stage(battle, target, 1, -1, move->move_id);
        break;
    case EFFECT_DEFENSE_DOWN_HIT:
        battle_change_stage(battle, target, 2, -1, move->move_id);
        break;
    case EFFECT_SPEED_DOWN_HIT:
        battle_change_stage(battle, target, 3, -1, move->move_id);
        break;
    case EFFECT_SPECIAL_ATTACK_DOWN_HIT:
        battle_change_stage(battle, target, 4, -1, move->move_id);
        break;
    case EFFECT_SPECIAL_DEFENSE_DOWN_HIT:
        battle_change_stage(battle, target, 5, -1, move->move_id);
        break;
    case EFFECT_ACCURACY_DOWN_HIT:
        battle_change_stage(battle, target, 6, -1, move->move_id);
        break;
    case EFFECT_EVASION_DOWN_HIT:
        battle_change_stage(battle, target, 7, -1, move->move_id);
        break;
    case EFFECT_TRI_ATTACK: {
        uint16_t effect =
            remaster_emerald_battle_random(&battle->rng) % 3u;
        uint32_t status = effect == 0
            ? REMASTER_EMERALD_STATUS1_BURN
            : (effect == 1
                ? REMASTER_EMERALD_STATUS1_FREEZE
                : REMASTER_EMERALD_STATUS1_PARALYSIS);
        battle_apply_status(
            battle, attacker, target, status, move->move_id);
        break;
    }
    case EFFECT_POISON_FANG:
        battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_TOXIC, move->move_id);
        break;
    case EFFECT_THUNDER:
        battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_PARALYSIS, move->move_id);
        break;
    case EFFECT_DEFENSE_UP_HIT:
        battle_change_stage(
            battle, attacker, 2, 1, move->move_id);
        break;
    case EFFECT_ATTACK_UP_HIT:
        battle_change_stage(
            battle, attacker, 1, 1, move->move_id);
        break;
    case EFFECT_ALL_STATS_UP_HIT:
        battle_change_stage(battle, attacker, 1, 1, move->move_id);
        battle_change_stage(battle, attacker, 2, 1, move->move_id);
        battle_change_stage(battle, attacker, 3, 1, move->move_id);
        battle_change_stage(battle, attacker, 4, 1, move->move_id);
        battle_change_stage(battle, attacker, 5, 1, move->move_id);
        break;
    case EFFECT_SECRET_POWER:
        switch (battle->terrain) {
        case 0:
            battle_apply_status(
                battle, attacker, target,
                REMASTER_EMERALD_STATUS1_POISON, move->move_id);
            break;
        case 1:
            battle_apply_status(
                battle, attacker, target,
                REMASTER_EMERALD_STATUS1_SLEEP, move->move_id);
            break;
        case 2:
            battle_change_stage(
                battle, target, 6, -1, move->move_id);
            break;
        case 3:
            battle_change_stage(
                battle, target, 2, -1, move->move_id);
            break;
        case 4:
            battle_change_stage(
                battle, target, 1, -1, move->move_id);
            break;
        case 5:
            battle_change_stage(
                battle, target, 3, -1, move->move_id);
            break;
        case 6:
            battle_apply_confusion(
                battle, attacker, target, move->move_id);
            break;
        case 7:
            if (battle->battlers[target].ability != ABILITY_INNER_FOCUS)
                battle->battlers[target].status2 |=
                    REMASTER_EMERALD_STATUS2_FLINCHED;
            break;
        default:
            battle_apply_status(
                battle, attacker, target,
                REMASTER_EMERALD_STATUS1_PARALYSIS, move->move_id);
            break;
        }
        break;
    default:
        break;
    }
}

static int battle_apply_primary_effect(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker,
    uint8_t target,
    const RemasterEmeraldMoveInfo *move)
{
    RemasterEmeraldBattleMon *user;
    RemasterEmeraldBattleMon *foe;
    uint16_t amount;
    uint8_t i;

    if (battle == 0 || move == 0)
        return 0;

    user = &battle->battlers[attacker];
    foe = &battle->battlers[target];

    switch (move->effect) {
    case EFFECT_SLEEP:
        return battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_SLEEP, move->move_id);
    case EFFECT_TOXIC:
        return battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_TOXIC, move->move_id);
    case EFFECT_POISON:
        return battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_POISON, move->move_id);
    case EFFECT_PARALYZE:
        return battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_PARALYSIS, move->move_id);
    case EFFECT_WILL_O_WISP:
        return battle_apply_status(
            battle, attacker, target,
            REMASTER_EMERALD_STATUS1_BURN, move->move_id);
    case EFFECT_CONFUSE:
        return battle_apply_confusion(
            battle, attacker, target, move->move_id);
    case EFFECT_ATTACK_UP:
        return battle_change_stage(battle, attacker, 1, 1, move->move_id);
    case EFFECT_DEFENSE_UP:
        return battle_change_stage(battle, attacker, 2, 1, move->move_id);
    case EFFECT_SPEED_UP:
        return battle_change_stage(battle, attacker, 3, 1, move->move_id);
    case EFFECT_SPECIAL_ATTACK_UP:
        return battle_change_stage(battle, attacker, 4, 1, move->move_id);
    case EFFECT_SPECIAL_DEFENSE_UP:
        return battle_change_stage(battle, attacker, 5, 1, move->move_id);
    case EFFECT_ACCURACY_UP:
        return battle_change_stage(battle, attacker, 6, 1, move->move_id);
    case EFFECT_EVASION_UP:
        return battle_change_stage(battle, attacker, 7, 1, move->move_id);
    case EFFECT_ATTACK_UP_2:
        return battle_change_stage(battle, attacker, 1, 2, move->move_id);
    case EFFECT_DEFENSE_UP_2:
        return battle_change_stage(battle, attacker, 2, 2, move->move_id);
    case EFFECT_SPEED_UP_2:
        return battle_change_stage(battle, attacker, 3, 2, move->move_id);
    case EFFECT_SPECIAL_ATTACK_UP_2:
        return battle_change_stage(battle, attacker, 4, 2, move->move_id);
    case EFFECT_SPECIAL_DEFENSE_UP_2:
        return battle_change_stage(battle, attacker, 5, 2, move->move_id);
    case EFFECT_ACCURACY_UP_2:
        return battle_change_stage(battle, attacker, 6, 2, move->move_id);
    case EFFECT_EVASION_UP_2:
        return battle_change_stage(battle, attacker, 7, 2, move->move_id);
    case EFFECT_ATTACK_DOWN:
        return battle_change_stage(battle, target, 1, -1, move->move_id);
    case EFFECT_DEFENSE_DOWN:
        return battle_change_stage(battle, target, 2, -1, move->move_id);
    case EFFECT_SPEED_DOWN:
        return battle_change_stage(battle, target, 3, -1, move->move_id);
    case EFFECT_SPECIAL_ATTACK_DOWN:
        return battle_change_stage(battle, target, 4, -1, move->move_id);
    case EFFECT_SPECIAL_DEFENSE_DOWN:
        return battle_change_stage(battle, target, 5, -1, move->move_id);
    case EFFECT_ACCURACY_DOWN:
        return battle_change_stage(battle, target, 6, -1, move->move_id);
    case EFFECT_EVASION_DOWN:
        return battle_change_stage(battle, target, 7, -1, move->move_id);
    case EFFECT_ATTACK_DOWN_2:
        return battle_change_stage(battle, target, 1, -2, move->move_id);
    case EFFECT_DEFENSE_DOWN_2:
        return battle_change_stage(battle, target, 2, -2, move->move_id);
    case EFFECT_SPEED_DOWN_2:
        return battle_change_stage(battle, target, 3, -2, move->move_id);
    case EFFECT_SPECIAL_ATTACK_DOWN_2:
        return battle_change_stage(battle, target, 4, -2, move->move_id);
    case EFFECT_SPECIAL_DEFENSE_DOWN_2:
        return battle_change_stage(battle, target, 5, -2, move->move_id);
    case EFFECT_ACCURACY_DOWN_2:
        return battle_change_stage(battle, target, 6, -2, move->move_id);
    case EFFECT_EVASION_DOWN_2:
        return battle_change_stage(battle, target, 7, -2, move->move_id);
    case EFFECT_SPLASH:
        return 1;
    case EFFECT_ENDURE: {
        uint32_t denominator = UINT32_C(1) << user->protect_chain;
        if (denominator > 8)
            denominator = 8;
        if (remaster_emerald_battle_random(&battle->rng)
                % denominator
            != 0) {
            user->protect_chain = 0;
            return 0;
        }
        user->endure_turn =
            (uint8_t)(battle->turn_number & 0xFFu);
        if (user->protect_chain < 3)
            user->protect_chain++;
        return 1;
    }
    case EFFECT_MINIMIZE:
        user->status3 |= REMASTER_EMERALD_STATUS3_MINIMIZED;
        return battle_change_stage(
            battle, attacker, 7, 1, move->move_id);
    case EFFECT_DESTINY_BOND:
        user->status2 |= REMASTER_EMERALD_STATUS2_DESTINY_BOND;
        user->destiny_bond_turn =
            (uint8_t)(battle->turn_number & 0xFFu);
        return 1;
    case EFFECT_GRUDGE:
        user->status3 |= REMASTER_EMERALD_STATUS3_GRUDGE;
        user->grudge_turn =
            (uint8_t)(battle->turn_number & 0xFFu);
        return 1;
    case EFFECT_NIGHTMARE:
        if (!(foe->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP)
            || (foe->status2 & REMASTER_EMERALD_STATUS2_NIGHTMARE))
            return 0;
        foe->status2 |= REMASTER_EMERALD_STATUS2_NIGHTMARE;
        return 1;
    case EFFECT_LOCK_ON:
        user->lock_on_target = target;
        user->lock_on_turns = 2;
        user->status3 |= REMASTER_EMERALD_STATUS3_ALWAYS_HITS;
        return 1;
    case EFFECT_SPITE:
        if (foe->last_move == 0)
            return 0;
        for (i = 0; i < 4; ++i) {
            if (foe->moves[i] == foe->last_move && foe->pp[i] != 0) {
                uint8_t loss = (uint8_t)(
                    2u + remaster_emerald_battle_random(&battle->rng) % 4u);
                if (loss > foe->pp[i])
                    loss = foe->pp[i];
                foe->pp[i] = (uint8_t)(foe->pp[i] - loss);
                remaster_emerald_box_pokemon_set_move(
                    &foe->pokemon.box,
                    i,
                    foe->moves[i],
                    foe->pp[i]);
                battle_event(
                    battle,
                    REMASTER_EMERALD_BATTLE_EVENT_PP,
                    target,
                    target,
                    foe->last_move,
                    -(int32_t)loss,
                    0);
                return 1;
            }
        }
        return 0;
    case EFFECT_PAIN_SPLIT: {
        uint32_t total = (uint32_t)user->pokemon.hp + foe->pokemon.hp;
        uint16_t shared = (uint16_t)(total / 2u);
        user->pokemon.hp = shared > user->pokemon.max_hp
            ? user->pokemon.max_hp
            : shared;
        foe->pokemon.hp = shared > foe->pokemon.max_hp
            ? foe->pokemon.max_hp
            : shared;
        return 1;
    }
    case EFFECT_YAWN:
        if (foe->pokemon.status != 0
            || (foe->status3 & REMASTER_EMERALD_STATUS3_YAWN)
            || foe->ability == ABILITY_INSOMNIA
            || foe->ability == ABILITY_VITAL_SPIRIT)
            return 0;
        foe->status3 |= 2u << 11u;
        return 1;
    case EFFECT_STOCKPILE:
        if (user->stockpile >= 3)
            return 0;
        user->stockpile++;
        return 1;
    case EFFECT_SWALLOW:
        if (user->stockpile == 0
            || user->pokemon.hp == user->pokemon.max_hp)
            return 0;
        amount = user->stockpile == 1
            ? user->pokemon.max_hp / 4u
            : (user->stockpile == 2
                ? user->pokemon.max_hp / 2u
                : user->pokemon.max_hp);
        user->stockpile = 0;
        if (amount == 0)
            amount = 1;
        battle_heal(battle, attacker, amount, move->move_id);
        return 1;
    case EFFECT_CHARGE:
        user->status3 |= REMASTER_EMERALD_STATUS3_CHARGED_UP;
        return battle_change_stage(
            battle, attacker, 5, 1, move->move_id);
    case EFFECT_CURSE:
        if (user->types[0] == TYPE_GHOST
            || user->types[1] == TYPE_GHOST) {
            if (foe->status2 & REMASTER_EMERALD_STATUS2_CURSED)
                return 0;
            amount = user->pokemon.max_hp / 2u;
            if (amount == 0)
                amount = 1;
            battle_damage_direct(
                battle, attacker, attacker, amount, move->move_id, 0);
            foe->status2 |= REMASTER_EMERALD_STATUS2_CURSED;
            return 1;
        }
        battle_change_stage(battle, attacker, 3, -1, move->move_id);
        battle_change_stage(battle, attacker, 1, 1, move->move_id);
        battle_change_stage(battle, attacker, 2, 1, move->move_id);
        return 1;
    case EFFECT_ROAR: {
        uint8_t slot;
        if (foe->status3 & REMASTER_EMERALD_STATUS3_ROOTED)
            return 0;
        if (!battle_find_switch_slot(battle, foe->side, &slot))
            return 0;
        return remaster_emerald_battle_switch(
            battle, target, slot);
    }
    case EFFECT_CONVERSION: {
        uint8_t candidates[4];
        uint8_t candidate_count = 0;
        for (i = 0; i < 4; ++i) {
            const RemasterEmeraldMoveInfo *candidate =
                remaster_emerald_move_info(user->moves[i]);
            if (candidate != 0
                && user->moves[i] != 0
                && candidate->type != user->types[0])
                candidates[candidate_count++] = candidate->type;
        }
        if (candidate_count == 0)
            return 0;
        user->types[0] = candidates[
            remaster_emerald_battle_random(&battle->rng)
            % candidate_count];
        user->types[1] = user->types[0];
        return 1;
    }
    case EFFECT_TRANSFORM:
        user->types[0] = foe->types[0];
        user->types[1] = foe->types[1];
        user->ability = foe->ability;
        user->pokemon.attack = foe->pokemon.attack;
        user->pokemon.defense = foe->pokemon.defense;
        user->pokemon.speed = foe->pokemon.speed;
        user->pokemon.sp_attack = foe->pokemon.sp_attack;
        user->pokemon.sp_defense = foe->pokemon.sp_defense;
        memcpy(user->stat_stages, foe->stat_stages, sizeof(user->stat_stages));
        for (i = 0; i < 4; ++i) {
            user->moves[i] = foe->moves[i];
            user->pp[i] = foe->moves[i] != 0 ? 5u : 0u;
        }
        user->temporary_move_mask = 0x0Fu;
        user->status2 |= REMASTER_EMERALD_STATUS2_TRANSFORMED;
        return 1;
    case EFFECT_HAZE:
        for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
            if (battle->battlers[i].active)
                memset(
                    battle->battlers[i].stat_stages,
                    6,
                    sizeof(battle->battlers[i].stat_stages));
        }
        return 1;
    case EFFECT_RESTORE_HP:
    case EFFECT_SOFTBOILED:
        amount = user->pokemon.max_hp / 2u;
        if (amount == 0)
            amount = 1;
        battle_heal(battle, attacker, amount, move->move_id);
        return 1;
    case EFFECT_MORNING_SUN:
    case EFFECT_SYNTHESIS:
    case EFFECT_MOONLIGHT:
        if (battle_weather_has_effect(battle)) {
            if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SUN)
                amount = (uint16_t)(user->pokemon.max_hp * 2u / 3u);
            else if (battle->weather != REMASTER_EMERALD_BATTLE_WEATHER_NONE)
                amount = user->pokemon.max_hp / 4u;
            else
                amount = user->pokemon.max_hp / 2u;
        } else {
            amount = user->pokemon.max_hp / 2u;
        }
        if (amount == 0)
            amount = 1;
        battle_heal(battle, attacker, amount, move->move_id);
        return 1;
    case EFFECT_REST:
        if (user->pokemon.hp == user->pokemon.max_hp)
            return 0;
        user->pokemon.hp = user->pokemon.max_hp;
        user->pokemon.status = 2u;
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_HEAL,
            attacker,
            attacker,
            move->move_id,
            user->pokemon.max_hp,
            0);
        return 1;
    case EFFECT_LIGHT_SCREEN:
        battle->side_status[user->side] |=
            REMASTER_EMERALD_SIDE_LIGHT_SCREEN;
        return 1;
    case EFFECT_REFLECT:
        battle->side_status[user->side] |=
            REMASTER_EMERALD_SIDE_REFLECT;
        return 1;
    case EFFECT_MIST:
        battle->side_status[user->side] |= REMASTER_EMERALD_SIDE_MIST;
        return 1;
    case EFFECT_SAFEGUARD:
        battle->side_status[user->side] |= REMASTER_EMERALD_SIDE_SAFEGUARD;
        return 1;
    case EFFECT_FOCUS_ENERGY:
        user->status2 |= REMASTER_EMERALD_STATUS2_FOCUS_ENERGY;
        return 1;
    case EFFECT_SUBSTITUTE:
        amount = user->pokemon.max_hp / 4u;
        if (amount == 0
            || user->pokemon.hp <= amount
            || user->substitute_hp != 0)
            return 0;
        user->pokemon.hp = (uint16_t)(user->pokemon.hp - amount);
        user->substitute_hp = amount;
        return 1;
    case EFFECT_LEECH_SEED:
        if (foe->types[0] == TYPE_GRASS
            || foe->types[1] == TYPE_GRASS
            || (foe->status3 & REMASTER_EMERALD_STATUS3_LEECH_SEED))
            return 0;
        foe->status3 |= REMASTER_EMERALD_STATUS3_LEECH_SEED;
        return 1;
    case EFFECT_DISABLE:
        if (foe->last_move == 0)
            return 0;
        for (i = 0; i < 4; ++i) {
            if (foe->moves[i] == foe->last_move) {
                foe->disable_move_slot = i;
                foe->disable_turns =
                    (uint8_t)(2u
                        + remaster_emerald_battle_random(&battle->rng) % 5u);
                return 1;
            }
        }
        return 0;
    case EFFECT_ENCORE:
        if (foe->last_move == 0)
            return 0;
        for (i = 0; i < 4; ++i) {
            if (foe->moves[i] == foe->last_move) {
                foe->encore_move_slot = i;
                foe->encore_turns =
                    (uint8_t)(3u
                        + remaster_emerald_battle_random(&battle->rng) % 5u);
                return 1;
            }
        }
        return 0;
    case EFFECT_MEAN_LOOK:
        foe->status2 |= REMASTER_EMERALD_STATUS2_ESCAPE_PREVENTION;
        return 1;
    case EFFECT_FORESIGHT:
        foe->status2 |= REMASTER_EMERALD_STATUS2_FORESIGHT;
        return 1;
    case EFFECT_PERISH_SONG:
        for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
            if (battle->battlers[i].active
                && !battle->battlers[i].fainted
                && battle->battlers[i].perish_count == 0) {
                battle->battlers[i].status3 |=
                    REMASTER_EMERALD_STATUS3_PERISH_SONG;
                battle->battlers[i].perish_count = 3;
            }
        }
        return 1;
    case EFFECT_SANDSTORM:
        battle_set_weather(
            battle,
            REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM,
            5,
            attacker);
        return 1;
    case EFFECT_RAIN_DANCE:
        battle_set_weather(
            battle,
            REMASTER_EMERALD_BATTLE_WEATHER_RAIN,
            5,
            attacker);
        return 1;
    case EFFECT_SUNNY_DAY:
        battle_set_weather(
            battle,
            REMASTER_EMERALD_BATTLE_WEATHER_SUN,
            5,
            attacker);
        return 1;
    case EFFECT_HAIL:
        battle_set_weather(
            battle,
            REMASTER_EMERALD_BATTLE_WEATHER_HAIL,
            5,
            attacker);
        return 1;
    case EFFECT_PROTECT: {
        uint32_t denominator = UINT32_C(1) << user->protect_chain;
        if (denominator > 8)
            denominator = 8;
        if (remaster_emerald_battle_random(&battle->rng)
                % denominator
            != 0) {
            user->protect_chain = 0;
            return 0;
        }
        user->protected_turn = (uint8_t)(battle->turn_number & 0xFFu);
        if (user->protect_chain < 3)
            user->protect_chain++;
        return 1;
    }
    case EFFECT_SPIKES:
        if (battle->spikes_layers[foe->side] >= 3)
            return 0;
        battle->spikes_layers[foe->side]++;
        battle->side_status[foe->side] |= REMASTER_EMERALD_SIDE_SPIKES;
        return 1;
    case EFFECT_DEFENSE_CURL:
        user->status2 |= REMASTER_EMERALD_STATUS2_DEFENSE_CURL;
        return battle_change_stage(
            battle, attacker, 2, 1, move->move_id);
    case EFFECT_BELLY_DRUM:
        if (user->pokemon.hp <= user->pokemon.max_hp / 2u)
            return 0;
        user->pokemon.hp = (uint16_t)(
            user->pokemon.hp - user->pokemon.max_hp / 2u);
        user->stat_stages[1] = 12;
        return 1;
    case EFFECT_PSYCH_UP:
        memcpy(
            user->stat_stages,
            foe->stat_stages,
            sizeof(user->stat_stages));
        return 1;
    case EFFECT_TORMENT:
        foe->status2 |= REMASTER_EMERALD_STATUS2_TORMENT;
        return 1;
    case EFFECT_FLATTER:
        battle_change_stage(battle, target, 4, 1, move->move_id);
        return battle_apply_confusion(
            battle, attacker, target, move->move_id);
    case EFFECT_SWAGGER:
        battle_change_stage(battle, target, 1, 2, move->move_id);
        return battle_apply_confusion(
            battle, attacker, target, move->move_id);
    case EFFECT_TAUNT:
        foe->taunt_turns = 2;
        return 1;
    case EFFECT_TRICK: {
        uint16_t item = user->held_item;
        user->held_item = foe->held_item;
        foe->held_item = item;
        return 1;
    }
    case EFFECT_ROLE_PLAY:
        if (foe->ability == ABILITY_WONDER_GUARD)
            return 0;
        user->ability = foe->ability;
        return 1;
    case EFFECT_INGRAIN:
        if (user->status3 & REMASTER_EMERALD_STATUS3_ROOTED)
            return 0;
        user->status3 |= REMASTER_EMERALD_STATUS3_ROOTED;
        return 1;
    case EFFECT_SKILL_SWAP: {
        uint8_t ability;
        if (user->ability == ABILITY_WONDER_GUARD
            || foe->ability == ABILITY_WONDER_GUARD)
            return 0;
        ability = user->ability;
        user->ability = foe->ability;
        foe->ability = ability;
        return 1;
    }
    case EFFECT_REFRESH:
        user->pokemon.status &=
            ~(REMASTER_EMERALD_STATUS1_POISON
              | REMASTER_EMERALD_STATUS1_TOXIC
              | REMASTER_EMERALD_STATUS1_BURN
              | REMASTER_EMERALD_STATUS1_PARALYSIS);
        return 1;
    case EFFECT_MEMENTO:
        user->pokemon.hp = 0;
        battle_change_stage(battle, target, 1, -2, move->move_id);
        battle_change_stage(battle, target, 4, -2, move->move_id);
        return 1;
    case EFFECT_TICKLE:
        battle_change_stage(battle, target, 1, -1, move->move_id);
        battle_change_stage(battle, target, 2, -1, move->move_id);
        return 1;
    case EFFECT_COSMIC_POWER:
        battle_change_stage(battle, attacker, 2, 1, move->move_id);
        battle_change_stage(battle, attacker, 5, 1, move->move_id);
        return 1;
    case EFFECT_BULK_UP:
        battle_change_stage(battle, attacker, 1, 1, move->move_id);
        battle_change_stage(battle, attacker, 2, 1, move->move_id);
        return 1;
    case EFFECT_CALM_MIND:
        battle_change_stage(battle, attacker, 4, 1, move->move_id);
        battle_change_stage(battle, attacker, 5, 1, move->move_id);
        return 1;
    case EFFECT_DRAGON_DANCE:
        battle_change_stage(battle, attacker, 1, 1, move->move_id);
        battle_change_stage(battle, attacker, 3, 1, move->move_id);
        return 1;
    case EFFECT_MUD_SPORT:
        user->status3 |= REMASTER_EMERALD_STATUS3_MUD_SPORT;
        return 1;
    case EFFECT_WATER_SPORT:
        user->status3 |= REMASTER_EMERALD_STATUS3_WATER_SPORT;
        return 1;
    case EFFECT_HEAL_BELL:
        for (i = 0; i < battle->party_count[user->side]; ++i)
            battle->parties[user->side][i].status = 0;
        user->pokemon.status = 0;
        return 1;
    case EFFECT_TEETER_DANCE:
        for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
            if (i != attacker
                && battle->battlers[i].active
                && !battle->battlers[i].fainted)
                battle_apply_confusion(
                    battle, attacker, i, move->move_id);
        }
        return 1;
    case EFFECT_BATON_PASS:
        return 1;
    case EFFECT_CONVERSION_2: {
        const RemasterEmeraldMoveInfo *last =
            remaster_emerald_move_info(user->last_taken_move);
        uint8_t candidates[17];
        uint8_t count = 0;
        uint8_t type;
        if (last == 0)
            return 0;
        for (type = 0; type < 18; ++type) {
            uint8_t effectiveness;
            if (type == TYPE_MYSTERY)
                continue;
            effectiveness = remaster_emerald_battle_type_effectiveness(
                last->type,
                type);
            if (effectiveness <= 5
                && effectiveness != 0
                && type != user->types[0]
                && type != user->types[1])
                candidates[count++] = type;
        }
        if (count == 0)
            return 0;
        user->types[0] = candidates[
            remaster_emerald_battle_random(&battle->rng) % count];
        user->types[1] = user->types[0];
        return 1;
    }
    case EFFECT_ATTRACT: {
        uint8_t user_gender;
        uint8_t foe_gender;
        uint32_t bit;
        if (foe->ability == ABILITY_OBLIVIOUS)
            return 0;
        user_gender = battle_gender(user);
        foe_gender = battle_gender(foe);
        if (user_gender > 1
            || foe_gender > 1
            || user_gender == foe_gender)
            return 0;
        bit = UINT32_C(1) << (16u + attacker);
        if (foe->status2 & bit)
            return 0;
        foe->status2 |= bit;
        return 1;
    }
    case EFFECT_FOLLOW_ME:
        battle->follow_me_target[user->side] = attacker;
        battle->follow_me_turns[user->side] = 1;
        return 1;
    case EFFECT_HELPING_HAND: {
        uint8_t partner = battle_partner(attacker);
        if (!(battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE)
            || !battle_valid_battler(partner)
            || !battle->battlers[partner].active
            || battle->battlers[partner].fainted
            || battle->battlers[partner].helping_hand)
            return 0;
        battle->battlers[partner].helping_hand = 1;
        return 1;
    }
    case EFFECT_WISH:
        if (battle->wish_turns[attacker] != 0)
            return 0;
        battle->wish_turns[attacker] = 2;
        battle->wish_amount[attacker] =
            user->pokemon.max_hp / 2u;
        if (battle->wish_amount[attacker] == 0)
            battle->wish_amount[attacker] = 1;
        return 1;
    case EFFECT_RECYCLE:
        if (user->held_item != 0 || user->last_consumed_item == 0)
            return 0;
        user->held_item = user->last_consumed_item;
        user->last_consumed_item = 0;
        return 1;
    case EFFECT_IMPRISON:
        if (user->status3 & REMASTER_EMERALD_STATUS3_IMPRISONED_OTHERS)
            return 0;
        user->status3 |= REMASTER_EMERALD_STATUS3_IMPRISONED_OTHERS;
        user->imprison = 1;
        return 1;
    case EFFECT_MAGIC_COAT:
        if (user->magic_coat)
            return 0;
        user->magic_coat = 1;
        return 1;
    case EFFECT_SNATCH:
        if (user->snatch)
            return 0;
        user->snatch = 1;
        return 1;
    case EFFECT_CAMOUFLAGE:
        user->types[0] = TYPE_NORMAL;
        user->types[1] = TYPE_NORMAL;
        return 1;
    default:
        return move->power != 0;
    }
}

static int battle_can_act(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint16_t move_id)
{
    RemasterEmeraldBattleMon *mon;
    const RemasterEmeraldMoveInfo *move;
    uint32_t turns;
    uint16_t self_damage;
    int usable_while_asleep;

    if (battle == 0 || !battle_valid_battler(battler))
        return 0;

    mon = &battle->battlers[battler];
    move = remaster_emerald_move_info(move_id);
    usable_while_asleep = move != 0
        && (move->effect == EFFECT_SLEEP_TALK
            || move->effect == EFFECT_SNORE);

    if (mon->status2 & REMASTER_EMERALD_STATUS2_RECHARGE) {
        mon->status2 &= ~REMASTER_EMERALD_STATUS2_RECHARGE;
        return 0;
    }

    if (mon->status2 & REMASTER_EMERALD_STATUS2_FLINCHED) {
        mon->status2 &= ~REMASTER_EMERALD_STATUS2_FLINCHED;
        return 0;
    }

    turns = mon->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP;
    if (turns != 0) {
        turns--;
        mon->pokemon.status =
            (mon->pokemon.status & ~REMASTER_EMERALD_STATUS1_SLEEP)
            | turns;
        if (turns != 0) {
            if (!usable_while_asleep)
                return 0;
        } else {
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_STATUS,
                battler,
                battler,
                move_id,
                0,
                REMASTER_EMERALD_STATUS1_SLEEP);
            if (usable_while_asleep)
                return 0;
        }
    }

    if (mon->pokemon.status & REMASTER_EMERALD_STATUS1_FREEZE) {
        if (remaster_emerald_battle_random(&battle->rng) % 5u != 0)
            return 0;
        mon->pokemon.status &= ~REMASTER_EMERALD_STATUS1_FREEZE;
    }

    if ((mon->pokemon.status & REMASTER_EMERALD_STATUS1_PARALYSIS)
        && remaster_emerald_battle_random(&battle->rng) % 4u == 0)
        return 0;

    if ((mon->status2 & REMASTER_EMERALD_STATUS2_INFATUATION)
        && (remaster_emerald_battle_random(&battle->rng) & 1u) == 0)
        return 0;

    turns = mon->status2 & REMASTER_EMERALD_STATUS2_CONFUSION;
    if (turns != 0) {
        turns--;
        mon->status2 =
            (mon->status2 & ~REMASTER_EMERALD_STATUS2_CONFUSION)
            | turns;
        if (turns != 0
            && remaster_emerald_battle_random(&battle->rng) % 2u == 0) {
            uint32_t attack = battle_apply_stage(
                mon->pokemon.attack,
                mon->stat_stages[1]);
            uint32_t defense = battle_apply_stage(
                mon->pokemon.defense,
                mon->stat_stages[2]);
            uint32_t damage =
                (((2u * mon->pokemon.level / 5u + 2u)
                  * 40u * attack / defense)
                 / 50u)
                + 2u;
            if (damage == 0)
                damage = 1;
            if (damage > UINT16_MAX)
                damage = UINT16_MAX;
            self_damage = (uint16_t)damage;
            battle_damage_direct(
                battle,
                battler,
                battler,
                self_damage,
                move_id,
                0);
            return 0;
        }
    }

    return 1;
}

static void battle_clear_fainted_state(
    RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    RemasterEmeraldBattleMon *mon;
    uint8_t i;
    const uint32_t infatuation_bit =
        UINT32_C(1) << (16u + battler);

    if (battle == 0 || !battle_valid_battler(battler))
        return;

    mon = &battle->battlers[battler];
    mon->pokemon.status = 0;
    memset(mon->stat_stages, 6, sizeof(mon->stat_stages));
    mon->status2 = 0;
    mon->status3 = 0;
    mon->substitute_hp = 0;
    mon->last_move = 0;
    mon->last_taken_move = 0;
    mon->choice_locked_move = 0;
    mon->last_damage = 0;
    mon->bide_damage = 0;
    mon->trapped_move = 0;
    mon->last_damage_from = 0xFFu;
    mon->last_damage_type = 0;
    mon->last_damage_was_physical = 0;
    mon->last_damage_turn = 0;
    mon->protected_turn = 0xFFu;
    mon->endure_turn = 0xFFu;
    mon->protect_chain = 0;
    mon->stockpile = 0;
    mon->rollout_count = 0;
    mon->fury_cutter_count = 0;
    mon->trapped_turns = 0;
    mon->rampage_turns = 0;
    mon->uproar_turns = 0;
    mon->bide_turns = 0;
    mon->lock_on_turns = 0;
    mon->lock_on_target = 0xFFu;
    mon->perish_count = 0;
    mon->toxic_counter = 0;
    mon->flash_fire = 0;
    mon->taunt_turns = 0;
    mon->encore_turns = 0;
    mon->encore_move_slot = 0;
    mon->disable_turns = 0;
    mon->disable_move_slot = 0;
    mon->destiny_bond_turn = 0;
    mon->grudge_turn = 0;
    mon->charging_move = 0;
    mon->charging_move_slot = 0;
    mon->charging_target = 0xFFu;
    mon->helping_hand = 0;
    mon->magic_coat = 0;
    mon->snatch = 0;
    mon->imprison = 0;
    mon->temporary_move_mask = 0;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (i != battler)
            battle->battlers[i].status2 &= ~infatuation_bit;
    }

    battle_sync_battler(battle, battler);
}

static void battle_faint_check(
    RemasterEmeraldBattleState *battle,
    uint8_t battler);

static int battle_mon_knows_move(
    const RemasterEmeraldBattleMon *mon,
    uint16_t move_id)
{
    uint8_t slot;

    if (mon == 0 || move_id == 0)
        return 0;

    for (slot = 0; slot < REMASTER_EMERALD_MAX_MOVES; ++slot) {
        if (mon->moves[slot] == move_id)
            return 1;
    }
    return 0;
}

static int battle_evolution_method_checks_on_level_up(uint16_t method)
{
    switch (method) {
    case 1:  /* EVO_FRIENDSHIP */
    case 2:  /* EVO_FRIENDSHIP_DAY */
    case 3:  /* EVO_FRIENDSHIP_NIGHT */
    case 4:  /* EVO_LEVEL */
    case 8:  /* EVO_LEVEL_ATK_GT_DEF */
    case 9:  /* EVO_LEVEL_ATK_EQ_DEF */
    case 10: /* EVO_LEVEL_ATK_LT_DEF */
    case 11: /* EVO_LEVEL_SILCOON */
    case 12: /* EVO_LEVEL_CASCOON */
    case 13: /* EVO_LEVEL_NINJASK */
    case 14: /* EVO_LEVEL_SHEDINJA */
    case 15: /* EVO_BEAUTY */
        return 1;
    default:
        return 0;
    }
}

static void battle_emit_progression_handoffs(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t old_level,
    uint8_t new_level)
{
    RemasterEmeraldBattleMon *mon;
    const RemasterEmeraldLevelUpMove *learnset;
    const RemasterEmeraldEvolution *evolutions;
    size_t learnset_count = 0;
    size_t i;
    int evolution_check = 0;

    if (battle == 0
        || !battle_valid_battler(battler)
        || new_level <= old_level)
        return;

    mon = &battle->battlers[battler];
    learnset = remaster_emerald_species_level_up_moves(
        mon->species,
        &learnset_count);

    for (i = 0; i < learnset_count; ++i) {
        if (learnset[i].level <= old_level
            || learnset[i].level > new_level
            || learnset[i].move_id == 0
            || battle_mon_knows_move(mon, learnset[i].move_id))
            continue;

        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_MOVE_LEARN,
            battler,
            battler,
            learnset[i].move_id,
            learnset[i].level,
            mon->species);
    }

    evolutions = remaster_emerald_species_evolutions(mon->species);
    if (evolutions == 0)
        return;

    for (i = 0; i < REMASTER_EMERALD_EVOLUTIONS_PER_SPECIES; ++i) {
        if (evolutions[i].target_species != 0
            && battle_evolution_method_checks_on_level_up(
                evolutions[i].method)) {
            evolution_check = 1;
            break;
        }
    }

    if (evolution_check) {
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_EVOLUTION_CHECK,
            battler,
            battler,
            0,
            new_level,
            mon->species);
    }
}

static void battle_award_exp_for_faint(
    RemasterEmeraldBattleState *battle,
    uint8_t defeated_id)
{
    RemasterEmeraldBattleMon *defeated;
    uint8_t participant_count = 0;
    uint8_t i;

    if (battle == 0 || !battle_valid_battler(defeated_id))
        return;

    defeated = &battle->battlers[defeated_id];
    if (defeated->side != 1)
        return;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; i += 2) {
        if (battle->battlers[i].active && !battle->battlers[i].fainted)
            participant_count++;
    }
    if (participant_count == 0)
        participant_count = 1;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; i += 2) {
        RemasterEmeraldBattleMon *mon;
        const RemasterEmeraldSpeciesInfo *species;
        uint32_t exp;
        uint32_t old_exp;
        uint32_t new_exp;
        uint8_t old_level;
        uint8_t new_level;

        if (!battle->battlers[i].active || battle->battlers[i].fainted)
            continue;

        mon = &battle->battlers[i];
        exp = remaster_emerald_battle_exp_gain(
            defeated,
            participant_count,
            (battle->battle_type_flags
                & REMASTER_EMERALD_BATTLE_TYPE_TRAINER) != 0);
        if (battle_hold_effect(mon) == HOLD_EFFECT_LUCKY_EGG)
            exp = exp * 150u / 100u;

        old_exp = remaster_emerald_box_pokemon_experience(&mon->pokemon.box);
        new_exp = old_exp + exp;
        old_level = mon->pokemon.level;
        remaster_emerald_box_pokemon_set_experience(
            &mon->pokemon.box,
            new_exp);
        new_level = remaster_emerald_level_from_experience(
            mon->species,
            new_exp);
        mon->pokemon.level = new_level;
        battle->last_exp_gain = exp;
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_EXP,
            i,
            defeated_id,
            0,
            (int32_t)exp,
            0);

        if (new_level > old_level) {
            uint8_t ivs[6];
            uint8_t evs[6];
            RemasterEmeraldCalculatedStats stats;
            uint16_t old_max = mon->pokemon.max_hp;

            species = remaster_emerald_species_info(mon->species);
            if (species != 0) {
                remaster_emerald_box_pokemon_ivs(&mon->pokemon.box, ivs);
                remaster_emerald_box_pokemon_evs(&mon->pokemon.box, evs);
                if (remaster_emerald_calculate_stats(
                        mon->species,
                        new_level,
                        remaster_emerald_box_pokemon_nature(
                            &mon->pokemon.box),
                        ivs,
                        evs,
                        &stats)) {
                    mon->pokemon.max_hp = stats.hp;
                    mon->pokemon.hp = (uint16_t)(
                        mon->pokemon.hp + stats.hp - old_max);
                    mon->pokemon.attack = stats.attack;
                    mon->pokemon.defense = stats.defense;
                    mon->pokemon.speed = stats.speed;
                    mon->pokemon.sp_attack = stats.sp_attack;
                    mon->pokemon.sp_defense = stats.sp_defense;
                }
            }
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_LEVEL_UP,
                i,
                i,
                0,
                new_level,
                old_level);
            battle_emit_progression_handoffs(
                battle,
                i,
                old_level,
                new_level);
        }

        mon->pokemon.box.checksum =
            remaster_emerald_box_pokemon_checksum(&mon->pokemon.box);
        battle_sync_battler(battle, i);
    }
}

static void battle_finish_if_over(
    RemasterEmeraldBattleState *battle)
{
    int player_alive;
    int opponent_alive;

    if (battle == 0 || battle->ended)
        return;

    player_alive = battle_party_has_alive(battle, 0);
    opponent_alive = battle_party_has_alive(battle, 1);

    if (player_alive && opponent_alive)
        return;

    battle->ended = 1;
    if (player_alive)
        battle->outcome = REMASTER_EMERALD_BATTLE_OUTCOME_WON;
    else if (opponent_alive)
        battle->outcome = REMASTER_EMERALD_BATTLE_OUTCOME_LOST;
    else
        battle->outcome = REMASTER_EMERALD_BATTLE_OUTCOME_DREW;

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_ENDED,
        0,
        0,
        0,
        battle->outcome,
        0);
}

static void battle_faint_check(
    RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    RemasterEmeraldBattleMon *mon;

    if (battle == 0 || !battle_valid_battler(battler))
        return;

    mon = &battle->battlers[battler];
    if (!mon->active || mon->fainted || mon->pokemon.hp != 0)
        return;

    mon->fainted = 1;
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_FAINT,
        battler,
        battler,
        0,
        0,
        0);

    if ((mon->status2 & REMASTER_EMERALD_STATUS2_DESTINY_BOND)
        && mon->destiny_bond_turn
            == (uint8_t)(battle->turn_number & 0xFFu)
        && battle_valid_battler(mon->last_damage_from)
        && mon->last_damage_from != battler
        && !battle->battlers[mon->last_damage_from].fainted) {
        battle->battlers[mon->last_damage_from].pokemon.hp = 0;
        battle_faint_check(battle, mon->last_damage_from);
    }

    if ((mon->status3 & REMASTER_EMERALD_STATUS3_GRUDGE)
        && mon->grudge_turn
            == (uint8_t)(battle->turn_number & 0xFFu)
        && battle_valid_battler(mon->last_damage_from)
        && mon->last_damage_from != battler) {
        RemasterEmeraldBattleMon *killer =
            &battle->battlers[mon->last_damage_from];
        uint8_t slot;
        for (slot = 0; slot < 4; ++slot) {
            if (killer->moves[slot] == killer->last_move) {
                killer->pp[slot] = 0;
                remaster_emerald_box_pokemon_set_move(
                    &killer->pokemon.box,
                    slot,
                    killer->moves[slot],
                    0);
                break;
            }
        }
    }

    battle_clear_fainted_state(battle, battler);
    battle_award_exp_for_faint(battle, battler);
    battle_finish_if_over(battle);
}

static int battle_execute_beat_up(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker_id,
    uint8_t target_id,
    const RemasterEmeraldMoveInfo *move)
{
    RemasterEmeraldBattleMon *attacker;
    RemasterEmeraldBattleMon *target;
    const RemasterEmeraldSpeciesInfo *target_species;
    uint8_t party_slot;
    int hit_any = 0;

    if (battle == 0
        || move == 0
        || !battle_valid_battler(attacker_id)
        || !battle_valid_battler(target_id))
        return 0;

    attacker = &battle->battlers[attacker_id];
    target = &battle->battlers[target_id];
    target_species = remaster_emerald_species_info(target->species);
    if (target_species == 0 || target_species->base_defense == 0)
        return 0;

    for (party_slot = 0;
         party_slot < battle->party_count[attacker->side]
            && !target->fainted;
         ++party_slot) {
        const RemasterEmeraldPartyPokemon *party_mon =
            &battle->parties[attacker->side][party_slot];
        const uint16_t species_id =
            remaster_emerald_box_pokemon_species(&party_mon->box);
        const RemasterEmeraldSpeciesInfo *species;
        uint32_t damage;

        if (species_id == 0
            || party_mon->hp == 0
            || party_mon->status != 0)
            continue;

        species = remaster_emerald_species_info(species_id);
        if (species == 0)
            continue;

        damage = (uint32_t)species->base_attack
            * move->power
            * (2u * party_mon->level / 5u + 2u);
        damage /= target_species->base_defense;
        damage = damage / 50u + 2u;

        if (attacker->helping_hand)
            damage = damage * 15u / 10u;
        if (battle_critical(
                battle,
                attacker,
                target,
                move->effect))
            damage *= 2u;

        damage = damage
            * (85u + remaster_emerald_battle_random(&battle->rng) % 16u)
            / 100u;
        if (damage == 0)
            damage = 1;
        if (damage > UINT16_MAX)
            damage = UINT16_MAX;

        battle_damage_direct(
            battle,
            attacker_id,
            target_id,
            (uint16_t)damage,
            move->move_id,
            0);
        hit_any = 1;
        battle_faint_check(battle, target_id);
    }

    return hit_any;
}

int remaster_emerald_battle_use_move(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker_id,
    uint8_t target_id,
    uint8_t move_slot)
{
    RemasterEmeraldBattleMon *attacker;
    RemasterEmeraldBattleMon *target;
    const RemasterEmeraldMoveInfo *move;
    RemasterEmeraldBattleDamageResult result;
    uint16_t move_id;
    uint8_t hits = 1;
    uint8_t hit;
    uint32_t total_damage = 0;
    uint8_t pp_cost = 1;
    int direct_effect_result = 0;
    int false_swipe;
    int continuing_charge;
    int continuing_repeat;
    int move_landed = 0;

    if (battle == 0
        || battle->ended
        || !battle_valid_battler(attacker_id)
        || move_slot >= 4)
        return 0;

    attacker = &battle->battlers[attacker_id];
    if (!attacker->active || attacker->fainted)
        return 0;

    if (!battle_valid_battler(target_id)
        || !battle->battlers[target_id].active
        || battle->battlers[target_id].fainted
        || battle_side(target_id) == attacker->side)
        target_id = battle_default_target(battle, attacker_id);
    if (!battle_valid_battler(target_id))
        return 0;

    if (battle->battlers[target_id].side != attacker->side) {
        uint8_t target_side = battle->battlers[target_id].side;
        uint8_t follow = battle->follow_me_target[target_side];
        if (battle->follow_me_turns[target_side] != 0
            && battle_valid_battler(follow)
            && battle->battlers[follow].active
            && !battle->battlers[follow].fainted)
            target_id = follow;
    }

    target = &battle->battlers[target_id];
    move_id = attacker->moves[move_slot];
    move = remaster_emerald_move_info(move_id);
    continuing_charge = attacker->charging_move == move_id
        && attacker->charging_move_slot == move_slot;
    continuing_repeat =
        (move->effect == EFFECT_RAMPAGE && attacker->rampage_turns != 0)
        || (move->effect == EFFECT_UPROAR && attacker->uproar_turns != 0);
    (void)battle_effect_is_plain_hit(move->effect);
    if (move == 0
        || move_id == 0
        || (!continuing_charge
            && battle->called_move_depth == 0
            && attacker->pp[move_slot] == 0))
        return 0;

    if (!continuing_charge
        && !continuing_repeat
        && battle->called_move_depth == 0) {
        uint8_t enemy;
        for (enemy = 0;
             enemy < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
             ++enemy) {
            uint8_t slot;
            if (!battle->battlers[enemy].active
                || battle->battlers[enemy].fainted
                || battle->battlers[enemy].side == attacker->side
                || !(battle->battlers[enemy].status3
                    & REMASTER_EMERALD_STATUS3_IMPRISONED_OTHERS))
                continue;
            for (slot = 0; slot < 4; ++slot) {
                if (battle->battlers[enemy].moves[slot] == move_id)
                    return 0;
            }
        }
    }

    if (!continuing_charge
        && !continuing_repeat
        && battle->called_move_depth == 0
        && attacker->disable_turns != 0
        && attacker->disable_move_slot == move_slot)
        return 0;
    if (!continuing_charge
        && !continuing_repeat
        && battle->called_move_depth == 0
        && attacker->encore_turns != 0
        && attacker->encore_move_slot != move_slot)
        return 0;
    if (!continuing_charge
        && !continuing_repeat
        && battle->called_move_depth == 0
        && attacker->taunt_turns != 0
        && move->power == 0)
        return 0;
    if (!continuing_charge
        && !continuing_repeat
        && battle->called_move_depth == 0
        && battle_hold_effect(attacker) == HOLD_EFFECT_CHOICE_BAND
        && attacker->choice_locked_move != 0
        && attacker->choice_locked_move != move_id)
        return 0;

    if (!continuing_charge && battle->called_move_depth == 0) {
        if (target->ability == ABILITY_PRESSURE)
            pp_cost = 2;
        if (pp_cost > attacker->pp[move_slot])
            pp_cost = attacker->pp[move_slot];
        attacker->pp[move_slot] =
            (uint8_t)(attacker->pp[move_slot] - pp_cost);
        if (!(attacker->temporary_move_mask & (1u << move_slot)))
            remaster_emerald_box_pokemon_set_move(
                &attacker->pokemon.box,
                move_slot,
                move_id,
                attacker->pp[move_slot]);
    } else {
        pp_cost = 0;
    }
    if (move->effect != EFFECT_RAGE)
        attacker->status2 &= ~REMASTER_EMERALD_STATUS2_RAGE;
    attacker->last_move = move_id;
    if (battle_hold_effect(attacker) == HOLD_EFFECT_CHOICE_BAND
        && attacker->choice_locked_move == 0)
        attacker->choice_locked_move = move_id;

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED,
        attacker_id,
        target_id,
        move_id,
        move_slot,
        pp_cost);

    if (battle->called_move_depth == 0
        && !battle_can_act(battle, attacker_id, move_id)) {
        if (continuing_charge)
            battle_clear_charge(attacker);
        battle_faint_check(battle, attacker_id);
        return 1;
    }

    if (continuing_charge) {
        battle_clear_charge(attacker);
    } else if (battle_is_two_turn_effect(move)
        && !(move->effect == EFFECT_SOLAR_BEAM
            && battle_weather_has_effect(battle)
            && battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SUN)) {
        attacker->charging_move = move_id;
        attacker->charging_move_slot = move_slot;
        attacker->charging_target = target_id;
        attacker->status2 |= REMASTER_EMERALD_STATUS2_MULTIPLETURNS;
        attacker->status3 |= battle_semi_invulnerable_flag(move_id);
        if (move->effect == EFFECT_SKULL_BASH)
            battle_change_stage(
                battle,
                attacker_id,
                2,
                1,
                move_id);
        battle_sync_battler(battle, attacker_id);
        return 1;
    }

    if (move->effect == EFFECT_SNORE
        && !(attacker->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP))
        return 1;

    if (move->effect == EFFECT_FOCUS_PUNCH
        && attacker->last_damage_turn
            == (uint8_t)(battle->turn_number & 0xFFu)
        && attacker->last_damage != 0)
        return 1;

    if (move->effect == EFFECT_MIRROR_MOVE) {
        uint16_t called = attacker->last_taken_move;
        if (called == 0 || battle_move_forbidden_metronome(called))
            return 1;
        return battle_call_move(
            battle,
            attacker_id,
            target_id,
            move_slot,
            called);
    }

    if (move->effect == EFFECT_METRONOME) {
        uint16_t called = 0;
        uint32_t attempts;
        for (attempts = 0; attempts < 2048u; ++attempts) {
            uint16_t candidate = (uint16_t)(
                (remaster_emerald_battle_random(&battle->rng) & 0x01FFu)
                + 1u);
            if (remaster_emerald_move_info(candidate) != 0
                && !battle_move_forbidden_metronome(candidate)) {
                called = candidate;
                break;
            }
        }
        if (called == 0)
            return 1;
        return battle_call_move(
            battle,
            attacker_id,
            target_id,
            move_slot,
            called);
    }

    if (move->effect == EFFECT_SLEEP_TALK) {
        uint8_t candidates[4];
        uint8_t count = 0;
        uint8_t i;
        if (!(attacker->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP))
            return 1;
        for (i = 0; i < 4; ++i) {
            if (!battle_move_invalid_for_sleep_assist(attacker->moves[i]))
                candidates[count++] = i;
        }
        if (count == 0)
            return 1;
        {
            uint8_t chosen = candidates[
                remaster_emerald_battle_random(&battle->rng) % count];
            return battle_call_move(
                battle,
                attacker_id,
                target_id,
                move_slot,
                attacker->moves[chosen]);
        }
    }

    if (move->effect == EFFECT_ASSIST) {
        uint16_t candidates[24];
        uint8_t count = 0;
        uint8_t p;
        for (p = 0; p < battle->party_count[attacker->side]; ++p) {
            uint16_t party_moves[4];
            uint8_t party_pp[4];
            uint8_t i;
            if (p == attacker->party_slot)
                continue;
            if (remaster_emerald_box_pokemon_species(
                    &battle->parties[attacker->side][p].box) == 0)
                continue;
            remaster_emerald_box_pokemon_moves(
                &battle->parties[attacker->side][p].box,
                party_moves,
                party_pp);
            for (i = 0; i < 4; ++i) {
                uint16_t candidate = party_moves[i];
                if (candidate != 0
                    && !battle_move_invalid_for_sleep_assist(candidate)
                    && !battle_move_forbidden_metronome(candidate)
                    && count < 24)
                    candidates[count++] = candidate;
            }
        }
        if (count == 0)
            return 1;
        return battle_call_move(
            battle,
            attacker_id,
            target_id,
            move_slot,
            candidates[
                ((uint32_t)(remaster_emerald_battle_random(&battle->rng)
                    & 0xFFu)
                 * count)
                >> 8u]);
    }

    if (move->effect == EFFECT_NATURE_POWER)
        return battle_call_move(
            battle,
            attacker_id,
            target_id,
            move_slot,
            battle_nature_power_move(battle->terrain));

    if (move->effect == EFFECT_MIMIC) {
        const RemasterEmeraldMoveInfo *copied;
        if (target->last_move == 0
            || target->last_move == MOVE_METRONOME
            || target->last_move == MOVE_STRUGGLE
            || target->last_move == MOVE_SKETCH
            || target->last_move == MOVE_MIMIC)
            return 1;
        copied = remaster_emerald_move_info(target->last_move);
        if (copied == 0)
            return 1;
        attacker->moves[move_slot] = target->last_move;
        attacker->pp[move_slot] = copied->pp < 5u ? copied->pp : 5u;
        return 1;
    }

    if (move->effect == EFFECT_SKETCH) {
        const RemasterEmeraldMoveInfo *copied;
        if (target->last_move == 0
            || target->last_move == MOVE_STRUGGLE
            || target->last_move == MOVE_SKETCH)
            return 1;
        copied = remaster_emerald_move_info(target->last_move);
        if (copied == 0)
            return 1;
        attacker->moves[move_slot] = target->last_move;
        attacker->pp[move_slot] = copied->pp;
        remaster_emerald_box_pokemon_set_move(
            &attacker->pokemon.box,
            move_slot,
            target->last_move,
            copied->pp);
        return 1;
    }

    if (move->effect == EFFECT_DREAM_EATER
        && !(target->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP)) {
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_MESSAGE,
            attacker_id,
            target_id,
            move_id,
            0,
            EFFECT_DREAM_EATER);
        return 1;
    }

    if (move->effect == EFFECT_FAKE_OUT
        && attacker->entered_turn
            != (uint8_t)(battle->turn_number & 0xFFu)) {
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_MESSAGE,
            attacker_id,
            target_id,
            move_id,
            0,
            EFFECT_FAKE_OUT);
        return 1;
    }

    if (move->effect == EFFECT_TELEPORT) {
        if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_TRAINER)
            return 1;
        battle->outcome =
            REMASTER_EMERALD_BATTLE_OUTCOME_PLAYER_TELEPORTED;
        battle->ended = 1;
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_ENDED,
            attacker_id,
            target_id,
            move_id,
            battle->outcome,
            0);
        return 1;
    }

    if (move->effect == EFFECT_FUTURE_SIGHT) {
        RemasterEmeraldBattleDamageResult future_result;
        if (battle->future_turns[target_id] != 0)
            return 1;
        if (!remaster_emerald_battle_calculate_damage(
                battle,
                attacker_id,
                target_id,
                move_id,
                &future_result)
            || !future_result.hit
            || future_result.immune)
            return 1;
        battle->future_turns[target_id] = 3;
        battle->future_attacker[target_id] = attacker_id;
        battle->future_move[target_id] = move_id;
        battle->future_damage[target_id] =
            future_result.damage != 0 ? future_result.damage : 1u;
        return 1;
    }

    if (move->effect == EFFECT_COUNTER
        || move->effect == EFFECT_MIRROR_COAT) {
        uint8_t needs_physical = move->effect == EFFECT_COUNTER ? 1u : 0u;
        uint8_t effectiveness;
        uint32_t reflected;

        if (attacker->last_damage_turn
                != (uint8_t)(battle->turn_number & 0xFFu)
            || attacker->last_damage_from != target_id
            || attacker->last_damage == 0
            || attacker->last_damage_was_physical != needs_physical)
            return 1;

        effectiveness = battle_type_multiplier(target, move->type);
        if (effectiveness == 0)
            return 1;

        /*
         * Emerald Cmd_typecalc2 only rejects type immunity for Counter and
         * Mirror Coat. It records resistance/super-effective flags but does
         * not scale the fixed reflected damage.
         */
        reflected = (uint32_t)attacker->last_damage * 2u;
        if (reflected == 0)
            reflected = 1;
        if (reflected > UINT16_MAX)
            reflected = UINT16_MAX;
        battle_damage_direct(
            battle,
            attacker_id,
            target_id,
            (uint16_t)reflected,
            move_id,
            0);
        battle_faint_check(battle, target_id);
        battle_sync_battler(battle, attacker_id);
        battle_sync_battler(battle, target_id);
        return 1;
    }

    if (target->protected_turn == (uint8_t)(battle->turn_number & 0xFFu)
        && (move->flags & (1u << 1)) != 0) {
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_MESSAGE,
            attacker_id,
            target_id,
            move_id,
            0,
            EFFECT_PROTECT);
        return 1;
    }

    if (move->effect == EFFECT_BEAT_UP) {
        uint8_t accuracy = battle_accuracy_percent(
            battle, attacker, target, move);
        if (remaster_emerald_battle_random(&battle->rng) % 100u
            >= accuracy) {
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_MOVE_MISSED,
                attacker_id,
                target_id,
                move_id,
                0,
                0);
            return 1;
        }
        battle_execute_beat_up(
            battle,
            attacker_id,
            target_id,
            move);
        target->last_taken_move = move_id;
        battle_sync_battler(battle, attacker_id);
        battle_sync_battler(battle, target_id);
        return 1;
    }

    if (move->effect == EFFECT_BEAT_UP) {
        uint8_t party_slot;
        uint8_t accuracy = battle_accuracy_percent(
            battle,
            attacker,
            target,
            move);

        if (remaster_emerald_battle_random(&battle->rng) % 100u
            >= accuracy) {
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_MOVE_MISSED,
                attacker_id,
                target_id,
                move_id,
                0,
                0);
            return 1;
        }

        for (party_slot = 0;
             party_slot < battle->party_count[attacker->side]
                 && !target->fainted;
             ++party_slot) {
            const RemasterEmeraldPartyPokemon *member =
                &battle->parties[attacker->side][party_slot];
            const uint16_t member_species =
                remaster_emerald_box_pokemon_species(&member->box);
            const RemasterEmeraldSpeciesInfo *member_info;
            const RemasterEmeraldSpeciesInfo *target_info;
            uint32_t damage;
            int critical;

            if (member_species == 0
                || member->hp == 0
                || member->status != 0)
                continue;

            member_info = remaster_emerald_species_info(member_species);
            target_info = remaster_emerald_species_info(target->species);
            if (member_info == 0
                || target_info == 0
                || target_info->base_defense == 0)
                continue;

            damage = member_info->base_attack;
            damage *= move->power;
            damage *= (2u * member->level / 5u + 2u);
            damage /= target_info->base_defense;
            damage = damage / 50u + 2u;

            if (attacker->helping_hand)
                damage = damage * 15u / 10u;

            critical = battle_critical(
                battle,
                attacker,
                target,
                move->effect);
            if (critical) {
                damage *= 2u;
                battle_event(
                    battle,
                    REMASTER_EMERALD_BATTLE_EVENT_CRITICAL,
                    attacker_id,
                    target_id,
                    move_id,
                    1,
                    party_slot);
            }

            damage = damage
                * (85u
                    + remaster_emerald_battle_random(&battle->rng) % 16u)
                / 100u;
            if (damage == 0)
                damage = 1;
            if (damage > UINT16_MAX)
                damage = UINT16_MAX;

            battle_damage_direct(
                battle,
                attacker_id,
                target_id,
                (uint16_t)damage,
                move_id,
                0);
            move_landed = 1;
            battle_faint_check(battle, target_id);
        }

        if (move_landed)
            target->last_taken_move = move_id;
        battle_sync_battler(battle, attacker_id);
        battle_sync_battler(battle, target_id);
        return 1;
    }

    if (move->effect == EFFECT_MULTI_HIT)
        hits = (uint8_t)(2u + remaster_emerald_battle_random(&battle->rng) % 4u);
    else if (move->effect == EFFECT_DOUBLE_HIT
        || move->effect == EFFECT_TWINEEDLE)
        hits = 2;
    else if (move->effect == EFFECT_TRIPLE_KICK)
        hits = 3;

    for (hit = 0; hit < hits && !target->fainted; ++hit) {
        uint16_t damage = 0;

        if (!remaster_emerald_battle_calculate_damage(
                battle,
                attacker_id,
                target_id,
                move_id,
                &result))
            return 0;

        if (!result.hit) {
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_MOVE_MISSED,
                attacker_id,
                target_id,
                move_id,
                0,
                0);
            if (hit == 0 && move->effect == EFFECT_RECOIL_IF_MISS) {
                uint8_t saved_lock_turns = attacker->lock_on_turns;
                uint8_t saved_lock_target = attacker->lock_on_target;
                RemasterEmeraldBattleDamageResult crash_result;
                uint16_t crash;

                attacker->lock_on_turns = 1;
                attacker->lock_on_target = target_id;
                if (remaster_emerald_battle_calculate_damage(
                        battle,
                        attacker_id,
                        target_id,
                        move_id,
                        &crash_result)
                    && crash_result.damage != 0) {
                    crash = (uint16_t)(crash_result.damage / 2u);
                    if (crash == 0)
                        crash = 1;
                    if (crash > target->pokemon.max_hp / 2u)
                        crash = target->pokemon.max_hp / 2u;
                    if (crash == 0)
                        crash = 1;
                    battle_damage_direct(
                        battle,
                        attacker_id,
                        attacker_id,
                        crash,
                        move_id,
                        0);
                    battle_faint_check(battle, attacker_id);
                }
                attacker->lock_on_turns = saved_lock_turns;
                attacker->lock_on_target = saved_lock_target;
                return 1;
            }
            if (hit == 0)
                return 1;
            break;
        }

        move_landed = 1;

        if (result.immune) {
            if (target->ability == ABILITY_VOLT_ABSORB
                && move->type == TYPE_ELECTRIC) {
                battle_heal(
                    battle,
                    target_id,
                    target->pokemon.max_hp / 4u,
                    move_id);
            } else if (target->ability == ABILITY_WATER_ABSORB
                && move->type == TYPE_WATER) {
                battle_heal(
                    battle,
                    target_id,
                    target->pokemon.max_hp / 4u,
                    move_id);
            } else if (target->ability == ABILITY_FLASH_FIRE
                && move->type == TYPE_FIRE) {
                target->flash_fire = 1;
            }
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_EFFECTIVENESS,
                attacker_id,
                target_id,
                move_id,
                0,
                0);
            return 1;
        }

        if (result.critical) {
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_CRITICAL,
                attacker_id,
                target_id,
                move_id,
                1,
                0);
        }
        if (result.effectiveness_tenths != 10) {
            battle_event(
                battle,
                REMASTER_EMERALD_BATTLE_EVENT_EFFECTIVENESS,
                attacker_id,
                target_id,
                move_id,
                result.effectiveness_tenths,
                0);
        }

        switch (move->effect) {
        case EFFECT_SUPER_FANG:
            damage = target->pokemon.hp / 2u;
            if (damage == 0)
                damage = 1;
            break;
        case EFFECT_DRAGON_RAGE:
            damage = 40;
            break;
        case EFFECT_SONICBOOM:
            damage = 20;
            break;
        case EFFECT_LEVEL_DAMAGE:
            damage = attacker->pokemon.level;
            break;
        case EFFECT_PSYWAVE:
            damage = (uint16_t)(
                attacker->pokemon.level
                * (50u + remaster_emerald_battle_random(&battle->rng) % 101u)
                / 100u);
            if (damage == 0)
                damage = 1;
            break;
        case EFFECT_OHKO:
            if (attacker->pokemon.level < target->pokemon.level)
                return 1;
            damage = target->pokemon.hp;
            break;
        case EFFECT_ENDEAVOR:
            if (target->pokemon.hp <= attacker->pokemon.hp)
                return 1;
            damage = (uint16_t)(target->pokemon.hp - attacker->pokemon.hp);
            break;
        case EFFECT_PRESENT: {
            uint16_t roll =
                remaster_emerald_battle_random(&battle->rng) % 100u;
            if (roll < 40)
                damage = 40;
            else if (roll < 70)
                damage = 80;
            else if (roll < 80)
                damage = 120;
            else {
                battle_heal(
                    battle,
                    target_id,
                    target->pokemon.max_hp / 4u,
                    move_id);
                return 1;
            }
            break;
        }
        case EFFECT_SPIT_UP:
            if (attacker->stockpile == 0)
                return 1;
            damage = result.damage;
            break;
        default:
            damage = result.damage;
            break;
        }

        false_swipe = move->effect == EFFECT_FALSE_SWIPE;
        battle_damage_direct(
            battle,
            attacker_id,
            target_id,
            damage,
            move_id,
            false_swipe);
        total_damage += target->last_damage;

        if (move->effect == EFFECT_THAW_HIT)
            target->pokemon.status &= ~REMASTER_EMERALD_STATUS1_FREEZE;

        battle_faint_check(battle, target_id);
        if (target->fainted)
            break;
    }

    if (move->power == 0)
        direct_effect_result = battle_apply_primary_effect(
            battle,
            attacker_id,
            target_id,
            move);
    else
        battle_secondary_effect(
            battle,
            attacker_id,
            target_id,
            move);

    if ((move->effect == EFFECT_ABSORB
            || move->effect == EFFECT_DREAM_EATER)
        && total_damage != 0)
        battle_heal(
            battle,
            attacker_id,
            (uint16_t)(total_damage / 2u),
            move_id);

    if (move->effect == EFFECT_TRAP
        && total_damage != 0
        && !target->fainted) {
        target->status2 |= REMASTER_EMERALD_STATUS2_WRAPPED;
        target->trapped_turns = (uint8_t)(
            2u + remaster_emerald_battle_random(&battle->rng) % 4u);
        target->trapped_move = move_id;
    }

    if (move->effect == EFFECT_FAKE_OUT
        && total_damage != 0
        && target->ability != ABILITY_INNER_FOCUS)
        target->status2 |= REMASTER_EMERALD_STATUS2_FLINCHED;

    if (move->effect == EFFECT_RAPID_SPIN && total_damage != 0) {
        attacker->status2 &= ~REMASTER_EMERALD_STATUS2_WRAPPED;
        attacker->trapped_turns = 0;
        attacker->trapped_move = 0;
        attacker->status3 &= ~REMASTER_EMERALD_STATUS3_LEECH_SEED;
        battle->spikes_layers[attacker->side] = 0;
        battle->side_status[attacker->side] &=
            ~REMASTER_EMERALD_SIDE_SPIKES;
    }

    if (move->effect == EFFECT_PAY_DAY && total_damage != 0)
        battle->money_reward +=
            (uint32_t)attacker->pokemon.level * 5u;

    if (move->effect == EFFECT_ROLLOUT && total_damage != 0) {
        if (attacker->rollout_count < 4)
            attacker->rollout_count++;
    } else {
        attacker->rollout_count = 0;
    }

    if (move->effect == EFFECT_FURY_CUTTER && total_damage != 0) {
        if (attacker->fury_cutter_count < 4)
            attacker->fury_cutter_count++;
    } else {
        attacker->fury_cutter_count = 0;
    }

    if (move->effect == EFFECT_SPIT_UP)
        attacker->stockpile = 0;

    if (move->type == TYPE_ELECTRIC
        && (attacker->status3 & REMASTER_EMERALD_STATUS3_CHARGED_UP))
        attacker->status3 &= ~REMASTER_EMERALD_STATUS3_CHARGED_UP;

    if (move_landed && move->effect == EFFECT_RAGE)
        attacker->status2 |= REMASTER_EMERALD_STATUS2_RAGE;

    if (move_landed && move->effect == EFFECT_RAMPAGE) {
        if (attacker->rampage_turns == 0) {
            attacker->rampage_turns = (uint8_t)(
                2u + remaster_emerald_battle_random(&battle->rng) % 2u);
            attacker->status2 |=
                REMASTER_EMERALD_STATUS2_MULTIPLETURNS
                | REMASTER_EMERALD_STATUS2_LOCK_CONFUSE;
        }
        attacker->rampage_turns--;
        if (attacker->rampage_turns == 0) {
            attacker->status2 &=
                ~(REMASTER_EMERALD_STATUS2_MULTIPLETURNS
                  | REMASTER_EMERALD_STATUS2_LOCK_CONFUSE);
            battle_apply_confusion(
                battle, attacker_id, attacker_id, move_id);
        }
    }

    if (move_landed && move->effect == EFFECT_UPROAR) {
        uint8_t awake;
        if (attacker->uproar_turns == 0) {
            attacker->uproar_turns = (uint8_t)(
                2u + remaster_emerald_battle_random(&battle->rng) % 4u);
            attacker->status2 |=
                REMASTER_EMERALD_STATUS2_MULTIPLETURNS
                | REMASTER_EMERALD_STATUS2_UPROAR;
        }
        for (awake = 0;
             awake < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
             ++awake) {
            RemasterEmeraldBattleMon *woken = &battle->battlers[awake];
            if (woken->active
                && !woken->fainted
                && woken->ability != ABILITY_SOUNDPROOF
                && (woken->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP))
                woken->pokemon.status &=
                    ~REMASTER_EMERALD_STATUS1_SLEEP;
        }
        attacker->uproar_turns--;
        if (attacker->uproar_turns == 0)
            attacker->status2 &=
                ~(REMASTER_EMERALD_STATUS2_MULTIPLETURNS
                  | REMASTER_EMERALD_STATUS2_UPROAR);
    }

    if ((move->effect == EFFECT_RECOIL
            || move->effect == EFFECT_DOUBLE_EDGE)
        && total_damage != 0) {
        uint16_t recoil = (uint16_t)(total_damage / 4u);
        if (recoil == 0)
            recoil = 1;
        battle_damage_direct(
            battle,
            attacker_id,
            attacker_id,
            recoil,
            move_id,
            0);
    }

    if (battle_hold_effect(attacker) == HOLD_EFFECT_SHELL_BELL
        && total_damage != 0) {
        uint16_t heal = (uint16_t)(total_damage / 8u);
        if (heal == 0)
            heal = 1;
        battle_heal(battle, attacker_id, heal, move_id);
    }

    if (move->effect == EFFECT_EXPLOSION) {
        attacker->pokemon.hp = 0;
        battle_faint_check(battle, attacker_id);
    } else if (move->effect == EFFECT_RECHARGE
        && !target->fainted) {
        attacker->status2 |= REMASTER_EMERALD_STATUS2_RECHARGE;
    } else if (move->effect == EFFECT_SUPERPOWER) {
        battle_change_stage(battle, attacker_id, 1, -1, move_id);
        battle_change_stage(battle, attacker_id, 2, -1, move_id);
    } else if (move->effect == EFFECT_OVERHEAT) {
        battle_change_stage(battle, attacker_id, 4, -2, move_id);
    } else if (move->effect == EFFECT_BRICK_BREAK) {
        battle->side_status[target->side] &=
            ~(REMASTER_EMERALD_SIDE_REFLECT
              | REMASTER_EMERALD_SIDE_LIGHT_SCREEN);
    } else if (move->effect == EFFECT_KNOCK_OFF) {
        target->held_item = 0;
    } else if (move->effect == EFFECT_THIEF
        && attacker->held_item == 0
        && target->held_item != 0) {
        attacker->held_item = target->held_item;
        target->held_item = 0;
    } else if (move->effect == EFFECT_SMELLINGSALT
        && (target->pokemon.status & REMASTER_EMERALD_STATUS1_PARALYSIS)) {
        target->pokemon.status &= ~REMASTER_EMERALD_STATUS1_PARALYSIS;
    }

    battle_contact_ability(battle, attacker_id, target_id, move);
    battle_faint_check(battle, attacker_id);
    battle_faint_check(battle, target_id);

    if (move->power == 0 && !direct_effect_result) {
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_MESSAGE,
            attacker_id,
            target_id,
            move_id,
            0,
            0);
    }

    if (move_landed && target_id != attacker_id)
        target->last_taken_move = move_id;

    battle_sync_battler(battle, attacker_id);
    battle_sync_battler(battle, target_id);
    return 1;
}

static int battle_baton_pass(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t party_slot)
{
    RemasterEmeraldBattleMon *old;
    uint8_t stages[REMASTER_EMERALD_BATTLE_STAT_COUNT];
    uint32_t status2;
    uint32_t status3;
    uint16_t substitute_hp;
    uint8_t perish_count;
    uint8_t lock_on_turns;
    uint8_t lock_on_target;

    if (battle == 0
        || !battle_valid_battler(battler)
        || (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_ARENA))
        return 0;

    old = &battle->battlers[battler];
    memcpy(stages, old->stat_stages, sizeof(stages));
    status2 = old->status2
        & (REMASTER_EMERALD_STATUS2_CONFUSION
           | REMASTER_EMERALD_STATUS2_FOCUS_ENERGY
           | REMASTER_EMERALD_STATUS2_SUBSTITUTE
           | REMASTER_EMERALD_STATUS2_ESCAPE_PREVENTION
           | REMASTER_EMERALD_STATUS2_CURSED);
    status3 = old->status3
        & (UINT32_C(0x00000007)
           | REMASTER_EMERALD_STATUS3_ALWAYS_HITS
           | REMASTER_EMERALD_STATUS3_PERISH_SONG
           | REMASTER_EMERALD_STATUS3_ROOTED
           | REMASTER_EMERALD_STATUS3_MUD_SPORT
           | REMASTER_EMERALD_STATUS3_WATER_SPORT);
    substitute_hp = old->substitute_hp;
    perish_count = old->perish_count;
    lock_on_turns = old->lock_on_turns;
    lock_on_target = old->lock_on_target;

    if (!remaster_emerald_battle_switch(
            battle,
            battler,
            party_slot))
        return 0;

    old = &battle->battlers[battler];
    memcpy(old->stat_stages, stages, sizeof(stages));
    old->status2 = status2;
    old->status3 = status3;
    old->substitute_hp = substitute_hp;
    old->perish_count = perish_count;
    old->lock_on_turns = lock_on_turns;
    old->lock_on_target = lock_on_target;
    battle_sync_battler(battle, battler);
    return 1;
}

int remaster_emerald_battle_switch(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t party_slot)
{
    RemasterEmeraldBattleMon old;
    uint8_t side;
    uint16_t spikes_damage;

    if (battle == 0
        || !battle_valid_battler(battler)
        || !battle->battlers[battler].active)
        return 0;

    side = battle->battlers[battler].side;
    if (party_slot >= battle->party_count[side]
        || battle->parties[side][party_slot].hp == 0
        || remaster_emerald_box_pokemon_species(
            &battle->parties[side][party_slot].box) == 0
        || battle_party_slot_active(battle, side, party_slot))
        return 0;

    old = battle->battlers[battler];
    if (old.ability == ABILITY_NATURAL_CURE)
        old.pokemon.status = 0;
    battle->battlers[battler] = old;
    battle_sync_battler(battle, battler);

    if (!battle_load_mon(
            &battle->battlers[battler],
            &battle->parties[side][party_slot],
            side,
            party_slot))
        return 0;

    battle->active_party_slot[battler] = party_slot;
    battle->battlers[battler].entered_turn =
        (uint8_t)((battle->turn_number + 1u) & 0xFFu);
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_SWITCH,
        battler,
        battler,
        0,
        party_slot,
        0);

    if ((battle->side_status[side] & REMASTER_EMERALD_SIDE_SPIKES)
        && battle->battlers[battler].types[0] != TYPE_FLYING
        && battle->battlers[battler].types[1] != TYPE_FLYING
        && battle->battlers[battler].ability != ABILITY_LEVITATE) {
        spikes_damage =
            battle->battlers[battler].pokemon.max_hp / 8u;
        if (battle->spikes_layers[side] == 2)
            spikes_damage =
                battle->battlers[battler].pokemon.max_hp / 6u;
        else if (battle->spikes_layers[side] >= 3)
            spikes_damage =
                battle->battlers[battler].pokemon.max_hp / 4u;
        if (spikes_damage == 0)
            spikes_damage = 1;
        battle_damage_direct(
            battle,
            battler,
            battler,
            spikes_damage,
            0,
            0);
        battle_faint_check(battle, battler);
        if (battle->battlers[battler].fainted)
            return 1;
    }

    battle_entry_ability(battle, battler);
    battle_sync_battler(battle, battler);
    return 1;
}

int remaster_emerald_battle_needs_replacement(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    uint8_t slot;

    if (battle == 0
        || battle->ended
        || !battle_valid_battler(battler)
        || !battle->battlers[battler].active
        || !battle->battlers[battler].fainted)
        return 0;

    return battle_find_switch_slot(
        battle,
        battle->battlers[battler].side,
        &slot);
}

int remaster_emerald_battle_replace_fainted(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t party_slot)
{
    if (!remaster_emerald_battle_needs_replacement(
            battle, battler))
        return 0;

    return remaster_emerald_battle_switch(
        battle, battler, party_slot);
}

static int battle_move_score(
    const RemasterEmeraldBattleState *battle,
    uint8_t attacker,
    uint8_t target,
    uint8_t slot)
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[attacker];
    const RemasterEmeraldBattleMon *foe = &battle->battlers[target];
    const RemasterEmeraldMoveInfo *move =
        remaster_emerald_move_info(user->moves[slot]);
    uint8_t type_mult;
    int score;

    if (move == 0 || user->pp[slot] == 0)
        return INT_MIN;

    type_mult = battle_type_multiplier(foe, move->type);
    if (move->power != 0) {
        if (type_mult == 0)
            return -10000;
        score = (int)move->power * type_mult;
        if (user->types[0] == move->type
            || user->types[1] == move->type)
            score = score * 3 / 2;
        if (move->accuracy != 0)
            score = score * move->accuracy / 100;
    } else {
        score = 100;
        if (move->effect == EFFECT_RESTORE_HP
            && user->pokemon.hp * 2u < user->pokemon.max_hp)
            score += 300;
        if ((move->effect == EFFECT_SLEEP
                || move->effect == EFFECT_TOXIC
                || move->effect == EFFECT_PARALYZE)
            && foe->pokemon.status == 0)
            score += 200;
    }

    return score;
}

static int battle_ai_move_usable(
    const RemasterEmeraldBattleMon *mon,
    uint8_t slot)
{
    const RemasterEmeraldMoveInfo *move;
    uint16_t move_id;

    if (mon == 0 || slot >= REMASTER_EMERALD_MAX_MOVES)
        return 0;

    move_id = mon->moves[slot];
    move = remaster_emerald_move_info(move_id);
    if (move_id == 0 || move == 0 || mon->pp[slot] == 0)
        return 0;

    if (mon->disable_turns != 0 && mon->disable_move_slot == slot)
        return 0;
    if (mon->encore_turns != 0 && mon->encore_move_slot != slot)
        return 0;
    if (mon->choice_locked_move != 0
        && mon->choice_locked_move != move_id)
        return 0;
    if ((mon->status2 & REMASTER_EMERALD_STATUS2_TORMENT)
        && mon->last_move == move_id)
        return 0;
    if (mon->taunt_turns != 0 && move->power == 0)
        return 0;

    return 1;
}

static void battle_ai_score_add(int scores[4], uint8_t slot, int delta)
{
    int value;

    if (slot >= 4 || scores[slot] < 0)
        return;

    value = scores[slot] + delta;
    if (value < 0)
        value = 0;
    if (value > 255)
        value = 255;
    scores[slot] = value;
}

static int battle_ai_has_reserve(
    const RemasterEmeraldBattleState *battle,
    uint8_t side)
{
    uint8_t slot;

    if (battle == 0 || side > 1)
        return 0;

    for (slot = 0; slot < battle->party_count[side]; ++slot) {
        if (!battle_party_slot_active(battle, side, slot)
            && battle->parties[side][slot].hp != 0
            && remaster_emerald_box_pokemon_species(
                &battle->parties[side][slot].box) != 0)
            return 1;
    }
    return 0;
}

static int battle_ai_is_first_turn(
    const RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleMon *mon)
{
    return battle != 0
        && mon != 0
        && (uint8_t)(battle->turn_number & 0xFFu) == mon->entered_turn;
}

static int battle_ai_stat_up_effect(uint8_t effect, uint8_t *out_stage)
{
    switch (effect) {
    case EFFECT_ATTACK_UP:
    case EFFECT_ATTACK_UP_2:
        *out_stage = 1;
        return 1;
    case EFFECT_DEFENSE_UP:
    case EFFECT_DEFENSE_UP_2:
        *out_stage = 2;
        return 1;
    case EFFECT_SPEED_UP:
    case EFFECT_SPEED_UP_2:
        *out_stage = 3;
        return 1;
    case EFFECT_SPECIAL_ATTACK_UP:
    case EFFECT_SPECIAL_ATTACK_UP_2:
        *out_stage = 4;
        return 1;
    case EFFECT_SPECIAL_DEFENSE_UP:
    case EFFECT_SPECIAL_DEFENSE_UP_2:
        *out_stage = 5;
        return 1;
    case EFFECT_ACCURACY_UP:
    case EFFECT_ACCURACY_UP_2:
        *out_stage = 6;
        return 1;
    case EFFECT_EVASION_UP:
    case EFFECT_EVASION_UP_2:
    case EFFECT_MINIMIZE:
        *out_stage = 7;
        return 1;
    default:
        return 0;
    }
}

static int battle_ai_stat_down_effect(uint8_t effect, uint8_t *out_stage)
{
    switch (effect) {
    case EFFECT_ATTACK_DOWN:
    case EFFECT_ATTACK_DOWN_2:
        *out_stage = 1;
        return 1;
    case EFFECT_DEFENSE_DOWN:
    case EFFECT_DEFENSE_DOWN_2:
        *out_stage = 2;
        return 1;
    case EFFECT_SPEED_DOWN:
    case EFFECT_SPEED_DOWN_2:
        *out_stage = 3;
        return 1;
    case EFFECT_SPECIAL_ATTACK_DOWN:
    case EFFECT_SPECIAL_ATTACK_DOWN_2:
        *out_stage = 4;
        return 1;
    case EFFECT_SPECIAL_DEFENSE_DOWN:
    case EFFECT_SPECIAL_DEFENSE_DOWN_2:
        *out_stage = 5;
        return 1;
    case EFFECT_ACCURACY_DOWN:
    case EFFECT_ACCURACY_DOWN_2:
        *out_stage = 6;
        return 1;
    case EFFECT_EVASION_DOWN:
    case EFFECT_EVASION_DOWN_2:
        *out_stage = 7;
        return 1;
    default:
        return 0;
    }
}

static void battle_ai_apply_check_bad_move(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const RemasterEmeraldBattleMon *foe = &battle->battlers[target];
    uint8_t slot;

    if (foe->side == user->side)
        return;

    for (slot = 0; slot < 4; ++slot) {
        const RemasterEmeraldMoveInfo *move;
        uint8_t stage = 0;
        uint8_t type_mult;

        if (scores[slot] < 0)
            continue;

        move = remaster_emerald_move_info(user->moves[slot]);
        if (move == 0)
            continue;

        type_mult = battle_type_multiplier(foe, move->type);
        if (move->power != 0) {
            if (foe->ability == ABILITY_VOLT_ABSORB
                && move->type == TYPE_ELECTRIC)
                battle_ai_score_add(scores, slot, -12);
            else if (foe->ability == ABILITY_WATER_ABSORB
                && move->type == TYPE_WATER)
                battle_ai_score_add(scores, slot, -12);
            else if (foe->ability == ABILITY_FLASH_FIRE
                && move->type == TYPE_FIRE)
                battle_ai_score_add(scores, slot, -12);
            else if (foe->ability == ABILITY_WONDER_GUARD
                && type_mult <= 10)
                battle_ai_score_add(scores, slot, -10);
            else if (type_mult == 0)
                battle_ai_score_add(scores, slot, -10);
        }

        if (battle_ai_stat_up_effect(move->effect, &stage)) {
            if (user->stat_stages[stage] >= 12)
                battle_ai_score_add(scores, slot, -10);
            continue;
        }

        if (battle_ai_stat_down_effect(move->effect, &stage)) {
            if (foe->stat_stages[stage] == 0
                || foe->ability == ABILITY_CLEAR_BODY
                || foe->ability == ABILITY_WHITE_SMOKE
                || (stage == 1 && foe->ability == ABILITY_HYPER_CUTTER))
                battle_ai_score_add(scores, slot, -10);
            continue;
        }

        switch (move->effect) {
        case EFFECT_SLEEP:
            if (!battle_status_allowed(
                    battle, foe, REMASTER_EMERALD_STATUS1_SLEEP)
                || (battle->side_status[foe->side]
                    & REMASTER_EMERALD_SIDE_SAFEGUARD))
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_TOXIC:
        case EFFECT_POISON:
            if (!battle_status_allowed(
                    battle, foe, REMASTER_EMERALD_STATUS1_TOXIC)
                || (battle->side_status[foe->side]
                    & REMASTER_EMERALD_SIDE_SAFEGUARD))
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_PARALYZE:
            if (type_mult == 0
                || !battle_status_allowed(
                    battle, foe, REMASTER_EMERALD_STATUS1_PARALYSIS)
                || (battle->side_status[foe->side]
                    & REMASTER_EMERALD_SIDE_SAFEGUARD))
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_LIGHT_SCREEN:
            if (battle->side_status[user->side]
                & REMASTER_EMERALD_SIDE_LIGHT_SCREEN)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_REFLECT:
            if (battle->side_status[user->side]
                & REMASTER_EMERALD_SIDE_REFLECT)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_MIST:
            if (battle->side_status[user->side] & REMASTER_EMERALD_SIDE_MIST)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_SAFEGUARD:
            if (battle->side_status[user->side]
                & REMASTER_EMERALD_SIDE_SAFEGUARD)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_FOCUS_ENERGY:
            if (user->status2 & REMASTER_EMERALD_STATUS2_FOCUS_ENERGY)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_CONFUSE:
        case EFFECT_FLATTER:
        case EFFECT_SWAGGER:
            if ((foe->status2 & REMASTER_EMERALD_STATUS2_CONFUSION)
                || foe->ability == ABILITY_OWN_TEMPO
                || (battle->side_status[foe->side]
                    & REMASTER_EMERALD_SIDE_SAFEGUARD))
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_SUBSTITUTE:
            if ((user->status2 & REMASTER_EMERALD_STATUS2_SUBSTITUTE)
                || user->pokemon.hp * 100u
                    < user->pokemon.max_hp * 26u)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_LEECH_SEED:
            if ((foe->status3 & REMASTER_EMERALD_STATUS3_LEECH_SEED)
                || foe->types[0] == TYPE_GRASS
                || foe->types[1] == TYPE_GRASS)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_DISABLE:
            if (foe->disable_turns != 0)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_ENCORE:
            if (foe->encore_turns != 0)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_MEAN_LOOK:
            if (foe->status2 & REMASTER_EMERALD_STATUS2_ESCAPE_PREVENTION)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_SPIKES:
            if (battle->side_status[foe->side] & REMASTER_EMERALD_SIDE_SPIKES)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_FORESIGHT:
            if (foe->status2 & REMASTER_EMERALD_STATUS2_FORESIGHT)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_PERISH_SONG:
            if (foe->status3 & REMASTER_EMERALD_STATUS3_PERISH_SONG)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_BATON_PASS:
            if (!battle_ai_has_reserve(battle, user->side))
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_RAIN_DANCE:
            if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_RAIN)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_SUNNY_DAY:
            if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SUN)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_SANDSTORM:
            if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_HAIL:
            if (battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_HAIL)
                battle_ai_score_add(scores, slot, -8);
            break;
        case EFFECT_FAKE_OUT:
            if (!battle_ai_is_first_turn(battle, user))
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_STOCKPILE:
            if (user->stockpile >= 3)
                battle_ai_score_add(scores, slot, -10);
            break;
        case EFFECT_TELEPORT:
            battle_ai_score_add(scores, slot, -10);
            break;
        default:
            break;
        }
    }
}

static uint16_t battle_ai_estimated_damage(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    uint16_t move_id,
    uint8_t *out_effectiveness)
{
    RemasterEmeraldBattleState probe;
    RemasterEmeraldBattleDamageResult result;

    if (battle == 0)
        return 0;

    probe = *battle;
    if (!battle_calculate_damage_internal(
            &probe,
            battler,
            target,
            move_id,
            &result,
            1,
            0))
        return 0;

    if (out_effectiveness != 0)
        *out_effectiveness = result.effectiveness_tenths;
    return result.damage;
}

static void battle_ai_apply_try_to_faint(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const RemasterEmeraldBattleMon *foe = &battle->battlers[target];
    uint16_t damage[4] = {0};
    uint8_t effectiveness[4] = {0};
    uint16_t best_damage = 0;
    uint8_t slot;

    if (foe->side == user->side)
        return;

    for (slot = 0; slot < 4; ++slot) {
        if (scores[slot] < 0)
            continue;
        damage[slot] = battle_ai_estimated_damage(
            battle,
            battler,
            target,
            user->moves[slot],
            &effectiveness[slot]);
        if (damage[slot] > best_damage)
            best_damage = damage[slot];
    }

    for (slot = 0; slot < 4; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;
        move = remaster_emerald_move_info(user->moves[slot]);
        if (move == 0)
            continue;

        if (damage[slot] != 0 && damage[slot] >= foe->pokemon.hp) {
            if (move->effect != EFFECT_EXPLOSION) {
                battle_ai_score_add(
                    scores,
                    slot,
                    move->effect == EFFECT_QUICK_ATTACK ? 6 : 4);
            }
        } else if (damage[slot] < best_damage) {
            battle_ai_score_add(scores, slot, -1);
        }

        if (effectiveness[slot] >= 40
            && remaster_emerald_battle_random(&battle->rng) % 256u >= 80u)
            battle_ai_score_add(scores, slot, 2);
    }
}

static int battle_ai_target_faster(
    const RemasterEmeraldBattleMon *user,
    const RemasterEmeraldBattleMon *target)
{
    uint32_t user_speed;
    uint32_t target_speed;

    if (user == 0 || target == 0)
        return 0;

    user_speed = battle_apply_stage(user->pokemon.speed, user->stat_stages[3]);
    target_speed = battle_apply_stage(
        target->pokemon.speed, target->stat_stages[3]);
    return target_speed > user_speed;
}

static void battle_ai_apply_check_viability(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const RemasterEmeraldBattleMon *foe = &battle->battlers[target];
    uint8_t slot;

    if (foe->side == user->side)
        return;

    for (slot = 0; slot < 4; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;
        move = remaster_emerald_move_info(user->moves[slot]);
        if (move == 0)
            continue;

        switch (move->effect) {
        case EFFECT_RESTORE_HP:
        case EFFECT_SOFTBOILED:
        case EFFECT_MORNING_SUN:
        case EFFECT_SYNTHESIS:
        case EFFECT_MOONLIGHT:
            if (user->pokemon.hp >= user->pokemon.max_hp)
                battle_ai_score_add(scores, slot, -3);
            else if (user->pokemon.hp * 2u < user->pokemon.max_hp)
                battle_ai_score_add(scores, slot, 2);
            break;
        case EFFECT_REST:
            if (user->pokemon.hp >= user->pokemon.max_hp
                && user->pokemon.status == 0)
                battle_ai_score_add(scores, slot, -8);
            else if (user->pokemon.hp * 2u < user->pokemon.max_hp)
                battle_ai_score_add(scores, slot, 3);
            break;
        case EFFECT_PARALYZE:
            if (battle_ai_target_faster(user, foe)) {
                if (remaster_emerald_battle_random(&battle->rng) % 256u >= 20u)
                    battle_ai_score_add(scores, slot, 3);
            } else if (user->pokemon.hp * 100u
                <= user->pokemon.max_hp * 70u) {
                battle_ai_score_add(scores, slot, -1);
            }
            break;
        case EFFECT_FAKE_OUT:
            battle_ai_score_add(scores, slot, 2);
            break;
        case EFFECT_FACADE:
            if (foe->pokemon.status
                & (REMASTER_EMERALD_STATUS1_POISON
                    | REMASTER_EMERALD_STATUS1_BURN
                    | REMASTER_EMERALD_STATUS1_PARALYSIS
                    | REMASTER_EMERALD_STATUS1_TOXIC))
                battle_ai_score_add(scores, slot, 1);
            break;
        case EFFECT_SMELLINGSALT:
            if (foe->pokemon.status & REMASTER_EMERALD_STATUS1_PARALYSIS)
                battle_ai_score_add(scores, slot, 1);
            break;
        case EFFECT_BRICK_BREAK:
            if (battle->side_status[foe->side]
                & (REMASTER_EMERALD_SIDE_REFLECT
                    | REMASTER_EMERALD_SIDE_LIGHT_SCREEN))
                battle_ai_score_add(scores, slot, 1);
            break;
        case EFFECT_KNOCK_OFF:
            if (foe->held_item != 0)
                battle_ai_score_add(scores, slot, 1);
            break;
        case EFFECT_REFRESH:
            if (user->pokemon.status
                & (REMASTER_EMERALD_STATUS1_POISON
                    | REMASTER_EMERALD_STATUS1_TOXIC
                    | REMASTER_EMERALD_STATUS1_BURN
                    | REMASTER_EMERALD_STATUS1_PARALYSIS))
                battle_ai_score_add(scores, slot, 2);
            else
                battle_ai_score_add(scores, slot, -3);
            break;
        default:
            break;
        }
    }
}

static int battle_ai_setup_effect(uint8_t effect)
{
    switch (effect) {
    case EFFECT_ATTACK_UP:
    case EFFECT_DEFENSE_UP:
    case EFFECT_SPEED_UP:
    case EFFECT_SPECIAL_ATTACK_UP:
    case EFFECT_SPECIAL_DEFENSE_UP:
    case EFFECT_ACCURACY_UP:
    case EFFECT_EVASION_UP:
    case EFFECT_ATTACK_DOWN:
    case EFFECT_DEFENSE_DOWN:
    case EFFECT_SPEED_DOWN:
    case EFFECT_SPECIAL_ATTACK_DOWN:
    case EFFECT_SPECIAL_DEFENSE_DOWN:
    case EFFECT_ACCURACY_DOWN:
    case EFFECT_EVASION_DOWN:
    case EFFECT_CONVERSION:
    case EFFECT_LIGHT_SCREEN:
    case EFFECT_SPECIAL_DEFENSE_UP_2:
    case EFFECT_FOCUS_ENERGY:
    case EFFECT_CONFUSE:
    case EFFECT_ATTACK_UP_2:
    case EFFECT_DEFENSE_UP_2:
    case EFFECT_SPEED_UP_2:
    case EFFECT_SPECIAL_ATTACK_UP_2:
    case EFFECT_ACCURACY_UP_2:
    case EFFECT_EVASION_UP_2:
    case EFFECT_ATTACK_DOWN_2:
    case EFFECT_DEFENSE_DOWN_2:
    case EFFECT_SPEED_DOWN_2:
    case EFFECT_SPECIAL_ATTACK_DOWN_2:
    case EFFECT_SPECIAL_DEFENSE_DOWN_2:
    case EFFECT_ACCURACY_DOWN_2:
    case EFFECT_EVASION_DOWN_2:
    case EFFECT_REFLECT:
    case EFFECT_POISON:
    case EFFECT_PARALYZE:
    case EFFECT_SUBSTITUTE:
    case EFFECT_LEECH_SEED:
    case EFFECT_MINIMIZE:
    case EFFECT_CURSE:
    case EFFECT_SWAGGER:
    case EFFECT_YAWN:
    case EFFECT_DEFENSE_CURL:
    case EFFECT_TORMENT:
    case EFFECT_FLATTER:
    case EFFECT_WILL_O_WISP:
    case EFFECT_INGRAIN:
    case EFFECT_IMPRISON:
    case EFFECT_TEETER_DANCE:
    case EFFECT_TICKLE:
    case EFFECT_COSMIC_POWER:
    case EFFECT_BULK_UP:
    case EFFECT_CALM_MIND:
    case EFFECT_CAMOUFLAGE:
        return 1;
    default:
        return 0;
    }
}

static void battle_ai_apply_setup_first_turn(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    uint8_t slot;

    if (battle->battlers[target].side == user->side
        || battle->turn_number > 1u)
        return;

    for (slot = 0; slot < 4; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;
        move = remaster_emerald_move_info(user->moves[slot]);
        if (move != 0
            && battle_ai_setup_effect(move->effect)
            && remaster_emerald_battle_random(&battle->rng) % 256u >= 80u)
            battle_ai_score_add(scores, slot, 2);
    }
}

static int battle_ai_risky_effect(uint8_t effect)
{
    switch (effect) {
    case EFFECT_SLEEP:
    case EFFECT_EXPLOSION:
    case EFFECT_MIRROR_MOVE:
    case EFFECT_OHKO:
    case EFFECT_HIGH_CRITICAL:
    case EFFECT_CONFUSE:
    case EFFECT_METRONOME:
    case EFFECT_PSYWAVE:
    case EFFECT_COUNTER:
    case EFFECT_DESTINY_BOND:
    case EFFECT_SWAGGER:
    case EFFECT_ATTRACT:
    case EFFECT_PRESENT:
    case EFFECT_ALL_STATS_UP_HIT:
    case EFFECT_BELLY_DRUM:
    case EFFECT_MIRROR_COAT:
    case EFFECT_FOCUS_PUNCH:
    case EFFECT_REVENGE:
    case EFFECT_TEETER_DANCE:
        return 1;
    default:
        return 0;
    }
}

static void battle_ai_apply_risky(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    uint8_t slot;

    if (battle->battlers[target].side == user->side)
        return;

    for (slot = 0; slot < 4; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;
        move = remaster_emerald_move_info(user->moves[slot]);
        if (move != 0
            && battle_ai_risky_effect(move->effect)
            && remaster_emerald_battle_random(&battle->rng) % 256u >= 128u)
            battle_ai_score_add(scores, slot, 2);
    }
}

static int battle_ai_power_is_other(
    const RemasterEmeraldMoveInfo *move)
{
    if (move == 0 || move->power <= 1)
        return 1;

    switch (move->effect) {
    case EFFECT_EXPLOSION:
    case EFFECT_DREAM_EATER:
    case EFFECT_RAZOR_WIND:
    case EFFECT_SKY_ATTACK:
    case EFFECT_RECHARGE:
    case EFFECT_SKULL_BASH:
    case EFFECT_SOLAR_BEAM:
    case EFFECT_SPIT_UP:
    case EFFECT_FOCUS_PUNCH:
    case EFFECT_SUPERPOWER:
    case EFFECT_ERUPTION:
    case EFFECT_OVERHEAT:
        return 1;
    default:
        return 0;
    }
}

static void battle_ai_apply_prefer_power_extremes(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    uint8_t slot;

    if (battle->battlers[target].side == user->side)
        return;

    for (slot = 0; slot < REMASTER_EMERALD_MAX_MOVES; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;

        move = remaster_emerald_move_info(user->moves[slot]);
        if (battle_ai_power_is_other(move)
            && remaster_emerald_battle_random(&battle->rng) % 256u >= 100u)
            battle_ai_score_add(scores, slot, 2);
    }
}

static int battle_ai_has_move_effect(
    const RemasterEmeraldBattleMon *user,
    uint8_t effect)
{
    uint8_t slot;

    if (user == 0)
        return 0;

    for (slot = 0; slot < REMASTER_EMERALD_MAX_MOVES; ++slot) {
        const RemasterEmeraldMoveInfo *move =
            remaster_emerald_move_info(user->moves[slot]);
        if (move != 0 && move->effect == effect)
            return 1;
    }
    return 0;
}

static void battle_ai_apply_prefer_baton_pass(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const int has_baton_pass =
        battle_ai_has_move_effect(user, EFFECT_BATON_PASS);
    uint8_t slot;

    if (battle->battlers[target].side == user->side
        || !battle_ai_has_reserve(battle, user->side))
        return;

    for (slot = 0; slot < REMASTER_EMERALD_MAX_MOVES; ++slot) {
        const RemasterEmeraldMoveInfo *move;
        uint16_t move_id;

        if (scores[slot] < 0)
            continue;

        move_id = user->moves[slot];
        move = remaster_emerald_move_info(move_id);
        if (!battle_ai_power_is_other(move))
            continue;

        if (!has_baton_pass
            && remaster_emerald_battle_random(&battle->rng) % 256u < 80u)
            continue;

        if (move_id == MOVE_SWORDS_DANCE
            || move_id == MOVE_DRAGON_DANCE
            || move_id == MOVE_CALM_MIND) {
            if (battle->turn_number <= 1u) {
                battle_ai_score_add(scores, slot, 5);
            } else if ((uint32_t)user->pokemon.hp * 100u
                    < (uint32_t)user->pokemon.max_hp * 60u) {
                battle_ai_score_add(scores, slot, -10);
            } else {
                battle_ai_score_add(scores, slot, 1);
            }
            continue;
        }

        if (move->effect == EFFECT_PROTECT) {
            if (user->last_move == MOVE_PROTECT
                || user->last_move == MOVE_DETECT)
                battle_ai_score_add(scores, slot, -2);
            else
                battle_ai_score_add(scores, slot, 2);
            continue;
        }

        if (move_id == MOVE_BATON_PASS) {
            if (battle->turn_number <= 1u) {
                battle_ai_score_add(scores, slot, -2);
            } else if (user->stat_stages[1] > 8u) {
                battle_ai_score_add(scores, slot, 3);
            } else if (user->stat_stages[1] > 7u) {
                battle_ai_score_add(scores, slot, 2);
            } else if (user->stat_stages[1] > 6u) {
                battle_ai_score_add(scores, slot, 1);
            } else if (user->stat_stages[4] > 8u) {
                battle_ai_score_add(scores, slot, 3);
            } else if (user->stat_stages[4] > 7u) {
                battle_ai_score_add(scores, slot, 2);
            } else if (user->stat_stages[4] > 6u) {
                battle_ai_score_add(scores, slot, 1);
            }
            continue;
        }

        if (remaster_emerald_battle_random(&battle->rng) % 256u < 20u)
            continue;
        battle_ai_score_add(scores, slot, 3);
    }
}

static void battle_ai_apply_double_battle(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4]);

static uint8_t battle_ai_hp_percent(
    const RemasterEmeraldBattleMon *mon)
{
    if (mon == 0 || mon->pokemon.max_hp == 0)
        return 0;
    return (uint8_t)(
        (uint32_t)mon->pokemon.hp * 100u / mon->pokemon.max_hp);
}

static int battle_ai_hp_aware_user_discouraged(
    uint8_t hp_percent,
    uint8_t effect)
{
    if (hp_percent > 70u) {
        switch (effect) {
        case EFFECT_EXPLOSION:
        case EFFECT_RESTORE_HP:
        case EFFECT_REST:
        case EFFECT_DESTINY_BOND:
        case EFFECT_FLAIL:
        case EFFECT_ENDURE:
        case EFFECT_MORNING_SUN:
        case EFFECT_SYNTHESIS:
        case EFFECT_MOONLIGHT:
        case EFFECT_SOFTBOILED:
        case EFFECT_MEMENTO:
        case EFFECT_GRUDGE:
        case EFFECT_OVERHEAT:
            return 1;
        default:
            return 0;
        }
    }

    if (hp_percent > 30u) {
        switch (effect) {
        case EFFECT_EXPLOSION:
        case EFFECT_ATTACK_UP:
        case EFFECT_DEFENSE_UP:
        case EFFECT_SPEED_UP:
        case EFFECT_SPECIAL_ATTACK_UP:
        case EFFECT_SPECIAL_DEFENSE_UP:
        case EFFECT_ACCURACY_UP:
        case EFFECT_EVASION_UP:
        case EFFECT_ATTACK_DOWN:
        case EFFECT_DEFENSE_DOWN:
        case EFFECT_SPEED_DOWN:
        case EFFECT_SPECIAL_ATTACK_DOWN:
        case EFFECT_SPECIAL_DEFENSE_DOWN:
        case EFFECT_ACCURACY_DOWN:
        case EFFECT_EVASION_DOWN:
        case EFFECT_BIDE:
        case EFFECT_CONVERSION:
        case EFFECT_LIGHT_SCREEN:
        case EFFECT_MIST:
        case EFFECT_FOCUS_ENERGY:
        case EFFECT_ATTACK_UP_2:
        case EFFECT_DEFENSE_UP_2:
        case EFFECT_SPEED_UP_2:
        case EFFECT_SPECIAL_ATTACK_UP_2:
        case EFFECT_SPECIAL_DEFENSE_UP_2:
        case EFFECT_ACCURACY_UP_2:
        case EFFECT_EVASION_UP_2:
        case EFFECT_ATTACK_DOWN_2:
        case EFFECT_DEFENSE_DOWN_2:
        case EFFECT_SPEED_DOWN_2:
        case EFFECT_SPECIAL_ATTACK_DOWN_2:
        case EFFECT_SPECIAL_DEFENSE_DOWN_2:
        case EFFECT_ACCURACY_DOWN_2:
        case EFFECT_EVASION_DOWN_2:
        case EFFECT_CONVERSION_2:
        case EFFECT_SAFEGUARD:
        case EFFECT_BELLY_DRUM:
        case EFFECT_TICKLE:
        case EFFECT_COSMIC_POWER:
        case EFFECT_BULK_UP:
        case EFFECT_CALM_MIND:
        case EFFECT_DRAGON_DANCE:
            return 1;
        default:
            return 0;
        }
    }

    switch (effect) {
    case EFFECT_ATTACK_UP:
    case EFFECT_DEFENSE_UP:
    case EFFECT_SPEED_UP:
    case EFFECT_SPECIAL_ATTACK_UP:
    case EFFECT_SPECIAL_DEFENSE_UP:
    case EFFECT_ACCURACY_UP:
    case EFFECT_EVASION_UP:
    case EFFECT_ATTACK_DOWN:
    case EFFECT_DEFENSE_DOWN:
    case EFFECT_SPEED_DOWN:
    case EFFECT_SPECIAL_ATTACK_DOWN:
    case EFFECT_SPECIAL_DEFENSE_DOWN:
    case EFFECT_ACCURACY_DOWN:
    case EFFECT_EVASION_DOWN:
    case EFFECT_BIDE:
    case EFFECT_CONVERSION:
    case EFFECT_LIGHT_SCREEN:
    case EFFECT_MIST:
    case EFFECT_FOCUS_ENERGY:
    case EFFECT_ATTACK_UP_2:
    case EFFECT_DEFENSE_UP_2:
    case EFFECT_SPEED_UP_2:
    case EFFECT_SPECIAL_ATTACK_UP_2:
    case EFFECT_SPECIAL_DEFENSE_UP_2:
    case EFFECT_ACCURACY_UP_2:
    case EFFECT_EVASION_UP_2:
    case EFFECT_ATTACK_DOWN_2:
    case EFFECT_DEFENSE_DOWN_2:
    case EFFECT_SPEED_DOWN_2:
    case EFFECT_SPECIAL_ATTACK_DOWN_2:
    case EFFECT_SPECIAL_DEFENSE_DOWN_2:
    case EFFECT_ACCURACY_DOWN_2:
    case EFFECT_EVASION_DOWN_2:
    case EFFECT_RAGE:
    case EFFECT_CONVERSION_2:
    case EFFECT_LOCK_ON:
    case EFFECT_SAFEGUARD:
    case EFFECT_BELLY_DRUM:
    case EFFECT_PSYCH_UP:
    case EFFECT_MIRROR_COAT:
    case EFFECT_SOLAR_BEAM:
    case EFFECT_ERUPTION:
    case EFFECT_TICKLE:
    case EFFECT_COSMIC_POWER:
    case EFFECT_BULK_UP:
    case EFFECT_CALM_MIND:
    case EFFECT_DRAGON_DANCE:
        return 1;
    default:
        return 0;
    }
}

static int battle_ai_hp_aware_target_discouraged(
    uint8_t hp_percent,
    uint8_t effect)
{
    if (hp_percent > 70u)
        return 0;

    if (hp_percent > 30u) {
        switch (effect) {
        case EFFECT_ATTACK_UP:
        case EFFECT_DEFENSE_UP:
        case EFFECT_SPEED_UP:
        case EFFECT_SPECIAL_ATTACK_UP:
        case EFFECT_SPECIAL_DEFENSE_UP:
        case EFFECT_ACCURACY_UP:
        case EFFECT_EVASION_UP:
        case EFFECT_ATTACK_DOWN:
        case EFFECT_DEFENSE_DOWN:
        case EFFECT_SPEED_DOWN:
        case EFFECT_SPECIAL_ATTACK_DOWN:
        case EFFECT_SPECIAL_DEFENSE_DOWN:
        case EFFECT_ACCURACY_DOWN:
        case EFFECT_EVASION_DOWN:
        case EFFECT_MIST:
        case EFFECT_FOCUS_ENERGY:
        case EFFECT_ATTACK_UP_2:
        case EFFECT_DEFENSE_UP_2:
        case EFFECT_SPEED_UP_2:
        case EFFECT_SPECIAL_ATTACK_UP_2:
        case EFFECT_SPECIAL_DEFENSE_UP_2:
        case EFFECT_ACCURACY_UP_2:
        case EFFECT_EVASION_UP_2:
        case EFFECT_ATTACK_DOWN_2:
        case EFFECT_DEFENSE_DOWN_2:
        case EFFECT_SPEED_DOWN_2:
        case EFFECT_SPECIAL_ATTACK_DOWN_2:
        case EFFECT_SPECIAL_DEFENSE_DOWN_2:
        case EFFECT_ACCURACY_DOWN_2:
        case EFFECT_EVASION_DOWN_2:
        case EFFECT_POISON:
        case EFFECT_PAIN_SPLIT:
        case EFFECT_PERISH_SONG:
        case EFFECT_SAFEGUARD:
        case EFFECT_TICKLE:
        case EFFECT_COSMIC_POWER:
        case EFFECT_BULK_UP:
        case EFFECT_CALM_MIND:
        case EFFECT_DRAGON_DANCE:
            return 1;
        default:
            return 0;
        }
    }

    switch (effect) {
    case EFFECT_SLEEP:
    case EFFECT_EXPLOSION:
    case EFFECT_ATTACK_UP:
    case EFFECT_DEFENSE_UP:
    case EFFECT_SPEED_UP:
    case EFFECT_SPECIAL_ATTACK_UP:
    case EFFECT_SPECIAL_DEFENSE_UP:
    case EFFECT_ACCURACY_UP:
    case EFFECT_EVASION_UP:
    case EFFECT_ATTACK_DOWN:
    case EFFECT_DEFENSE_DOWN:
    case EFFECT_SPEED_DOWN:
    case EFFECT_SPECIAL_ATTACK_DOWN:
    case EFFECT_SPECIAL_DEFENSE_DOWN:
    case EFFECT_ACCURACY_DOWN:
    case EFFECT_EVASION_DOWN:
    case EFFECT_BIDE:
    case EFFECT_CONVERSION:
    case EFFECT_TOXIC:
    case EFFECT_LIGHT_SCREEN:
    case EFFECT_OHKO:
    case EFFECT_SUPER_FANG:
    case EFFECT_MIST:
    case EFFECT_FOCUS_ENERGY:
    case EFFECT_CONFUSE:
    case EFFECT_ATTACK_UP_2:
    case EFFECT_DEFENSE_UP_2:
    case EFFECT_SPEED_UP_2:
    case EFFECT_SPECIAL_ATTACK_UP_2:
    case EFFECT_SPECIAL_DEFENSE_UP_2:
    case EFFECT_ACCURACY_UP_2:
    case EFFECT_EVASION_UP_2:
    case EFFECT_ATTACK_DOWN_2:
    case EFFECT_DEFENSE_DOWN_2:
    case EFFECT_SPEED_DOWN_2:
    case EFFECT_SPECIAL_ATTACK_DOWN_2:
    case EFFECT_SPECIAL_DEFENSE_DOWN_2:
    case EFFECT_ACCURACY_DOWN_2:
    case EFFECT_EVASION_DOWN_2:
    case EFFECT_POISON:
    case EFFECT_PARALYZE:
    case EFFECT_PAIN_SPLIT:
    case EFFECT_CONVERSION_2:
    case EFFECT_LOCK_ON:
    case EFFECT_SPITE:
    case EFFECT_PERISH_SONG:
    case EFFECT_SWAGGER:
    case EFFECT_FURY_CUTTER:
    case EFFECT_ATTRACT:
    case EFFECT_SAFEGUARD:
    case EFFECT_PSYCH_UP:
    case EFFECT_MIRROR_COAT:
    case EFFECT_WILL_O_WISP:
    case EFFECT_TICKLE:
    case EFFECT_COSMIC_POWER:
    case EFFECT_BULK_UP:
    case EFFECT_CALM_MIND:
    case EFFECT_DRAGON_DANCE:
        return 1;
    default:
        return 0;
    }
}

static void battle_ai_apply_hp_aware(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const RemasterEmeraldBattleMon *foe = &battle->battlers[target];
    const uint8_t user_hp = battle_ai_hp_percent(user);
    const uint8_t target_hp = battle_ai_hp_percent(foe);
    uint8_t slot;

    if (foe->side == user->side) {
        battle_ai_apply_double_battle(
            battle, battler, target, scores);
        return;
    }

    for (slot = 0; slot < REMASTER_EMERALD_MAX_MOVES; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;
        move = remaster_emerald_move_info(user->moves[slot]);
        if (move == 0)
            continue;

        if (battle_ai_hp_aware_user_discouraged(
                user_hp, move->effect)
            && remaster_emerald_battle_random(&battle->rng) % 256u >= 50u)
            battle_ai_score_add(scores, slot, -2);

        if (battle_ai_hp_aware_target_discouraged(
                target_hp, move->effect)
            && remaster_emerald_battle_random(&battle->rng) % 256u >= 50u)
            battle_ai_score_add(scores, slot, -2);
    }
}

static void battle_ai_apply_try_sunny_day_start(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    uint8_t slot;

    if (battle->battlers[target].side == user->side) {
        battle_ai_apply_double_battle(
            battle,
            battler,
            target,
            scores);
        return;
    }
    if (battle->turn_number > 1u)
        return;

    for (slot = 0; slot < REMASTER_EMERALD_MAX_MOVES; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;
        move = remaster_emerald_move_info(user->moves[slot]);
        if (move != 0 && move->effect == EFFECT_SUNNY_DAY)
            battle_ai_score_add(scores, slot, 5);
    }
}

static void battle_ai_apply_double_battle(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int scores[4])
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const RemasterEmeraldBattleMon *chosen = &battle->battlers[target];
    const uint8_t partner_id = (uint8_t)(battler ^ 2u);
    const RemasterEmeraldBattleMon *partner =
        battle_valid_battler(partner_id)
            && battle->battlers[partner_id].active
            && !battle->battlers[partner_id].fainted
        ? &battle->battlers[partner_id]
        : 0;
    uint8_t slot;

    for (slot = 0; slot < 4; ++slot) {
        const RemasterEmeraldMoveInfo *move;

        if (scores[slot] < 0)
            continue;
        move = remaster_emerald_move_info(user->moves[slot]);
        if (move == 0)
            continue;

        if (chosen->side == user->side) {
            if (move->power != 0) {
                if (move->type == TYPE_FIRE
                    && chosen->ability == ABILITY_FLASH_FIRE
                    && !chosen->flash_fire)
                    battle_ai_score_add(scores, slot, 3);
                else
                    battle_ai_score_add(scores, slot, -30);
                continue;
            }

            switch (move->effect) {
            case EFFECT_WILL_O_WISP:
            case EFFECT_TOXIC:
                if (chosen->ability == ABILITY_GUTS
                    && chosen->pokemon.status == 0
                    && user->pokemon.hp * 100u
                        >= user->pokemon.max_hp * 91u)
                    battle_ai_score_add(scores, slot, 5);
                else
                    battle_ai_score_add(scores, slot, -30);
                break;
            case EFFECT_HELPING_HAND:
                if (remaster_emerald_battle_random(&battle->rng)
                        % 256u < 64u)
                    battle_ai_score_add(scores, slot, -1);
                else
                    battle_ai_score_add(scores, slot, 2);
                break;
            case EFFECT_SWAGGER:
                if (chosen->held_item == 140u
                    && chosen->stat_stages[1] <= 7u)
                    battle_ai_score_add(scores, slot, 3);
                else if (chosen->held_item != 140u)
                    battle_ai_score_add(scores, slot, -30);
                break;
            case EFFECT_SKILL_SWAP:
                if (chosen->ability == ABILITY_TRUANT)
                    battle_ai_score_add(scores, slot, 10);
                else
                    battle_ai_score_add(scores, slot, -30);
                break;
            default:
                battle_ai_score_add(scores, slot, -30);
                break;
            }
            continue;
        }

        if ((move->effect == EFFECT_EARTHQUAKE
                || move->effect == EFFECT_MAGNITUDE)
            && partner != 0) {
            if (partner->ability == ABILITY_LEVITATE
                || partner->types[0] == TYPE_FLYING
                || partner->types[1] == TYPE_FLYING) {
                battle_ai_score_add(scores, slot, 2);
            } else if (partner->types[0] == TYPE_FIRE
                || partner->types[1] == TYPE_FIRE
                || partner->types[0] == TYPE_ELECTRIC
                || partner->types[1] == TYPE_ELECTRIC
                || partner->types[0] == TYPE_POISON
                || partner->types[1] == TYPE_POISON
                || partner->types[0] == TYPE_ROCK
                || partner->types[1] == TYPE_ROCK) {
                battle_ai_score_add(scores, slot, -10);
            } else {
                battle_ai_score_add(scores, slot, -3);
            }
        }

        if (move->type == TYPE_FIRE && user->flash_fire)
            battle_ai_score_add(scores, slot, 1);

        if (user->ability == ABILITY_GUTS
            && user->pokemon.status != 0
            && partner != 0) {
            uint8_t partner_slot;
            for (partner_slot = 0; partner_slot < 4; ++partner_slot) {
                const RemasterEmeraldMoveInfo *partner_move =
                    remaster_emerald_move_info(
                        partner->moves[partner_slot]);
                if (partner_move != 0
                    && partner_move->effect == EFFECT_HELPING_HAND
                    && move->power != 0) {
                    battle_ai_score_add(scores, slot, 1);
                    break;
                }
            }
        }
    }
}

static int battle_ai_active_has_super_effective(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target)
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const RemasterEmeraldBattleMon *foe = &battle->battlers[target];
    uint8_t slot;

    for (slot = 0; slot < 4; ++slot) {
        const RemasterEmeraldMoveInfo *move =
            remaster_emerald_move_info(user->moves[slot]);
        uint8_t effectiveness;

        if (move == 0 || user->pp[slot] == 0 || move->power == 0)
            continue;

        effectiveness = battle_type_multiplier(foe, move->type);
        if ((foe->ability == ABILITY_VOLT_ABSORB
                && move->type == TYPE_ELECTRIC)
            || (foe->ability == ABILITY_WATER_ABSORB
                && move->type == TYPE_WATER)
            || (foe->ability == ABILITY_FLASH_FIRE
                && move->type == TYPE_FIRE))
            effectiveness = 0;
        if (foe->ability == ABILITY_WONDER_GUARD
            && effectiveness <= 10)
            effectiveness = 0;

        if (effectiveness >= 20)
            return 1;
    }
    return 0;
}

static int battle_ai_reserve_has_super_effective(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t party_slot,
    uint8_t target)
{
    RemasterEmeraldBattleState probe;

    if (battle == 0
        || party_slot >= battle->party_count[1]
        || battle_party_slot_active(battle, 1, party_slot))
        return 0;

    probe = *battle;
    if (!battle_load_mon(
            &probe.battlers[battler],
            &probe.parties[1][party_slot],
            1,
            party_slot))
        return 0;

    return battle_ai_active_has_super_effective(
        &probe, battler, target);
}

static uint16_t battle_ai_reserve_best_damage(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t party_slot,
    uint8_t target)
{
    RemasterEmeraldBattleState probe;
    uint16_t best = 0;
    uint8_t slot;

    if (battle == 0
        || party_slot >= battle->party_count[1]
        || battle_party_slot_active(battle, 1, party_slot))
        return 0;

    probe = *battle;
    if (!battle_load_mon(
            &probe.battlers[battler],
            &probe.parties[1][party_slot],
            1,
            party_slot))
        return 0;

    for (slot = 0; slot < 4; ++slot) {
        uint16_t damage;
        if (!battle_ai_move_usable(&probe.battlers[battler], slot))
            continue;
        damage = battle_ai_estimated_damage(
            &probe,
            battler,
            target,
            probe.battlers[battler].moves[slot],
            0);
        if (damage > best)
            best = damage;
    }
    return best;
}

static int battle_ai_opponent_has_ability(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t ability)
{
    uint8_t i;
    const uint8_t side = battle->battlers[battler].side;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (battle->battlers[i].active
            && !battle->battlers[i].fainted
            && battle->battlers[i].side != side
            && battle->battlers[i].ability == ability)
            return 1;
    }
    return 0;
}

static int battle_ai_switch_is_trapped(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];

    if (user->status2
        & (REMASTER_EMERALD_STATUS2_WRAPPED
           | REMASTER_EMERALD_STATUS2_ESCAPE_PREVENTION))
        return 1;
    if (user->status3 & REMASTER_EMERALD_STATUS3_ROOTED)
        return 1;
    if (battle_ai_opponent_has_ability(
            battle, battler, ABILITY_SHADOW_TAG))
        return 1;
    /* Vanilla+ intentionally inherits Emerald's Arena Trap AI bug:
       the switch heuristic does not exempt Flying types or Levitate. */
    if (battle_ai_opponent_has_ability(
            battle, battler, ABILITY_ARENA_TRAP))
        return 1;
    if ((user->types[0] == TYPE_STEEL || user->types[1] == TYPE_STEEL)
        && battle_ability_on_field(battle, ABILITY_MAGNET_PULL))
        return 1;
    return 0;
}

static int battle_ai_has_super_effective_against_opponents(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    int no_rng)
{
    uint8_t target;
    const uint8_t side = battle->battlers[battler].side;

    for (target = 0;
         target < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
         ++target) {
        if (!battle->battlers[target].active
            || battle->battlers[target].fainted
            || battle->battlers[target].side == side)
            continue;
        if (!battle_ai_active_has_super_effective(
                battle, battler, target))
            continue;
        if (no_rng
            || remaster_emerald_battle_random(&battle->rng) % 10u != 0u)
            return 1;
    }
    return 0;
}

static int battle_ai_find_counter_switch(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    int require_immunity,
    uint8_t modulo,
    uint8_t *out_slot)
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    const RemasterEmeraldMoveInfo *last;
    uint8_t target;
    uint8_t party_slot;

    if (user->last_taken_move == 0
        || user->last_damage_from == 0xFFu
        || !battle_valid_battler(user->last_damage_from))
        return 0;

    last = remaster_emerald_move_info(user->last_taken_move);
    if (last == 0 || last->power == 0)
        return 0;
    target = user->last_damage_from;

    for (party_slot = 0;
         party_slot < battle->party_count[1];
         ++party_slot) {
        RemasterEmeraldBattleState probe;
        RemasterEmeraldBattleMon *reserve;
        uint8_t incoming;
        uint8_t move_slot;

        if (battle->parties[1][party_slot].hp == 0
            || remaster_emerald_box_pokemon_species(
                &battle->parties[1][party_slot].box) == 0
            || battle_party_slot_active(battle, 1, party_slot))
            continue;

        probe = *battle;
        if (!battle_load_mon(
                &probe.battlers[battler],
                &probe.parties[1][party_slot],
                1,
                party_slot))
            continue;
        reserve = &probe.battlers[battler];

        incoming = battle_type_multiplier(reserve, last->type);
        if ((reserve->ability == ABILITY_VOLT_ABSORB
                && last->type == TYPE_ELECTRIC)
            || (reserve->ability == ABILITY_WATER_ABSORB
                && last->type == TYPE_WATER)
            || (reserve->ability == ABILITY_FLASH_FIRE
                && last->type == TYPE_FIRE)
            || (reserve->ability == ABILITY_LEVITATE
                && last->type == TYPE_GROUND))
            incoming = 0;
        if (reserve->ability == ABILITY_WONDER_GUARD
            && incoming <= 10)
            incoming = 0;

        if (require_immunity ? incoming != 0
                             : (incoming == 0 || incoming >= 10))
            continue;

        for (move_slot = 0; move_slot < 4; ++move_slot) {
            if (!battle_ai_move_usable(reserve, move_slot))
                continue;
            if (!battle_ai_active_has_super_effective(
                    &probe, battler, target))
                break;
            if (modulo == 0
                || remaster_emerald_battle_random(&battle->rng)
                    % modulo == 0u) {
                *out_slot = party_slot;
                return 1;
            }
            break;
        }
    }
    return 0;
}

static int battle_ai_find_best_switch(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int require_super_effective,
    uint8_t *out_slot);

static int battle_ai_find_suitable_switch(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    uint8_t *out_slot)
{
    const RemasterEmeraldBattleMon *foe = &battle->battlers[target];
    uint8_t rejected = 0;

    while (rejected != 0x3Fu) {
        uint8_t party_slot;
        uint8_t best_slot = REMASTER_EMERALD_BATTLE_PARTY_SIZE;
        uint16_t best_type_damage = 0;

        for (party_slot = 0;
             party_slot < battle->party_count[1];
             ++party_slot) {
            RemasterEmeraldBattleState probe;
            RemasterEmeraldBattleMon *reserve;
            uint16_t type_damage;

            if (rejected & (1u << party_slot))
                continue;
            if (battle->parties[1][party_slot].hp == 0
                || remaster_emerald_box_pokemon_species(
                    &battle->parties[1][party_slot].box) == 0
                || battle_party_slot_active(battle, 1, party_slot)) {
                rejected |= (uint8_t)(1u << party_slot);
                continue;
            }

            probe = *battle;
            if (!battle_load_mon(
                    &probe.battlers[battler],
                    &probe.parties[1][party_slot],
                    1,
                    party_slot)) {
                rejected |= (uint8_t)(1u << party_slot);
                continue;
            }
            reserve = &probe.battlers[battler];

            type_damage = battle_type_multiplier(
                reserve, foe->types[0]);
            type_damage = (uint16_t)(
                type_damage
                * battle_type_multiplier(reserve, foe->types[1])
                / 10u);

            /* Match Emerald's documented oddity: it selects the reserve
               taking the greatest type damage before checking for an
               offensive super-effective move. */
            if (best_slot == REMASTER_EMERALD_BATTLE_PARTY_SIZE
                || best_type_damage < type_damage) {
                best_type_damage = type_damage;
                best_slot = party_slot;
            }
        }

        if (best_slot == REMASTER_EMERALD_BATTLE_PARTY_SIZE)
            break;
        if (battle_ai_reserve_has_super_effective(
                battle, battler, best_slot, target)) {
            *out_slot = best_slot;
            return 1;
        }
        rejected |= (uint8_t)(1u << best_slot);
    }

    return battle_ai_find_best_switch(
        battle, battler, target, 0, out_slot);
}

static int battle_ai_find_best_switch(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    int require_super_effective,
    uint8_t *out_slot)
{
    uint8_t party_slot;
    uint8_t best_slot = REMASTER_EMERALD_BATTLE_PARTY_SIZE;
    uint16_t best_damage = 0;

    if (battle == 0 || out_slot == 0)
        return 0;

    for (party_slot = 0;
         party_slot < battle->party_count[1];
         ++party_slot) {
        uint16_t damage;

        if (battle->parties[1][party_slot].hp == 0
            || remaster_emerald_box_pokemon_species(
                &battle->parties[1][party_slot].box) == 0
            || battle_party_slot_active(battle, 1, party_slot))
            continue;

        if (require_super_effective
            && !battle_ai_reserve_has_super_effective(
                battle, battler, party_slot, target))
            continue;

        damage = battle_ai_reserve_best_damage(
            battle, battler, party_slot, target);
        if (best_slot == REMASTER_EMERALD_BATTLE_PARTY_SIZE
            || damage > best_damage) {
            best_slot = party_slot;
            best_damage = damage;
        }
    }

    if (best_slot == REMASTER_EMERALD_BATTLE_PARTY_SIZE)
        return 0;

    *out_slot = best_slot;
    return 1;
}

static int battle_ai_find_absorbing_switch(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t move_type,
    uint8_t *out_slot)
{
    uint8_t desired_ability;
    uint8_t slot;

    if (move_type == TYPE_FIRE)
        desired_ability = ABILITY_FLASH_FIRE;
    else if (move_type == TYPE_WATER)
        desired_ability = ABILITY_WATER_ABSORB;
    else if (move_type == TYPE_ELECTRIC)
        desired_ability = ABILITY_VOLT_ABSORB;
    else
        return 0;

    if (battle->battlers[battler].ability == desired_ability)
        return 0;

    for (slot = 0; slot < battle->party_count[1]; ++slot) {
        const RemasterEmeraldPartyPokemon *member =
            &battle->parties[1][slot];
        uint16_t species;
        uint8_t ability_num;

        if (member->hp == 0
            || battle_party_slot_active(battle, 1, slot))
            continue;
        species = remaster_emerald_box_pokemon_species(&member->box);
        if (species == 0)
            continue;
        ability_num =
            remaster_emerald_box_pokemon_ability_num(&member->box);
        if (remaster_emerald_species_ability(species, ability_num)
            == desired_ability
            && (remaster_emerald_battle_random(&battle->rng) & 1u)) {
            *out_slot = slot;
            return 1;
        }
    }
    return 0;
}

static int battle_ai_stats_are_raised(
    const RemasterEmeraldBattleMon *mon)
{
    unsigned raised = 0;
    uint8_t i;

    if (mon == 0)
        return 0;

    for (i = 0; i < REMASTER_EMERALD_BATTLE_STAT_COUNT; ++i) {
        if (mon->stat_stages[i] > 6)
            raised += mon->stat_stages[i] - 6u;
    }
    return raised > 3u;
}

static int battle_ai_choose_switch_action(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    RemasterEmeraldBattleAction *out_action)
{
    RemasterEmeraldBattleMon *user;
    uint8_t target;
    uint8_t party_slot;
    const RemasterEmeraldMoveInfo *last = 0;

    if (battle == 0
        || out_action == 0
        || !(battle->battle_type_flags
            & REMASTER_EMERALD_BATTLE_TYPE_TRAINER)
        || (battle->battle_type_flags
            & REMASTER_EMERALD_BATTLE_TYPE_ARENA)
        || !battle_valid_battler(battler))
        return 0;

    user = &battle->battlers[battler];
    if (user->side != 1
        || battle_ai_switch_is_trapped(battle, battler)
        || !battle_ai_has_reserve(battle, 1))
        return 0;

    target = battle_default_target(battle, battler);
    if (!battle_valid_battler(target))
        return 0;

    if ((user->status3 & REMASTER_EMERALD_STATUS3_PERISH_SONG)
        && user->perish_count <= 1u
        && battle_ai_find_suitable_switch(
            battle, battler, target, &party_slot))
        goto choose_switch;

    if (!(battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE)
        && battle->battlers[target].ability == ABILITY_WONDER_GUARD
        && !battle_ai_active_has_super_effective(
            battle, battler, target)) {
        uint8_t slot;
        for (slot = 0; slot < battle->party_count[1]; ++slot) {
            if (battle->parties[1][slot].hp == 0
                || battle_party_slot_active(battle, 1, slot)
                || !battle_ai_reserve_has_super_effective(
                    battle, battler, slot, target))
                continue;
            if (remaster_emerald_battle_random(&battle->rng) % 3u < 2u) {
                party_slot = slot;
                goto choose_switch;
            }
        }
    }

    if (!(battle_ai_has_super_effective_against_opponents(
                battle, battler, 1)
            && remaster_emerald_battle_random(&battle->rng) % 3u != 0u)
        && user->last_taken_move != 0) {
        last = remaster_emerald_move_info(user->last_taken_move);
        if (last != 0
            && last->power != 0
            && battle_ai_find_absorbing_switch(
                battle, battler, last->type, &party_slot))
            goto choose_switch;
    }

    if ((user->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP)
        && user->ability == ABILITY_NATURAL_CURE
        && user->pokemon.hp >= user->pokemon.max_hp / 2u) {
        if (user->last_taken_move != 0)
            last = remaster_emerald_move_info(user->last_taken_move);

        if ((user->last_taken_move == 0
                || last == 0
                || last->power == 0)
            && (remaster_emerald_battle_random(&battle->rng) & 1u)
            && battle_ai_find_suitable_switch(
                battle, battler, target, &party_slot))
            goto choose_switch;

        if (battle_ai_find_counter_switch(
                battle, battler, 1, 1, &party_slot)
            || battle_ai_find_counter_switch(
                battle, battler, 0, 1, &party_slot))
            goto choose_switch;

        if ((remaster_emerald_battle_random(&battle->rng) & 1u)
            && battle_ai_find_suitable_switch(
                battle, battler, target, &party_slot))
            goto choose_switch;
    }

    if (battle_ai_has_super_effective_against_opponents(
            battle, battler, 0))
        return 0;

    if (battle_ai_stats_are_raised(user))
        return 0;

    if (battle_ai_find_counter_switch(
            battle, battler, 1, 2, &party_slot)
        || battle_ai_find_counter_switch(
            battle, battler, 0, 3, &party_slot))
        goto choose_switch;

    return 0;

choose_switch:
    memset(out_action, 0, sizeof(*out_action));
    out_action->kind = REMASTER_EMERALD_BATTLE_ACTION_SWITCH;
    out_action->party_slot = party_slot;
    out_action->target = battler;
    return 1;
}

static uint16_t battle_ai_trainer_item_heal(uint16_t item_id)
{
    switch (item_id) {
    case ITEM_POTION:
        return 20;
    case ITEM_SUPER_POTION:
        return 50;
    case ITEM_HYPER_POTION:
        return 200;
    case ITEM_MAX_POTION:
    case ITEM_FULL_RESTORE:
        return UINT16_MAX;
    default:
        return 0;
    }
}

enum {
    BATTLE_AI_ITEM_NOT_RECOGNIZABLE = 0,
    BATTLE_AI_ITEM_FULL_RESTORE,
    BATTLE_AI_ITEM_HEAL_HP,
    BATTLE_AI_ITEM_CURE_CONDITION,
    BATTLE_AI_ITEM_X_STAT,
    BATTLE_AI_ITEM_GUARD_SPEC
};

static uint8_t battle_ai_trainer_item_type(uint16_t item_id)
{
    switch (item_id) {
    case ITEM_FULL_RESTORE:
        return BATTLE_AI_ITEM_FULL_RESTORE;
    case ITEM_POTION:
    case ITEM_MAX_POTION:
    case ITEM_HYPER_POTION:
    case ITEM_SUPER_POTION:
        return BATTLE_AI_ITEM_HEAL_HP;
    case ITEM_ANTIDOTE:
    case ITEM_BURN_HEAL:
    case ITEM_ICE_HEAL:
    case ITEM_AWAKENING:
    case ITEM_PARALYZE_HEAL:
    case ITEM_FULL_HEAL:
        return BATTLE_AI_ITEM_CURE_CONDITION;
    case ITEM_DIRE_HIT:
    case ITEM_X_ATTACK:
    case ITEM_X_DEFEND:
    case ITEM_X_SPEED:
    case ITEM_X_ACCURACY:
    case ITEM_X_SPECIAL:
        return BATTLE_AI_ITEM_X_STAT;
    case ITEM_GUARD_SPEC:
        return BATTLE_AI_ITEM_GUARD_SPEC;
    default:
        return BATTLE_AI_ITEM_NOT_RECOGNIZABLE;
    }
}

static uint8_t battle_ai_alive_party_count(
    const RemasterEmeraldBattleState *battle,
    uint8_t side)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < battle->party_count[side]; ++i) {
        if (battle->parties[side][i].hp != 0
            && remaster_emerald_box_pokemon_species(
                &battle->parties[side][i].box) != 0)
            ++count;
    }
    return count;
}

static uint8_t battle_ai_trainer_item_count(
    const RemasterEmeraldBattleState *battle)
{
    const RemasterEmeraldTrainer *trainer =
        remaster_emerald_battle_trainer_find(
            battle->opponent_trainer_id);
    uint8_t i;
    uint8_t count = 0;

    if (trainer != 0) {
        for (i = 0; i < 4; ++i) {
            if (trainer->items[i] != 0)
                ++count;
        }
        if (count != 0)
            return count;
    }

    for (i = 0; i < 4; ++i) {
        if (battle->opponent_trainer_items[i] != 0)
            ++count;
    }
    return count;
}

static int battle_ai_item_cures_current_condition(
    const RemasterEmeraldBattleMon *user,
    uint16_t item_id)
{
    switch (item_id) {
    case ITEM_ANTIDOTE:
        return (user->pokemon.status
            & (REMASTER_EMERALD_STATUS1_POISON
               | REMASTER_EMERALD_STATUS1_TOXIC)) != 0;
    case ITEM_BURN_HEAL:
        return (user->pokemon.status
            & REMASTER_EMERALD_STATUS1_BURN) != 0;
    case ITEM_ICE_HEAL:
        return (user->pokemon.status
            & REMASTER_EMERALD_STATUS1_FREEZE) != 0;
    case ITEM_AWAKENING:
        return (user->pokemon.status
            & REMASTER_EMERALD_STATUS1_SLEEP) != 0;
    case ITEM_PARALYZE_HEAL:
        return (user->pokemon.status
            & REMASTER_EMERALD_STATUS1_PARALYSIS) != 0;
    case ITEM_FULL_HEAL:
        return user->pokemon.status != 0
            || (user->status2 & REMASTER_EMERALD_STATUS2_CONFUSION) != 0;
    default:
        return 0;
    }
}

static int battle_ai_choose_item_action(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    RemasterEmeraldBattleAction *out_action)
{
    RemasterEmeraldBattleMon *user;
    const uint8_t valid_mons =
        battle != 0 ? battle_ai_alive_party_count(battle, 1) : 0;
    const uint8_t items_no =
        battle != 0 ? battle_ai_trainer_item_count(battle) : 0;
    uint8_t i;

    if (battle == 0
        || out_action == 0
        || !battle_valid_battler(battler)
        || !(battle->battle_type_flags
            & REMASTER_EMERALD_BATTLE_TYPE_TRAINER)
        || battle_frontier_type(battle->battle_type_flags)
        || (battle->battle_type_flags
            & (REMASTER_EMERALD_BATTLE_TYPE_LINK
               | REMASTER_EMERALD_BATTLE_TYPE_EREADER_TRAINER
               | REMASTER_EMERALD_BATTLE_TYPE_SECRET_BASE
               | REMASTER_EMERALD_BATTLE_TYPE_INGAME_PARTNER
               | REMASTER_EMERALD_BATTLE_TYPE_RECORDED_LINK)))
        return 0;

    user = &battle->battlers[battler];
    if (user->side != 1 || user->pokemon.hp == 0)
        return 0;

    for (i = 0; i < 4; ++i) {
        const uint16_t item_id = battle->opponent_trainer_items[i];
        const uint8_t item_type =
            battle_ai_trainer_item_type(item_id);
        const uint16_t heal = battle_ai_trainer_item_heal(item_id);
        const uint16_t missing =
            (uint16_t)(user->pokemon.max_hp - user->pokemon.hp);
        int should_use = 0;

        if (i != 0
            && (int)valid_mons > (int)items_no - (int)i + 1)
            continue;
        if (item_id == 0)
            continue;
        if (item_type == BATTLE_AI_ITEM_NOT_RECOGNIZABLE)
            return 0;

        switch (item_type) {
        case BATTLE_AI_ITEM_FULL_RESTORE:
            should_use =
                user->pokemon.hp < user->pokemon.max_hp / 4u;
            break;
        case BATTLE_AI_ITEM_HEAL_HP:
            should_use =
                user->pokemon.hp < user->pokemon.max_hp / 4u
                || (heal != UINT16_MAX && missing > heal);
            break;
        case BATTLE_AI_ITEM_CURE_CONDITION:
            should_use = battle_ai_item_cures_current_condition(
                user, item_id);
            break;
        case BATTLE_AI_ITEM_X_STAT:
            should_use = battle->turn_number <= 1u;
            break;
        case BATTLE_AI_ITEM_GUARD_SPEC:
            should_use =
                battle->turn_number <= 1u
                && !(battle->side_status[user->side]
                    & REMASTER_EMERALD_SIDE_MIST);
            break;
        default:
            break;
        }

        if (!should_use)
            continue;

        memset(out_action, 0, sizeof(*out_action));
        out_action->kind = REMASTER_EMERALD_BATTLE_ACTION_ITEM;
        out_action->item_id = item_id;
        out_action->target = battler;
        battle->opponent_trainer_items[i] = 0;
        return 1;
    }

    return 0;
}

static int battle_use_trainer_item(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint16_t item_id)
{
    RemasterEmeraldBattleMon *user;
    uint16_t heal;

    if (battle == 0
        || !battle_valid_battler(battler)
        || battle->battlers[battler].side != 1)
        return 0;

    user = &battle->battlers[battler];
    if (user->fainted || !user->active)
        return 0;

    heal = battle_ai_trainer_item_heal(item_id);
    if (battle_ai_trainer_item_type(item_id)
        == BATTLE_AI_ITEM_NOT_RECOGNIZABLE)
        return 0;

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_ITEM,
        battler,
        battler,
        0,
        item_id,
        0);

    switch (item_id) {
    case ITEM_FULL_RESTORE:
        user->pokemon.status = 0;
        user->status2 &= ~REMASTER_EMERALD_STATUS2_CONFUSION;
        user->toxic_counter = 0;
        heal = user->pokemon.max_hp;
        break;
    case ITEM_MAX_POTION:
        heal = user->pokemon.max_hp;
        break;
    case ITEM_ANTIDOTE:
        user->pokemon.status &=
            ~(REMASTER_EMERALD_STATUS1_POISON
              | REMASTER_EMERALD_STATUS1_TOXIC
              | REMASTER_EMERALD_STATUS1_TOXIC_COUNTER);
        user->toxic_counter = 0;
        break;
    case ITEM_BURN_HEAL:
        user->pokemon.status &= ~REMASTER_EMERALD_STATUS1_BURN;
        break;
    case ITEM_ICE_HEAL:
        user->pokemon.status &= ~REMASTER_EMERALD_STATUS1_FREEZE;
        break;
    case ITEM_AWAKENING:
        user->pokemon.status &= ~REMASTER_EMERALD_STATUS1_SLEEP;
        break;
    case ITEM_PARALYZE_HEAL:
        user->pokemon.status &= ~REMASTER_EMERALD_STATUS1_PARALYSIS;
        break;
    case ITEM_FULL_HEAL:
        user->pokemon.status = 0;
        user->status2 &= ~REMASTER_EMERALD_STATUS2_CONFUSION;
        user->toxic_counter = 0;
        break;
    case ITEM_X_ATTACK:
        battle_change_stage(battle, battler, 1, 1, 0);
        break;
    case ITEM_X_DEFEND:
        battle_change_stage(battle, battler, 2, 1, 0);
        break;
    case ITEM_X_SPEED:
        battle_change_stage(battle, battler, 3, 1, 0);
        break;
    case ITEM_X_SPECIAL:
        battle_change_stage(battle, battler, 4, 1, 0);
        break;
    case ITEM_X_ACCURACY:
        battle_change_stage(battle, battler, 6, 1, 0);
        break;
    case ITEM_DIRE_HIT:
        user->status2 |= REMASTER_EMERALD_STATUS2_FOCUS_ENERGY;
        break;
    case ITEM_GUARD_SPEC:
        battle->side_status[user->side] |= REMASTER_EMERALD_SIDE_MIST;
        break;
    default:
        break;
    }

    if (heal != 0)
        battle_heal(battle, battler, heal, 0);
    battle_sync_battler(battle, battler);
    return 1;
}

static int battle_ai_choose_trainer_move(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target,
    uint8_t *out_slot,
    int *out_score)
{
    const RemasterEmeraldBattleMon *user = &battle->battlers[battler];
    uint32_t flags = battle->opponent_trainer_ai_flags;
    int scores[4];
    uint8_t best_slots[4];
    uint8_t best_count = 0;
    int best_score = -1;
    uint8_t slot;

    for (slot = 0; slot < 4; ++slot) {
        scores[slot] = battle_ai_move_usable(user, slot) ? 100 : -1;
    }

    if (flags & REMASTER_EMERALD_AI_CHECK_BAD_MOVE)
        battle_ai_apply_check_bad_move(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_TRY_TO_FAINT)
        battle_ai_apply_try_to_faint(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_CHECK_VIABILITY)
        battle_ai_apply_check_viability(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_SETUP_FIRST_TURN)
        battle_ai_apply_setup_first_turn(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_RISKY)
        battle_ai_apply_risky(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_PREFER_POWER_EXTREMES)
        battle_ai_apply_prefer_power_extremes(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_PREFER_BATON_PASS)
        battle_ai_apply_prefer_baton_pass(
            battle, battler, target, scores);
    if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE)
        battle_ai_apply_double_battle(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_HP_AWARE)
        battle_ai_apply_hp_aware(
            battle, battler, target, scores);
    if (flags & REMASTER_EMERALD_AI_TRY_SUNNY_DAY_START)
        battle_ai_apply_try_sunny_day_start(
            battle, battler, target, scores);

    for (slot = 0; slot < 4; ++slot) {
        if (scores[slot] < 0)
            continue;
        if (scores[slot] > best_score) {
            best_score = scores[slot];
            best_slots[0] = slot;
            best_count = 1;
        } else if (scores[slot] == best_score) {
            best_slots[best_count++] = slot;
        }
    }

    if (best_count == 0)
        return 0;

    *out_slot = best_slots[
        remaster_emerald_battle_random(&battle->rng) % best_count];
    if (out_score != 0)
        *out_score = best_score;
    return 1;
}

int remaster_emerald_battle_choose_ai_action(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    RemasterEmeraldBattleAction *out_action)
{
    RemasterEmeraldBattleMon *mon;
    uint8_t target;
    uint8_t slot;
    uint8_t best_slot = 0;
    int best_score = INT_MIN;

    if (battle == 0
        || out_action == 0
        || !battle_valid_battler(battler))
        return 0;

    mon = &battle->battlers[battler];
    if (!mon->active || mon->fainted)
        return 0;

    target = battle_default_target(battle, battler);

    if ((battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_TRAINER)
        && mon->side == 1) {
        if (battle_ai_choose_switch_action(
                battle, battler, out_action))
            return 1;
        if (battle_ai_choose_item_action(
                battle, battler, out_action))
            return 1;

        if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE) {
            uint8_t candidate_targets[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
            uint8_t candidate_slots[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
            uint8_t candidate_count = 0;
            int target_best_score = -1;
            uint8_t candidate;

            for (candidate = 0;
                 candidate < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
                 ++candidate) {
                uint8_t move_slot;
                int move_score;

                if (candidate == battler
                    || !battle->battlers[candidate].active
                    || battle->battlers[candidate].fainted)
                    continue;

                if (!battle_ai_choose_trainer_move(
                        battle,
                        battler,
                        candidate,
                        &move_slot,
                        &move_score))
                    continue;

                if (battle->battlers[candidate].side == mon->side
                    && move_score < 100)
                    continue;

                if (move_score > target_best_score) {
                    target_best_score = move_score;
                    candidate_targets[0] = candidate;
                    candidate_slots[0] = move_slot;
                    candidate_count = 1;
                } else if (move_score == target_best_score) {
                    candidate_targets[candidate_count] = candidate;
                    candidate_slots[candidate_count] = move_slot;
                    candidate_count++;
                }
            }

            if (candidate_count == 0)
                return 0;

            slot = (uint8_t)(
                remaster_emerald_battle_random(&battle->rng)
                % candidate_count);
            target = candidate_targets[slot];
            best_slot = candidate_slots[slot];
        } else if (!battle_ai_choose_trainer_move(
                battle, battler, target, &best_slot, 0)) {
            return 0;
        }
    } else {
        for (slot = 0; slot < 4; ++slot) {
            int score = battle_move_score(
                battle, battler, target, slot);
            if (score > best_score
                || (score == best_score
                    && (remaster_emerald_battle_random(&battle->rng) & 1u))) {
                best_score = score;
                best_slot = slot;
            }
        }

        if (best_score == INT_MIN)
            return 0;
    }

    memset(out_action, 0, sizeof(*out_action));
    out_action->kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
    out_action->move_slot = best_slot;
    out_action->target = target;
    return 1;
}

static int battle_action_priority(
    const RemasterEmeraldBattleState *battle,
    uint8_t battler,
    const RemasterEmeraldBattleAction *action)
{
    const RemasterEmeraldMoveInfo *move;

    if (battle == 0
        || action == 0
        || action->kind != REMASTER_EMERALD_BATTLE_ACTION_MOVE
        || action->move_slot >= 4)
        return 0;

    move = remaster_emerald_move_info(
        battle->battlers[battler].moves[action->move_slot]);
    return move != 0 ? (int)move->priority : 0;
}

static void battle_end_turn(
    RemasterEmeraldBattleState *battle)
{
    uint8_t battler;

    if (battle == 0 || battle->ended)
        return;

    for (battler = 0;
         battler < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
         ++battler) {
        RemasterEmeraldBattleMon *mon = &battle->battlers[battler];
        uint16_t damage = 0;
        uint16_t heal = 0;
        uint8_t hold;

        if (!mon->active || mon->fainted)
            continue;

        if (mon->pokemon.status & REMASTER_EMERALD_STATUS1_TOXIC) {
            if (mon->toxic_counter < 15)
                mon->toxic_counter++;
            damage = (uint16_t)(
                (uint32_t)mon->pokemon.max_hp
                * mon->toxic_counter / 16u);
        } else if (mon->pokemon.status
            & REMASTER_EMERALD_STATUS1_POISON) {
            damage = mon->pokemon.max_hp / 8u;
        } else if (mon->pokemon.status
            & REMASTER_EMERALD_STATUS1_BURN) {
            damage = mon->pokemon.max_hp / 8u;
        }

        if (damage != 0) {
            if (damage == 0)
                damage = 1;
            battle_damage_direct(
                battle,
                battler,
                battler,
                damage,
                0,
                0);
            battle_faint_check(battle, battler);
            if (mon->fainted)
                continue;
        }

        if (battle_weather_has_effect(battle)
            && (battle->weather
                    == REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM
                || battle->weather
                    == REMASTER_EMERALD_BATTLE_WEATHER_HAIL)) {
            int immune = 0;
            if (battle->weather
                == REMASTER_EMERALD_BATTLE_WEATHER_SANDSTORM) {
                immune = mon->types[0] == TYPE_ROCK
                    || mon->types[1] == TYPE_ROCK
                    || mon->types[0] == TYPE_GROUND
                    || mon->types[1] == TYPE_GROUND
                    || mon->types[0] == TYPE_STEEL
                    || mon->types[1] == TYPE_STEEL;
            } else {
                immune = mon->types[0] == TYPE_ICE
                    || mon->types[1] == TYPE_ICE;
            }
            if (!immune) {
                damage = mon->pokemon.max_hp / 16u;
                if (damage == 0)
                    damage = 1;
                battle_damage_direct(
                    battle,
                    battler,
                    battler,
                    damage,
                    0,
                    0);
                battle_faint_check(battle, battler);
                if (mon->fainted)
                    continue;
            }
        }

        hold = battle_hold_effect(mon);
        if (hold == HOLD_EFFECT_LEFTOVERS) {
            heal = mon->pokemon.max_hp / 16u;
        } else if (hold == HOLD_EFFECT_RESTORE_HP
            && mon->pokemon.hp * 2u <= mon->pokemon.max_hp) {
            heal = battle_hold_param(mon);
            mon->last_consumed_item = mon->held_item;
            mon->held_item = 0;
        }

        if (mon->ability == ABILITY_RAIN_DISH
            && battle_weather_has_effect(battle)
            && battle->weather == REMASTER_EMERALD_BATTLE_WEATHER_RAIN)
            heal = (uint16_t)(heal + mon->pokemon.max_hp / 16u);
        if (mon->status3 & REMASTER_EMERALD_STATUS3_ROOTED)
            heal = (uint16_t)(heal + mon->pokemon.max_hp / 16u);

        if (heal != 0)
            battle_heal(battle, battler, heal, 0);

        if (mon->status3 & REMASTER_EMERALD_STATUS3_LEECH_SEED) {
            uint8_t source = battle_default_target(battle, battler);
            damage = mon->pokemon.max_hp / 8u;
            if (damage == 0)
                damage = 1;
            battle_damage_direct(
                battle,
                source,
                battler,
                damage,
                0,
                0);
            if (battle_valid_battler(source))
                battle_heal(battle, source, damage, 0);
            battle_faint_check(battle, battler);
        }

        if (mon->trapped_turns != 0) {
            damage = mon->pokemon.max_hp / 16u;
            if (damage == 0)
                damage = 1;
            battle_damage_direct(
                battle, battler, battler, damage, mon->trapped_move, 0);
            mon->trapped_turns--;
            if (mon->trapped_turns == 0) {
                mon->status2 &= ~REMASTER_EMERALD_STATUS2_WRAPPED;
                mon->trapped_move = 0;
            }
            battle_faint_check(battle, battler);
            if (mon->fainted)
                continue;
        }

        if ((mon->status2 & REMASTER_EMERALD_STATUS2_NIGHTMARE)
            && (mon->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP)) {
            damage = mon->pokemon.max_hp / 4u;
            if (damage == 0)
                damage = 1;
            battle_damage_direct(
                battle, battler, battler, damage, 0, 0);
            battle_faint_check(battle, battler);
            if (mon->fainted)
                continue;
        } else if (!(mon->pokemon.status & REMASTER_EMERALD_STATUS1_SLEEP)) {
            mon->status2 &= ~REMASTER_EMERALD_STATUS2_NIGHTMARE;
        }

        if (mon->status2 & REMASTER_EMERALD_STATUS2_CURSED) {
            damage = mon->pokemon.max_hp / 4u;
            if (damage == 0)
                damage = 1;
            battle_damage_direct(
                battle, battler, battler, damage, 0, 0);
            battle_faint_check(battle, battler);
            if (mon->fainted)
                continue;
        }

        if (mon->perish_count != 0) {
            mon->perish_count--;
            if (mon->perish_count == 0) {
                mon->pokemon.hp = 0;
                battle_faint_check(battle, battler);
            }
        }

        if ((mon->status3 & REMASTER_EMERALD_STATUS3_YAWN) != 0) {
            uint32_t yawn = (mon->status3
                & REMASTER_EMERALD_STATUS3_YAWN) >> 11u;
            if (yawn > 0)
                yawn--;
            mon->status3 &=
                ~REMASTER_EMERALD_STATUS3_YAWN;
            mon->status3 |= yawn << 11u;
            if (yawn == 0)
                battle_apply_status(
                    battle,
                    battler,
                    battler,
                    REMASTER_EMERALD_STATUS1_SLEEP,
                    0);
        }

        if (mon->ability == ABILITY_SHED_SKIN
            && mon->pokemon.status != 0
            && remaster_emerald_battle_random(&battle->rng) % 3u == 0)
            mon->pokemon.status = 0;

        if (mon->taunt_turns != 0)
            mon->taunt_turns--;
        if (mon->encore_turns != 0)
            mon->encore_turns--;
        if (mon->disable_turns != 0)
            mon->disable_turns--;
        if (mon->lock_on_turns != 0) {
            mon->lock_on_turns--;
            if (mon->lock_on_turns == 0) {
                mon->status3 &= ~REMASTER_EMERALD_STATUS3_ALWAYS_HITS;
                mon->lock_on_target = 0xFFu;
            }
        }

        mon->protected_turn = 0xFFu;
        mon->endure_turn = 0xFFu;
        battle_sync_battler(battle, battler);
    }

    for (battler = 0;
         battler < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
         ++battler) {
        RemasterEmeraldBattleMon *mon = &battle->battlers[battler];

        if (battle->future_turns[battler] != 0) {
            battle->future_turns[battler]--;
            if (battle->future_turns[battler] == 0
                && mon->active
                && !mon->fainted) {
                uint8_t source = battle->future_attacker[battler];
                uint16_t move_id = battle->future_move[battler];
                uint16_t damage = battle->future_damage[battler];
                battle_damage_direct(
                    battle,
                    battle_valid_battler(source) ? source : battler,
                    battler,
                    damage != 0 ? damage : 1u,
                    move_id,
                    0);
                battle_faint_check(battle, battler);
                battle->future_attacker[battler] = 0xFFu;
                battle->future_move[battler] = 0;
                battle->future_damage[battler] = 0;
            }
        }

        if (battle->wish_turns[battler] != 0) {
            battle->wish_turns[battler]--;
            if (battle->wish_turns[battler] == 0
                && mon->active
                && !mon->fainted) {
                battle_heal(
                    battle,
                    battler,
                    battle->wish_amount[battler],
                    0);
                battle->wish_amount[battler] = 0;
            }
        }

        if (mon->active) {
            mon->helping_hand = 0;
            mon->magic_coat = 0;
            mon->snatch = 0;
            battle_sync_battler(battle, battler);
        }
    }

    battle->follow_me_turns[0] = 0;
    battle->follow_me_turns[1] = 0;
    battle->follow_me_target[0] = 0xFFu;
    battle->follow_me_target[1] = 0xFFu;

    if (battle->weather_turns != 0) {
        battle->weather_turns--;
        if (battle->weather_turns == 0)
            battle_set_weather(
                battle,
                REMASTER_EMERALD_BATTLE_WEATHER_NONE,
                0,
                0);
    }

    battle_finish_if_over(battle);
}

static int battle_handle_pending_replacements(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS],
    int *out_handled)
{
    uint8_t battler;
    int pending = 0;

    if (battle == 0 || actions == 0 || out_handled == 0)
        return 0;

    *out_handled = 0;

    for (battler = 0;
         battler < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
         ++battler) {
        uint8_t party_slot;
        uint8_t target;

        if (!remaster_emerald_battle_needs_replacement(
                battle, battler))
            continue;

        pending = 1;
        if (battle->battlers[battler].side == 0) {
            if (actions[battler].kind
                    == REMASTER_EMERALD_BATTLE_ACTION_SWITCH
                && remaster_emerald_battle_replace_fainted(
                    battle,
                    battler,
                    actions[battler].party_slot))
                continue;
            continue;
        }

        target = battle_default_target(battle, battler);
        if (!battle_valid_battler(target)
            || battle->battlers[target].fainted
            || battle->battlers[target].side
                == battle->battlers[battler].side) {
            uint8_t candidate;
            target = 0xFFu;
            for (candidate = 0;
                 candidate < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
                 ++candidate) {
                if (battle->battlers[candidate].active
                    && !battle->battlers[candidate].fainted
                    && battle->battlers[candidate].side == 0) {
                    target = candidate;
                    break;
                }
            }
        }

        if (target != 0xFFu
            && battle_ai_find_best_switch(
                battle,
                battler,
                target,
                0,
                &party_slot)) {
            remaster_emerald_battle_replace_fainted(
                battle, battler, party_slot);
        } else if (battle_find_switch_slot(
                battle, 1, &party_slot)) {
            remaster_emerald_battle_replace_fainted(
                battle, battler, party_slot);
        }
    }

    if (pending)
        *out_handled = 1;
    return 1;
}

static void battle_resolve_pursuit_on_switch(
    RemasterEmeraldBattleState *battle,
    uint8_t switcher,
    RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS])
{
    uint8_t pursuer;

    if (battle == 0
        || actions == 0
        || !battle_valid_battler(switcher)
        || battle->battlers[switcher].fainted)
        return;

    for (pursuer = 0;
         pursuer < REMASTER_EMERALD_BATTLE_MAX_BATTLERS;
         ++pursuer) {
        RemasterEmeraldBattleAction *action;
        const RemasterEmeraldMoveInfo *move;

        if (!battle->battlers[pursuer].active
            || battle->battlers[pursuer].fainted
            || battle->battlers[pursuer].side
                == battle->battlers[switcher].side)
            continue;

        action = &actions[pursuer];
        if (action->kind != REMASTER_EMERALD_BATTLE_ACTION_MOVE
            || action->move_slot >= 4
            || action->target != switcher)
            continue;

        move = remaster_emerald_move_info(
            battle->battlers[pursuer].moves[action->move_slot]);
        if (move == 0 || move->effect != EFFECT_PURSUIT)
            continue;
        if (battle->battlers[pursuer].pokemon.status
            & (REMASTER_EMERALD_STATUS1_SLEEP
               | REMASTER_EMERALD_STATUS1_FREEZE))
            continue;

        action->kind = REMASTER_EMERALD_BATTLE_ACTION_NONE;
        battle->pursuit_boost[pursuer] = 1;
        remaster_emerald_battle_use_move(
            battle,
            pursuer,
            switcher,
            action->move_slot);
        battle->pursuit_boost[pursuer] = 0;

        if (battle->battlers[switcher].fainted || battle->ended)
            return;
    }
}

int remaster_emerald_battle_resolve_turn(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS])
{
    RemasterEmeraldBattleAction resolved[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint8_t order[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint8_t count = 0;
    uint8_t sortable_start = 0;
    uint8_t run_battler = 0xFFu;
    uint16_t turn_random;
    uint8_t i;
    uint8_t j;
    int replacement_handled = 0;

    if (battle == 0 || actions == 0 || battle->ended)
        return 0;

    if (!battle_handle_pending_replacements(
            battle, actions, &replacement_handled))
        return 0;
    if (replacement_handled)
        return 1;

    battle->turn_number++;
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_TURN_STARTED,
        0,
        0,
        0,
        (int32_t)battle->turn_number,
        0);

    /* Emerald snapshots one shared random value for Quick Claw before
       action selection, then uses ordinary RNG only for exact speed ties. */
    turn_random = remaster_emerald_battle_random(&battle->rng);
    memcpy(resolved, actions, sizeof(resolved));

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (!battle->battlers[i].active || battle->battlers[i].fainted)
            continue;

        if (battle->battlers[i].charging_move != 0) {
            resolved[i].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
            resolved[i].move_slot =
                battle->battlers[i].charging_move_slot;
            resolved[i].target =
                battle->battlers[i].charging_target;
        } else if (battle->battlers[i].rampage_turns != 0
            || battle->battlers[i].uproar_turns != 0) {
            uint8_t slot;
            for (slot = 0; slot < 4; ++slot) {
                if (battle->battlers[i].moves[slot]
                    == battle->battlers[i].last_move)
                    break;
            }
            if (slot < 4) {
                resolved[i].kind = REMASTER_EMERALD_BATTLE_ACTION_MOVE;
                resolved[i].move_slot = slot;
                resolved[i].target = battle_default_target(battle, i);
            }
        } else if (resolved[i].kind == REMASTER_EMERALD_BATTLE_ACTION_NONE
            && battle->battlers[i].side == 1) {
            remaster_emerald_battle_choose_ai_action(
                battle, i, &resolved[i]);
        }
    }

    memset(battle->pursuit_boost, 0, sizeof(battle->pursuit_boost));

    if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_SAFARI) {
        for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
            if (battle->battlers[i].active
                && !battle->battlers[i].fainted
                && resolved[i].kind != REMASTER_EMERALD_BATTLE_ACTION_NONE)
                order[count++] = i;
        }
        sortable_start = count;
    } else {
        if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_LINK) {
            for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
                if (battle->battlers[i].active
                    && !battle->battlers[i].fainted
                    && resolved[i].kind
                        == REMASTER_EMERALD_BATTLE_ACTION_RUN) {
                    run_battler = i;
                    break;
                }
            }
        } else {
            for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; i += 2) {
                if (battle->battlers[i].active
                    && !battle->battlers[i].fainted
                    && resolved[i].kind
                        == REMASTER_EMERALD_BATTLE_ACTION_RUN)
                    run_battler = i;
            }
        }

        if (run_battler != 0xFFu) {
            order[count++] = run_battler;
            for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
                if (i != run_battler
                    && battle->battlers[i].active
                    && !battle->battlers[i].fainted
                    && resolved[i].kind
                        != REMASTER_EMERALD_BATTLE_ACTION_NONE)
                    order[count++] = i;
            }
            sortable_start = count;
        } else {
            for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
                if (!battle->battlers[i].active
                    || battle->battlers[i].fainted)
                    continue;
                if (resolved[i].kind == REMASTER_EMERALD_BATTLE_ACTION_ITEM
                    || resolved[i].kind
                        == REMASTER_EMERALD_BATTLE_ACTION_SWITCH)
                    order[count++] = i;
            }
            sortable_start = count;
            for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
                if (!battle->battlers[i].active
                    || battle->battlers[i].fainted)
                    continue;
                if (resolved[i].kind != REMASTER_EMERALD_BATTLE_ACTION_NONE
                    && resolved[i].kind
                        != REMASTER_EMERALD_BATTLE_ACTION_ITEM
                    && resolved[i].kind
                        != REMASTER_EMERALD_BATTLE_ACTION_SWITCH)
                    order[count++] = i;
            }
        }
    }

    for (i = sortable_start; i < count; ++i) {
        for (j = (uint8_t)(i + 1u); j < count; ++j) {
            uint8_t a = order[i];
            uint8_t b = order[j];
            int pa = battle_action_priority(battle, a, &resolved[a]);
            int pb = battle_action_priority(battle, b, &resolved[b]);
            uint32_t sa = battle_speed(battle, a);
            uint32_t sb = battle_speed(battle, b);
            int swap = 0;

            if (battle_hold_effect(&battle->battlers[a])
                    == HOLD_EFFECT_QUICK_CLAW
                && turn_random
                    < (uint32_t)UINT16_MAX
                        * battle_hold_param(&battle->battlers[a]) / 100u)
                sa = UINT32_MAX;
            if (battle_hold_effect(&battle->battlers[b])
                    == HOLD_EFFECT_QUICK_CLAW
                && turn_random
                    < (uint32_t)UINT16_MAX
                        * battle_hold_param(&battle->battlers[b]) / 100u)
                sb = UINT32_MAX;

            if (pb > pa)
                swap = 1;
            else if (pb == pa && sb > sa)
                swap = 1;
            else if (pb == pa && sb == sa
                && (remaster_emerald_battle_random(&battle->rng) & 1u))
                swap = 1;

            if (swap) {
                order[i] = b;
                order[j] = a;
            }
        }
    }

    for (i = 0; i < count && !battle->ended; ++i) {
        uint8_t battler = order[i];
        RemasterEmeraldBattleAction *action = &resolved[battler];

        if (battle->battlers[battler].fainted
            || action->kind == REMASTER_EMERALD_BATTLE_ACTION_NONE)
            continue;

        switch (action->kind) {
        case REMASTER_EMERALD_BATTLE_ACTION_MOVE: {
            const RemasterEmeraldMoveInfo *chosen =
                action->move_slot < 4
                ? remaster_emerald_move_info(
                    battle->battlers[battler].moves[action->move_slot])
                : 0;

            if (chosen != 0 && chosen->effect == EFFECT_BATON_PASS) {
                uint8_t slot = action->party_slot;
                uint8_t side = battle->battlers[battler].side;

                if (slot >= battle->party_count[side]
                    || battle->parties[side][slot].hp == 0
                    || battle_party_slot_active(battle, side, slot)) {
                    if (!battle_find_switch_slot(battle, side, &slot))
                        break;
                }

                if (remaster_emerald_battle_use_move(
                        battle,
                        battler,
                        action->target,
                        action->move_slot))
                    battle_baton_pass(battle, battler, slot);
            } else {
                remaster_emerald_battle_use_move(
                    battle,
                    battler,
                    action->target,
                    action->move_slot);
            }
            break;
        }
        case REMASTER_EMERALD_BATTLE_ACTION_SWITCH:
            if ((battle->battle_type_flags
                    & REMASTER_EMERALD_BATTLE_TYPE_ARENA)
                && !battle->battlers[battler].fainted)
                break;
            battle_resolve_pursuit_on_switch(
                battle, battler, resolved);
            if (!battle->battlers[battler].fainted && !battle->ended)
                remaster_emerald_battle_switch(
                    battle,
                    battler,
                    action->party_slot);
            break;
        case REMASTER_EMERALD_BATTLE_ACTION_ITEM:
            battle_use_trainer_item(
                battle,
                battler,
                action->item_id);
            break;
        case REMASTER_EMERALD_BATTLE_ACTION_RUN:
            remaster_emerald_battle_try_run(battle, battler);
            break;
        default:
            break;
        }
    }

    if (!battle->ended)
        battle_end_turn(battle);

    return 1;
}

int remaster_emerald_battle_try_run(
    RemasterEmeraldBattleState *battle,
    uint8_t battler)
{
    uint8_t target;
    uint32_t player_speed;
    uint32_t foe_speed;
    uint32_t threshold;

    if (battle == 0
        || !battle_valid_battler(battler)
        || battle->ended
        || battle->battlers[battler].side != 0)
        return 0;

    if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_TRAINER)
        return 0;

    target = battle_default_target(battle, battler);
    if (!battle_valid_battler(target))
        return 0;

    player_speed = battle_speed(battle, battler);
    foe_speed = battle_speed(battle, target);

    if (player_speed > foe_speed) {
        threshold = 256;
    } else {
        threshold = player_speed * 128u / foe_speed
            + battle->turn_number * 30u;
    }

    if (threshold > 255u
        || (remaster_emerald_battle_random(&battle->rng) & 0xFFu)
            < threshold) {
        battle->outcome = REMASTER_EMERALD_BATTLE_OUTCOME_RAN;
        battle->ended = 1;
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_RUN,
            battler,
            target,
            0,
            1,
            0);
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_ENDED,
            battler,
            target,
            0,
            battle->outcome,
            0);
        return 1;
    }

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_RUN,
        battler,
        target,
        0,
        0,
        0);
    return 0;
}

static uint32_t battle_isqrt(uint32_t value)
{
    uint32_t bit = UINT32_C(1) << 30u;
    uint32_t result = 0;

    while (bit > value)
        bit >>= 2u;

    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1u) + bit;
        } else {
            result >>= 1u;
        }
        bit >>= 2u;
    }
    return result;
}

static uint8_t battle_ball_multiplier(
    const RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleMon *target,
    uint16_t ball)
{
    switch (ball) {
    case ITEM_MASTER_BALL:
        return 255;
    case ITEM_ULTRA_BALL:
        return 40;
    case ITEM_GREAT_BALL:
        return 15;
    case ITEM_POKE_BALL:
        return 10;
    case ITEM_SAFARI_BALL:
        return 15;
    case ITEM_NET_BALL:
        return (target->types[0] == TYPE_WATER
                || target->types[1] == TYPE_WATER
                || target->types[0] == TYPE_BUG
                || target->types[1] == TYPE_BUG)
            ? 30u
            : 10u;
    case ITEM_DIVE_BALL:
        return 10;
    case ITEM_NEST_BALL:
        if (target->pokemon.level < 40) {
            uint8_t value = (uint8_t)(40u - target->pokemon.level);
            return value < 10 ? 10u : value;
        }
        return 10;
    case ITEM_REPEAT_BALL:
        return 10;
    case ITEM_TIMER_BALL: {
        uint32_t value = battle->turn_number + 10u;
        return (uint8_t)(value > 40u ? 40u : value);
    }
    case ITEM_LUXURY_BALL:
    case ITEM_PREMIER_BALL:
        return 10;
    default:
        return 10;
    }
}

int remaster_emerald_battle_throw_ball(
    RemasterEmeraldBattleState *battle,
    uint8_t battler,
    uint8_t target_id,
    uint16_t ball_item_id)
{
    RemasterEmeraldBattleMon *target;
    const RemasterEmeraldSpeciesInfo *species;
    uint32_t odds;
    uint32_t shake_limit;
    uint32_t root;
    uint8_t multiplier;
    uint8_t shakes = 0;

    if (battle == 0
        || battle->ended
        || !battle_valid_battler(battler)
        || !battle_valid_battler(target_id)
        || battle->battlers[battler].side != 0
        || battle->battlers[target_id].side != 1)
        return 0;

    if (battle->battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_TRAINER)
        return 0;

    target = &battle->battlers[target_id];
    species = remaster_emerald_species_info(target->species);
    if (species == 0 || target->pokemon.max_hp == 0)
        return 0;

    multiplier = battle_ball_multiplier(
        battle, target, ball_item_id);

    if (ball_item_id == ITEM_MASTER_BALL) {
        odds = 255;
    } else {
        odds = ((uint32_t)species->catch_rate * multiplier / 10u)
            * ((uint32_t)target->pokemon.max_hp * 3u
                - (uint32_t)target->pokemon.hp * 2u)
            / ((uint32_t)target->pokemon.max_hp * 3u);

        if (target->pokemon.status
            & (REMASTER_EMERALD_STATUS1_SLEEP
                | REMASTER_EMERALD_STATUS1_FREEZE))
            odds *= 2u;
        if (target->pokemon.status
            & (REMASTER_EMERALD_STATUS1_POISON
                | REMASTER_EMERALD_STATUS1_BURN
                | REMASTER_EMERALD_STATUS1_PARALYSIS
                | REMASTER_EMERALD_STATUS1_TOXIC))
            odds = odds * 15u / 10u;
    }

    if (odds > 254u || ball_item_id == ITEM_MASTER_BALL) {
        shakes = 4;
    } else if (odds != 0) {
        root = battle_isqrt(UINT32_C(16711680) / odds);
        root = battle_isqrt(root);
        if (root != 0)
            shake_limit = UINT32_C(1048560) / root;
        else
            shake_limit = UINT32_MAX;

        while (shakes < 4
            && remaster_emerald_battle_random(&battle->rng)
                < shake_limit)
            shakes++;
    }

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_CAPTURE_SHAKE,
        battler,
        target_id,
        0,
        shakes,
        ball_item_id);

    if (shakes == 4) {
        battle->caught_valid = 1;
        battle->caught_pokemon = target->pokemon;
        remaster_emerald_box_pokemon_set_pokeball(
            &battle->caught_pokemon.box,
            (uint8_t)ball_item_id);
        battle->caught_pokemon.box.checksum =
            remaster_emerald_box_pokemon_checksum(
                &battle->caught_pokemon.box);
        battle->outcome = REMASTER_EMERALD_BATTLE_OUTCOME_CAUGHT;
        battle->ended = 1;
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_CAPTURED,
            battler,
            target_id,
            0,
            target->species,
            ball_item_id);
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_ENDED,
            battler,
            target_id,
            0,
            battle->outcome,
            0);
        return 1;
    }

    return 0;
}

uint32_t remaster_emerald_battle_exp_gain(
    const RemasterEmeraldBattleMon *defeated,
    uint8_t participant_count,
    int trainer_battle)
{
    const RemasterEmeraldSpeciesInfo *species;
    uint32_t exp;

    if (defeated == 0 || participant_count == 0)
        return 0;

    species = remaster_emerald_species_info(defeated->species);
    if (species == 0)
        return 0;

    exp = (uint32_t)species->exp_yield
        * defeated->pokemon.level
        / 7u;
    if (trainer_battle)
        exp = exp * 150u / 100u;
    exp /= participant_count;
    return exp != 0 ? exp : 1;
}

int remaster_emerald_battle_commit_player_party(
    const RemasterEmeraldBattleState *battle,
    RemasterEmeraldSave *save)
{
    uint8_t count;
    uint8_t i;

    if (battle == 0 || save == 0)
        return 0;

    count = battle->party_count[0];
    if (!remaster_emerald_party_set_count(save, count))
        return 0;

    for (i = 0; i < count; ++i) {
        if (!remaster_emerald_party_set(
                save,
                i,
                &battle->parties[0][i]))
            return 0;
    }
    return 1;
}

int remaster_emerald_battle_store_caught(
    const RemasterEmeraldBattleState *battle,
    RemasterEmeraldSave *save,
    size_t *out_box,
    size_t *out_slot)
{
    uint8_t party_count;
    size_t box;
    size_t slot;

    if (battle == 0
        || save == 0
        || !battle->caught_valid)
        return 0;

    party_count = remaster_emerald_party_count(save);
    if (party_count < REMASTER_EMERALD_PARTY_SIZE) {
        if (!remaster_emerald_party_set(
                save,
                party_count,
                &battle->caught_pokemon)
            || !remaster_emerald_party_set_count(
                save,
                (uint8_t)(party_count + 1u)))
            return 0;
        if (out_box != 0)
            *out_box = SIZE_MAX;
        if (out_slot != 0)
            *out_slot = party_count;
        return 1;
    }

    for (box = 0;
         box < REMASTER_EMERALD_STORAGE_BOX_COUNT;
         ++box) {
        for (slot = 0;
             slot < REMASTER_EMERALD_STORAGE_BOX_CAPACITY;
             ++slot) {
            RemasterEmeraldBoxPokemon existing;
            if (!remaster_emerald_storage_get(
                    save,
                    box,
                    slot,
                    &existing,
                    0))
                return 0;
            if (remaster_emerald_box_pokemon_species(&existing) == 0) {
                if (!remaster_emerald_storage_set(
                        save,
                        box,
                        slot,
                        &battle->caught_pokemon.box))
                    return 0;
                if (out_box != 0)
                    *out_box = box;
                if (out_slot != 0)
                    *out_slot = slot;
                return 1;
            }
        }
    }

    return 0;
}
