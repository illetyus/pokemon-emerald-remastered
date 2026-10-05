#include "remaster/emerald_battle.h"

#include "remaster/emerald_items.h"
#include "remaster/emerald_state.h"

#include <limits.h>
#include <string.h>

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
    ABILITY_LIMBER = 7,
    ABILITY_SAND_VEIL = 8,
    ABILITY_STATIC = 9,
    ABILITY_VOLT_ABSORB = 10,
    ABILITY_WATER_ABSORB = 11,
    ABILITY_CLOUD_NINE = 13,
    ABILITY_COMPOUND_EYES = 14,
    ABILITY_INSOMNIA = 15,
    ABILITY_IMMUNITY = 17,
    ABILITY_FLASH_FIRE = 18,
    ABILITY_OWN_TEMPO = 20,
    ABILITY_INTIMIDATE = 22,
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
    ABILITY_DROUGHT = 70,
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

    SPECIES_PIKACHU = 25,
    SPECIES_FARFETCHD = 83,
    SPECIES_CUBONE = 104,
    SPECIES_MAROWAK = 105,
    SPECIES_CHANSEY = 113,
    SPECIES_DITTO = 132,
    SPECIES_CLAMPERL = 373,
    SPECIES_LATIAS = 407,
    SPECIES_LATIOS = 408,

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
    out->entered_turn = 0;
    out->last_damage_from = 0xFFu;
    out->lock_on_target = 0xFFu;
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

void remaster_emerald_battle_state_init(
    RemasterEmeraldBattleState *battle,
    uint32_t battle_type_flags,
    uint32_t seed)
{
    if (battle == 0)
        return;

    memset(battle, 0, sizeof(*battle));
    battle->battle_type_flags = battle_type_flags;
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
    default:
        return move->power;
    }
}

int remaster_emerald_battle_calculate_damage(
    RemasterEmeraldBattleState *battle,
    uint8_t attacker_id,
    uint8_t defender_id,
    uint16_t move_id,
    RemasterEmeraldBattleDamageResult *out_result)
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

    accuracy = battle_accuracy_percent(
        battle,
        attacker,
        defender,
        move);
    if (remaster_emerald_battle_random(&battle->rng) % 100u
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

    type_multiplier = battle_type_multiplier(defender, move_type);

    if (defender->ability == ABILITY_VOLT_ABSORB
        && move_type == TYPE_ELECTRIC)
        type_multiplier = 0;
    if (defender->ability == ABILITY_WATER_ABSORB
        && move_type == TYPE_WATER)
        type_multiplier = 0;
    if (defender->ability == ABILITY_FLASH_FIRE
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
    critical = battle_critical(
        battle,
        attacker,
        defender,
        move->effect);
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

    damage += 2u;

    if (critical)
        damage *= 2u;

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

static int battle_status_allowed(
    const RemasterEmeraldBattleMon *target,
    uint32_t status)
{
    if (target == 0 || target->pokemon.status != 0)
        return 0;

    if ((status & REMASTER_EMERALD_STATUS1_SLEEP)
        && (target->ability == ABILITY_INSOMNIA
            || target->ability == ABILITY_VITAL_SPIRIT))
        return 0;
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
    if (!battle_status_allowed(target, status))
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

    if (target->substitute_hp != 0) {
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
    if (false_swipe
        && actual >= target->pokemon.hp
        && target->pokemon.hp > 1)
        actual = (uint16_t)(target->pokemon.hp - 1u);

    target->pokemon.hp = (uint16_t)(target->pokemon.hp - actual);
    target->last_damage = actual;

    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_DAMAGE,
        source,
        target_id,
        move_id,
        actual,
        0);
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
    uint32_t turns;
    uint16_t self_damage;

    if (battle == 0 || !battle_valid_battler(battler))
        return 0;

    mon = &battle->battlers[battler];

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
        if (turns != 0)
            return 0;
        battle_event(
            battle,
            REMASTER_EMERALD_BATTLE_EVENT_STATUS,
            battler,
            battler,
            move_id,
            0,
            REMASTER_EMERALD_STATUS1_SLEEP);
    }

    if (mon->pokemon.status & REMASTER_EMERALD_STATUS1_FREEZE) {
        if (remaster_emerald_battle_random(&battle->rng) % 5u != 0)
            return 0;
        mon->pokemon.status &= ~REMASTER_EMERALD_STATUS1_FREEZE;
    }

    if ((mon->pokemon.status & REMASTER_EMERALD_STATUS1_PARALYSIS)
        && remaster_emerald_battle_random(&battle->rng) % 4u == 0)
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

static void battle_faint_check(
    RemasterEmeraldBattleState *battle,
    uint8_t battler);

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
    battle_sync_battler(battle, battler);
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_FAINT,
        battler,
        battler,
        0,
        0,
        0);

    battle_award_exp_for_faint(battle, battler);
    battle_finish_if_over(battle);
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

    target = &battle->battlers[target_id];
    move_id = attacker->moves[move_slot];
    move = remaster_emerald_move_info(move_id);
    if (move == 0 || move_id == 0 || attacker->pp[move_slot] == 0)
        return 0;

    if (attacker->disable_turns != 0
        && attacker->disable_move_slot == move_slot)
        return 0;
    if (attacker->encore_turns != 0
        && attacker->encore_move_slot != move_slot)
        return 0;
    if (attacker->taunt_turns != 0 && move->power == 0)
        return 0;
    if (battle_hold_effect(attacker) == HOLD_EFFECT_CHOICE_BAND
        && attacker->choice_locked_move != 0
        && attacker->choice_locked_move != move_id)
        return 0;

    if (target->ability == ABILITY_PRESSURE)
        pp_cost = 2;
    if (pp_cost > attacker->pp[move_slot])
        pp_cost = attacker->pp[move_slot];
    attacker->pp[move_slot] = (uint8_t)(attacker->pp[move_slot] - pp_cost);
    remaster_emerald_box_pokemon_set_move(
        &attacker->pokemon.box,
        move_slot,
        move_id,
        attacker->pp[move_slot]);
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

    if (!battle_can_act(battle, attacker_id, move_id)) {
        battle_faint_check(battle, attacker_id);
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

    if (move->effect == EFFECT_MULTI_HIT)
        hits = (uint8_t)(2u + remaster_emerald_battle_random(&battle->rng) % 4u);
    else if (move->effect == EFFECT_DOUBLE_HIT
        || move->effect == EFFECT_TWINEEDLE)
        hits = 2;

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
            if (hit == 0)
                return 1;
            break;
        }

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

    if (move->effect == EFFECT_ABSORB && total_damage != 0)
        battle_heal(
            battle,
            attacker_id,
            (uint16_t)(total_damage / 2u),
            move_id);

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

    battle_sync_battler(battle, attacker_id);
    battle_sync_battler(battle, target_id);
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
    }

    battle_entry_ability(battle, battler);
    battle_sync_battler(battle, battler);
    return 1;
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

    (void)battle;

    switch (action->kind) {
    case REMASTER_EMERALD_BATTLE_ACTION_SWITCH:
        return 60;
    case REMASTER_EMERALD_BATTLE_ACTION_ITEM:
        return 50;
    case REMASTER_EMERALD_BATTLE_ACTION_RUN:
        return 40;
    case REMASTER_EMERALD_BATTLE_ACTION_MOVE:
        move = remaster_emerald_move_info(
            battle->battlers[battler].moves[action->move_slot]);
        return move != 0 ? (int)move->priority : 0;
    default:
        return -100;
    }
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

        mon->protected_turn = 0xFFu;
        battle_sync_battler(battle, battler);
    }

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

int remaster_emerald_battle_resolve_turn(
    RemasterEmeraldBattleState *battle,
    const RemasterEmeraldBattleAction actions[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS])
{
    RemasterEmeraldBattleAction resolved[
        REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint8_t order[REMASTER_EMERALD_BATTLE_MAX_BATTLERS];
    uint8_t count = 0;
    uint8_t i;
    uint8_t j;

    if (battle == 0 || actions == 0 || battle->ended)
        return 0;

    battle->turn_number++;
    battle_event(
        battle,
        REMASTER_EMERALD_BATTLE_EVENT_TURN_STARTED,
        0,
        0,
        0,
        (int32_t)battle->turn_number,
        0);

    memcpy(resolved, actions, sizeof(resolved));

    for (i = 0; i < REMASTER_EMERALD_BATTLE_MAX_BATTLERS; ++i) {
        if (!battle->battlers[i].active || battle->battlers[i].fainted)
            continue;

        if (resolved[i].kind == REMASTER_EMERALD_BATTLE_ACTION_NONE
            && battle->battlers[i].side == 1) {
            remaster_emerald_battle_choose_ai_action(
                battle, i, &resolved[i]);
        }

        if (resolved[i].kind != REMASTER_EMERALD_BATTLE_ACTION_NONE)
            order[count++] = i;
    }

    for (i = 0; i < count; ++i) {
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
                && remaster_emerald_battle_random(&battle->rng) % 100u
                    < battle_hold_param(&battle->battlers[a]))
                pa++;
            if (battle_hold_effect(&battle->battlers[b])
                    == HOLD_EFFECT_QUICK_CLAW
                && remaster_emerald_battle_random(&battle->rng) % 100u
                    < battle_hold_param(&battle->battlers[b]))
                pb++;

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

        if (battle->battlers[battler].fainted)
            continue;

        switch (action->kind) {
        case REMASTER_EMERALD_BATTLE_ACTION_MOVE:
            remaster_emerald_battle_use_move(
                battle,
                battler,
                action->target,
                action->move_slot);
            break;
        case REMASTER_EMERALD_BATTLE_ACTION_SWITCH:
            remaster_emerald_battle_switch(
                battle,
                battler,
                action->party_slot);
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
