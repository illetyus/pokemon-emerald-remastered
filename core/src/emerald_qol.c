#include "remaster/emerald_qol.h"

#include "remaster/emerald_items.h"
#include "remaster/emerald_state.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    ITEM_NONE = 0,
    ITEM_SUN_STONE = 93,
    ITEM_MOON_STONE = 94,
    ITEM_FIRE_STONE = 95,
    ITEM_THUNDER_STONE = 96,
    ITEM_WATER_STONE = 97,
    ITEM_LEAF_STONE = 98,
    ITEM_FIRST_MAIL = 121,
    ITEM_LAST_MAIL = 132,
    ITEM_TM01 = 289,
    ITEM_TM50 = 338,
    ITEM_HM01 = 339,
    ITEM_HM02 = 340,
    ITEM_HM03 = 341,
    ITEM_HM04 = 342,
    ITEM_HM05 = 343,
    ITEM_HM06 = 344,
    ITEM_HM07 = 345,
    ITEM_HM08 = 346,

    MOVE_CUT = 15,
    MOVE_FLY = 19,
    MOVE_SURF = 57,
    MOVE_STRENGTH = 70,
    MOVE_WATERFALL = 127,
    MOVE_FLASH = 148,
    MOVE_ROCK_SMASH = 249,
    MOVE_DIVE = 291,

    FLAG_BADGE01_GET = 0x867,
    FLAG_BADGE02_GET = 0x868,
    FLAG_BADGE03_GET = 0x869,
    FLAG_BADGE04_GET = 0x86A,
    FLAG_BADGE05_GET = 0x86B,
    FLAG_BADGE06_GET = 0x86C,
    FLAG_BADGE07_GET = 0x86D,
    FLAG_BADGE08_GET = 0x86E,

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

    SB2_PLAYER_NAME = 0x0000,
    SB2_PLAYER_TRAINER_ID = 0x000A,
    SB2_ENCRYPTION_KEY = 0x00AC,
    SB1_REGISTERED_ITEM = 0x0496,
    SB1_PC_ITEMS = 0x0498,
    SB1_ITEMS = 0x0560,
    SB1_KEY_ITEMS = 0x05D8,
    SB1_POKE_BALLS = 0x0650,
    SB1_TM_HM = 0x0690,
    SB1_BERRIES = 0x0790,
    SB1_VANILLAPLUS_ITEM_META = 0x3598,

    ITEM_SLOT_BYTES = 4,
    PC_ITEMS_COUNT = 50,

    STORAGE_CURRENT_BOX = 0x0000,
    STORAGE_BOXES = 0x0004,

    VANILLAPLUS_ITEM_META_MAGIC = 0x35504956u,
    VANILLAPLUS_ITEM_META_VERSION = 1,
    VANILLAPLUS_ITEM_META_AUTO_SORT_MASK = 5,
    VANILLAPLUS_ITEM_META_BAG_SORT_MODES = 6,
    VANILLAPLUS_ITEM_META_BAG_SORT_COUNT = 5,
    VANILLAPLUS_ITEM_META_QUICK_COUNT = 11,
    VANILLAPLUS_ITEM_META_QUICK_ITEMS = 12,
    VANILLAPLUS_ITEM_META_LAST_QUICK = 20,
    VANILLAPLUS_ITEM_META_BYTES = 24,
    VANILLAPLUS_QUICK_ITEM_MAX = 4
};

typedef struct RemasterEmeraldQolItemView {
    size_t offset;
    size_t capacity;
    int encrypted_quantity;
} RemasterEmeraldQolItemView;

static uint8_t *ensure_item_meta(RemasterEmeraldSave *save);

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8u)
        | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
}

static void write16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static void write32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

static int item_is_mail(uint16_t item_id)
{
    return item_id >= ITEM_FIRST_MAIL && item_id <= ITEM_LAST_MAIL;
}

static int bag_view(uint8_t pocket, RemasterEmeraldQolItemView *out)
{
    if (out == 0)
        return 0;

    out->encrypted_quantity = 1;
    switch ((RemasterEmeraldBagPocket)pocket) {
    case REMASTER_EMERALD_POCKET_ITEMS:
        out->offset = SB1_ITEMS;
        out->capacity = 30;
        return 1;
    case REMASTER_EMERALD_POCKET_POKE_BALLS:
        out->offset = SB1_POKE_BALLS;
        out->capacity = 16;
        return 1;
    case REMASTER_EMERALD_POCKET_TM_HM:
        out->offset = SB1_TM_HM;
        out->capacity = 64;
        return 1;
    case REMASTER_EMERALD_POCKET_BERRIES:
        out->offset = SB1_BERRIES;
        out->capacity = 46;
        return 1;
    case REMASTER_EMERALD_POCKET_KEY_ITEMS:
        out->offset = SB1_KEY_ITEMS;
        out->capacity = 30;
        return 1;
    case REMASTER_EMERALD_POCKET_NONE:
    default:
        return 0;
    }
}

static int bag_metadata_index(uint8_t pocket)
{
    if (pocket < REMASTER_EMERALD_POCKET_ITEMS
        || pocket > REMASTER_EMERALD_POCKET_KEY_ITEMS)
        return -1;
    return (int)pocket - 1;
}

static uint16_t bag_key(const RemasterEmeraldSave *save)
{
    return save == 0
        ? 0
        : (uint16_t)read32(save->save_block2 + SB2_ENCRYPTION_KEY);
}

static uint8_t *item_slot_ptr(
    RemasterEmeraldSave *save,
    const RemasterEmeraldQolItemView *view,
    size_t slot)
{
    return save->save_block1 + view->offset + slot * ITEM_SLOT_BYTES;
}

static uint16_t item_slot_quantity(
    const RemasterEmeraldSave *save,
    const uint8_t *slot,
    int encrypted_quantity)
{
    const uint16_t raw = read16(slot + 2);
    return encrypted_quantity ? (uint16_t)(raw ^ bag_key(save)) : raw;
}

static void item_slot_clear(
    RemasterEmeraldSave *save,
    uint8_t *slot,
    int encrypted_quantity)
{
    write16(slot, ITEM_NONE);
    write16(slot + 2, encrypted_quantity ? bag_key(save) : 0);
}

static int compare_item_slots(
    const RemasterEmeraldSave *save,
    const uint8_t *a,
    const uint8_t *b,
    RemasterEmeraldQolItemSortMode mode,
    int encrypted_quantity)
{
    const uint16_t item_a = read16(a);
    const uint16_t item_b = read16(b);
    const RemasterEmeraldItemInfo *info_a;
    const RemasterEmeraldItemInfo *info_b;
    uint32_t key_a;
    uint32_t key_b;

    if (item_a == ITEM_NONE)
        return item_b == ITEM_NONE ? 0 : 1;
    if (item_b == ITEM_NONE)
        return -1;

    info_a = remaster_emerald_item_info(item_a);
    info_b = remaster_emerald_item_info(item_b);
    if (info_a == 0 || info_b == 0)
        return item_a < item_b ? -1 : item_a > item_b;

    switch (mode) {
    case REMASTER_EMERALD_QOL_ITEM_SORT_NAME:
        if (info_a->name_sort_rank != info_b->name_sort_rank)
            return info_a->name_sort_rank < info_b->name_sort_rank ? -1 : 1;
        break;
    case REMASTER_EMERALD_QOL_ITEM_SORT_TYPE:
        key_a = ((uint32_t)info_a->pocket << 8u) | info_a->type;
        key_b = ((uint32_t)info_b->pocket << 8u) | info_b->type;
        if (key_a != key_b)
            return key_a < key_b ? -1 : 1;
        break;
    case REMASTER_EMERALD_QOL_ITEM_SORT_QUANTITY:
        key_a = item_slot_quantity(save, a, encrypted_quantity);
        key_b = item_slot_quantity(save, b, encrypted_quantity);
        if (key_a != key_b)
            return key_a > key_b ? -1 : 1;
        break;
    case REMASTER_EMERALD_QOL_ITEM_SORT_VALUE:
        if (info_a->price != info_b->price)
            return info_a->price > info_b->price ? -1 : 1;
        break;
    case REMASTER_EMERALD_QOL_ITEM_SORT_NONE:
    case REMASTER_EMERALD_QOL_ITEM_SORT_COUNT:
    default:
        return 0;
    }

    return item_a < item_b ? -1 : item_a > item_b;
}

static size_t compact_item_slots(
    RemasterEmeraldSave *save,
    const RemasterEmeraldQolItemView *view)
{
    size_t read_slot;
    size_t write_slot = 0;

    for (read_slot = 0; read_slot < view->capacity; ++read_slot) {
        uint8_t *raw = item_slot_ptr(save, view, read_slot);
        if (read16(raw) == ITEM_NONE)
            continue;
        if (write_slot != read_slot) {
            memcpy(
                item_slot_ptr(save, view, write_slot),
                raw,
                ITEM_SLOT_BYTES);
            item_slot_clear(save, raw, view->encrypted_quantity);
        }
        ++write_slot;
    }

    while (write_slot < view->capacity) {
        item_slot_clear(
            save,
            item_slot_ptr(save, view, write_slot),
            view->encrypted_quantity);
        ++write_slot;
    }

    for (write_slot = 0;
         write_slot < view->capacity
             && read16(item_slot_ptr(save, view, write_slot)) != ITEM_NONE;
         ++write_slot) {
    }
    return write_slot;
}

static void sort_item_slots(
    RemasterEmeraldSave *save,
    const RemasterEmeraldQolItemView *view,
    size_t count,
    RemasterEmeraldQolItemSortMode mode)
{
    size_t i;

    for (i = 1; i < count; ++i) {
        uint8_t key[ITEM_SLOT_BYTES];
        size_t j = i;
        memcpy(key, item_slot_ptr(save, view, i), sizeof(key));

        while (j > 0
            && compare_item_slots(
                   save,
                   key,
                   item_slot_ptr(save, view, j - 1u),
                   mode,
                   view->encrypted_quantity) < 0) {
            memcpy(
                item_slot_ptr(save, view, j),
                item_slot_ptr(save, view, j - 1u),
                ITEM_SLOT_BYTES);
            --j;
        }
        memcpy(item_slot_ptr(save, view, j), key, sizeof(key));
    }
}

int remaster_emerald_qol_reusable_evolution_stone(uint16_t item_id)
{
    switch (item_id) {
    case ITEM_SUN_STONE:
    case ITEM_MOON_STONE:
    case ITEM_FIRE_STONE:
    case ITEM_THUNDER_STONE:
    case ITEM_WATER_STONE:
    case ITEM_LEAF_STONE:
        return 1;
    default:
        return 0;
    }
}

int remaster_emerald_qol_evolution_item_consumed(uint16_t item_id)
{
    return !remaster_emerald_qol_reusable_evolution_stone(item_id);
}

int remaster_emerald_qol_teach_item_consumed(uint16_t item_id)
{
    return !(item_id >= ITEM_TM01 && item_id <= ITEM_HM08);
}

int remaster_emerald_qol_move_replaceable(uint16_t move_id)
{
    return move_id != 0;
}

uint16_t remaster_emerald_qol_field_poison_hp_after_step(uint16_t hp)
{
    return hp > 1 ? (uint16_t)(hp - 1u) : hp;
}

uint8_t remaster_emerald_qol_flash_level_after_use(void)
{
    return 0;
}

uint16_t remaster_emerald_qol_fishing_response_frames(uint8_t rod)
{
    return rod <= 2 ? 90u : 0u;
}

uint8_t remaster_emerald_qol_fishing_required_rounds(
    uint8_t rod,
    uint16_t emerald_random_value)
{
    static const uint8_t min_round_modulus[3] = {1u, 3u, 6u};

    if (rod > 2)
        return 0;

    return (uint8_t)(1u + (emerald_random_value % min_round_modulus[rod]));
}

uint8_t remaster_emerald_qol_fishing_optional_round_chance(
    uint8_t rod,
    uint8_t round_index)
{
    if (rod > 2 || round_index >= 2)
        return 0;
    return 0;
}

int remaster_emerald_qol_running_requires_shoes(void)
{
    return 0;
}

int remaster_emerald_qol_running_environment_allows(
    int underwater,
    int metatile_disallowed)
{
    return !underwater && !metatile_disallowed;
}

int remaster_emerald_qol_bike_toggle_allowed(
    const RemasterEmeraldQolBikeToggleState *state)
{
    if (state == 0
        || !state->r_pressed
        || state->cycling_road
        || state->player_moving)
        return 0;

    if (state->on_mach_bike)
        return state->mach_speed_standing != 0;

    if (state->on_acro_bike)
        return state->acro_state_normal != 0;

    return 0;
}

int remaster_emerald_qol_hm_tool(
    uint16_t item_id,
    RemasterEmeraldQolHmTool *out_tool)
{
    RemasterEmeraldQolHmTool tool;

    tool.item_id = item_id;
    switch (item_id) {
    case ITEM_HM01:
        tool.move_id = MOVE_CUT;
        tool.badge_flag = FLAG_BADGE01_GET;
        break;
    case ITEM_HM02:
        tool.move_id = MOVE_FLY;
        tool.badge_flag = FLAG_BADGE06_GET;
        break;
    case ITEM_HM03:
        tool.move_id = MOVE_SURF;
        tool.badge_flag = FLAG_BADGE05_GET;
        break;
    case ITEM_HM04:
        tool.move_id = MOVE_STRENGTH;
        tool.badge_flag = FLAG_BADGE04_GET;
        break;
    case ITEM_HM05:
        tool.move_id = MOVE_FLASH;
        tool.badge_flag = FLAG_BADGE02_GET;
        break;
    case ITEM_HM06:
        tool.move_id = MOVE_ROCK_SMASH;
        tool.badge_flag = FLAG_BADGE03_GET;
        break;
    case ITEM_HM07:
        tool.move_id = MOVE_WATERFALL;
        tool.badge_flag = FLAG_BADGE08_GET;
        break;
    case ITEM_HM08:
        tool.move_id = MOVE_DIVE;
        tool.badge_flag = FLAG_BADGE07_GET;
        break;
    default:
        return 0;
    }

    if (out_tool != 0)
        *out_tool = tool;
    return 1;
}

int remaster_emerald_qol_hm_access(
    const RemasterEmeraldSave *save,
    uint16_t item_id)
{
    RemasterEmeraldQolHmTool tool;
    int badge = 0;

    if (save == 0
        || !remaster_emerald_qol_hm_tool(item_id, &tool)
        || remaster_emerald_bag_count(save, item_id) == 0
        || !remaster_emerald_flag_get(save, tool.badge_flag, &badge))
        return 0;

    return badge != 0;
}

int remaster_emerald_qol_box_pokemon_is_egg(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    uint32_t iv_word;

    if (pokemon == 0)
        return 0;

    iv_word =
        (uint32_t)pokemon->substruct[3][4]
        | ((uint32_t)pokemon->substruct[3][5] << 8u)
        | ((uint32_t)pokemon->substruct[3][6] << 16u)
        | ((uint32_t)pokemon->substruct[3][7] << 24u);
    return (int)((iv_word >> 30u) & 1u);
}

int remaster_emerald_qol_hm_field_actor(
    const RemasterEmeraldSave *save,
    uint8_t *out_party_slot)
{
    size_t slot;

    if (save == 0)
        return 0;

    for (slot = 0; slot < REMASTER_EMERALD_PARTY_SIZE; ++slot) {
        RemasterEmeraldPartyPokemon mon;
        if (!remaster_emerald_party_get(save, slot, &mon, 0))
            continue;
        if (remaster_emerald_box_pokemon_species(&mon.box) != 0
            && !remaster_emerald_qol_box_pokemon_is_egg(&mon.box)) {
            if (out_party_slot != 0)
                *out_party_slot = (uint8_t)slot;
            return 1;
        }
    }

    return 0;
}

RemasterEmeraldQolHmFieldUseResult
remaster_emerald_qol_hm_field_use(
    const RemasterEmeraldSave *save,
    uint16_t item_id,
    RemasterEmeraldQolHmTool *out_tool,
    uint8_t *out_party_slot)
{
    RemasterEmeraldQolHmTool tool;
    uint8_t actor;

    if (!remaster_emerald_qol_hm_tool(item_id, &tool))
        return REMASTER_EMERALD_QOL_HM_FIELD_USE_INVALID_ITEM;
    if (!remaster_emerald_qol_hm_access(save, item_id))
        return REMASTER_EMERALD_QOL_HM_FIELD_USE_NO_ACCESS;
    if (!remaster_emerald_qol_hm_field_actor(save, &actor))
        return REMASTER_EMERALD_QOL_HM_FIELD_USE_NO_ACTOR;

    if (out_tool != 0)
        *out_tool = tool;
    if (out_party_slot != 0)
        *out_party_slot = actor;
    return REMASTER_EMERALD_QOL_HM_FIELD_USE_OK;
}

uint16_t remaster_emerald_qol_total_evs(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    uint8_t evs[REMASTER_EMERALD_STAT_COUNT];
    uint16_t total = 0;
    size_t i;

    if (pokemon == 0)
        return 0;

    remaster_emerald_box_pokemon_evs(pokemon, evs);
    for (i = 0; i < REMASTER_EMERALD_STAT_COUNT; ++i)
        total = (uint16_t)(total + evs[i]);
    return total;
}

uint8_t remaster_emerald_qol_hidden_power_type(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    static const uint8_t hidden_power_types[16] = {
        TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND,
        TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL,
        TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC,
        TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON, TYPE_DARK
    };
    uint8_t ivs[REMASTER_EMERALD_STAT_COUNT];
    uint32_t bits;

    (void)TYPE_MYSTERY;

    if (pokemon == 0)
        return 0;

    remaster_emerald_box_pokemon_ivs(pokemon, ivs);
    bits = (ivs[0] & 1u)
        | ((uint32_t)(ivs[1] & 1u) << 1u)
        | ((uint32_t)(ivs[2] & 1u) << 2u)
        | ((uint32_t)(ivs[3] & 1u) << 3u)
        | ((uint32_t)(ivs[4] & 1u) << 4u)
        | ((uint32_t)(ivs[5] & 1u) << 5u);
    return hidden_power_types[(bits * 15u) / 63u];
}

int remaster_emerald_qol_nickname_owned_by_player(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldBoxPokemon *pokemon)
{
    if (save == 0 || pokemon == 0
        || remaster_emerald_qol_box_pokemon_is_egg(pokemon))
        return 0;

    return pokemon->ot_id == read32(
               save->save_block2 + SB2_PLAYER_TRAINER_ID)
        && memcmp(
               pokemon->ot_name,
               save->save_block2 + SB2_PLAYER_NAME,
               sizeof(pokemon->ot_name)) == 0;
}

static uint16_t national_dex_number(uint16_t species)
{
    if (species >= 1 && species <= 251)
        return species;
    if (species >= 277 && species <= 411)
        return (uint16_t)(species - 25u);
    return (uint16_t)(0x8000u + species);
}

static int compare_box_mons(
    const RemasterEmeraldBoxPokemon *a,
    const RemasterEmeraldBoxPokemon *b,
    RemasterEmeraldQolBoxSortMode mode)
{
    const uint16_t species_a = remaster_emerald_box_pokemon_species(a);
    const uint16_t species_b = remaster_emerald_box_pokemon_species(b);
    uint16_t dex_a;
    uint16_t dex_b;

    if (species_a == 0)
        return species_b == 0 ? 0 : 1;
    if (species_b == 0)
        return -1;

    if (remaster_emerald_qol_box_pokemon_is_egg(a)
        != remaster_emerald_qol_box_pokemon_is_egg(b))
        return remaster_emerald_qol_box_pokemon_is_egg(a) ? 1 : -1;
    if (remaster_emerald_qol_box_pokemon_is_egg(a))
        return 0;

    dex_a = national_dex_number(species_a);
    dex_b = national_dex_number(species_b);

    if (mode == REMASTER_EMERALD_QOL_BOX_SORT_LEVEL) {
        const uint8_t level_a = remaster_emerald_level_from_experience(
            species_a, remaster_emerald_box_pokemon_experience(a));
        const uint8_t level_b = remaster_emerald_level_from_experience(
            species_b, remaster_emerald_box_pokemon_experience(b));
        if (level_a != level_b)
            return level_a > level_b ? -1 : 1;
    } else if (mode == REMASTER_EMERALD_QOL_BOX_SORT_TYPE) {
        const RemasterEmeraldSpeciesInfo *info_a =
            remaster_emerald_species_info(species_a);
        const RemasterEmeraldSpeciesInfo *info_b =
            remaster_emerald_species_info(species_b);
        if (info_a == 0 || info_b == 0)
            return dex_a < dex_b ? -1 : dex_a > dex_b;
        if (info_a->type1 != info_b->type1)
            return info_a->type1 < info_b->type1 ? -1 : 1;
        if (info_a->type2 != info_b->type2)
            return info_a->type2 < info_b->type2 ? -1 : 1;
    }

    if (dex_a != dex_b)
        return dex_a < dex_b ? -1 : 1;
    return 0;
}

static uint8_t *box_raw(
    RemasterEmeraldSave *save,
    uint8_t box,
    size_t slot)
{
    return save->pokemon_storage
        + STORAGE_BOXES
        + ((size_t)box * REMASTER_EMERALD_STORAGE_BOX_CAPACITY + slot)
        * REMASTER_EMERALD_BOX_POKEMON_BYTES;
}

static int decode_box_raw(
    const uint8_t raw[REMASTER_EMERALD_BOX_POKEMON_BYTES],
    RemasterEmeraldBoxPokemon *out)
{
    return remaster_emerald_box_pokemon_decode(
        raw,
        REMASTER_EMERALD_BOX_POKEMON_BYTES,
        out,
        0);
}

int remaster_emerald_qol_storage_sort_current_box(
    RemasterEmeraldSave *save,
    RemasterEmeraldQolBoxSortMode mode)
{
    uint8_t box;
    size_t i;

    if (save == 0
        || mode < REMASTER_EMERALD_QOL_BOX_SORT_SPECIES
        || mode > REMASTER_EMERALD_QOL_BOX_SORT_TYPE)
        return 0;

    box = remaster_emerald_storage_current_box(save);
    for (i = 1; i < REMASTER_EMERALD_STORAGE_BOX_CAPACITY; ++i) {
        uint8_t key[REMASTER_EMERALD_BOX_POKEMON_BYTES];
        size_t j = i;
        memcpy(key, box_raw(save, box, i), sizeof(key));

        while (j > 0) {
            RemasterEmeraldBoxPokemon left;
            RemasterEmeraldBoxPokemon right;
            if (!decode_box_raw(box_raw(save, box, j - 1u), &left)
                || !decode_box_raw(key, &right)
                || compare_box_mons(&left, &right, mode) <= 0)
                break;
            memcpy(
                box_raw(save, box, j),
                box_raw(save, box, j - 1u),
                REMASTER_EMERALD_BOX_POKEMON_BYTES);
            --j;
        }
        memcpy(
            box_raw(save, box, j),
            key,
            REMASTER_EMERALD_BOX_POKEMON_BYTES);
    }
    return 1;
}

int remaster_emerald_qol_storage_compact_current_box(
    RemasterEmeraldSave *save)
{
    uint8_t box;
    size_t read_slot;
    size_t write_slot = 0;

    if (save == 0)
        return 0;

    box = remaster_emerald_storage_current_box(save);
    for (read_slot = 0;
         read_slot < REMASTER_EMERALD_STORAGE_BOX_CAPACITY;
         ++read_slot) {
        RemasterEmeraldBoxPokemon mon;
        uint8_t *raw = box_raw(save, box, read_slot);

        if (!decode_box_raw(raw, &mon))
            return 0;
        if (remaster_emerald_box_pokemon_species(&mon) == 0)
            continue;

        if (write_slot != read_slot) {
            memcpy(
                box_raw(save, box, write_slot),
                raw,
                REMASTER_EMERALD_BOX_POKEMON_BYTES);
            memset(raw, 0, REMASTER_EMERALD_BOX_POKEMON_BYTES);
        }
        ++write_slot;
    }

    while (write_slot < REMASTER_EMERALD_STORAGE_BOX_CAPACITY) {
        memset(
            box_raw(save, box, write_slot),
            0,
            REMASTER_EMERALD_BOX_POKEMON_BYTES);
        ++write_slot;
    }
    return 1;
}

static int first_free_box_slot(
    RemasterEmeraldSave *save,
    uint8_t box,
    size_t *out_slot)
{
    size_t slot;

    for (slot = 0; slot < REMASTER_EMERALD_STORAGE_BOX_CAPACITY; ++slot) {
        RemasterEmeraldBoxPokemon mon;
        if (!remaster_emerald_storage_get(save, box, slot, &mon, 0))
            return 0;
        if (remaster_emerald_box_pokemon_species(&mon) == 0) {
            if (out_slot != 0)
                *out_slot = slot;
            return 1;
        }
    }
    return 0;
}

static size_t usable_party_mons_except(
    const RemasterEmeraldSave *save,
    uint8_t excluded)
{
    const uint8_t count = remaster_emerald_party_count(save);
    size_t usable = 0;
    size_t slot;

    for (slot = 0; slot < count; ++slot) {
        RemasterEmeraldPartyPokemon mon;
        if (slot == excluded)
            continue;
        if (!remaster_emerald_party_get(save, slot, &mon, 0))
            continue;
        if (remaster_emerald_box_pokemon_species(&mon.box) != 0
            && !remaster_emerald_qol_box_pokemon_is_egg(&mon.box)
            && mon.hp != 0)
            ++usable;
    }
    return usable;
}

RemasterEmeraldQolTransferResult
remaster_emerald_qol_quick_deposit_current_box(
    RemasterEmeraldSave *save,
    uint8_t party_slot)
{
    RemasterEmeraldPartyPokemon selected;
    uint8_t count;
    uint8_t box;
    size_t target;
    size_t slot;

    if (save == 0)
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    count = remaster_emerald_party_count(save);
    if (party_slot >= count
        || !remaster_emerald_party_get(save, party_slot, &selected, 0)
        || remaster_emerald_box_pokemon_species(&selected.box) == 0)
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    if (usable_party_mons_except(save, party_slot) == 0)
        return REMASTER_EMERALD_QOL_TRANSFER_LAST_USABLE;

    if (item_is_mail(remaster_emerald_box_pokemon_held_item(&selected.box)))
        return REMASTER_EMERALD_QOL_TRANSFER_MAIL;

    box = remaster_emerald_storage_current_box(save);
    if (!first_free_box_slot(save, box, &target))
        return REMASTER_EMERALD_QOL_TRANSFER_BOX_FULL;

    if (!remaster_emerald_storage_set(save, box, target, &selected.box))
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    for (slot = party_slot; slot + 1u < count; ++slot) {
        RemasterEmeraldPartyPokemon next;
        if (!remaster_emerald_party_get(save, slot + 1u, &next, 0)
            || !remaster_emerald_party_set(save, slot, &next))
            return REMASTER_EMERALD_QOL_TRANSFER_INVALID;
    }

    if (count != 0) {
        RemasterEmeraldPartyPokemon empty;
        memset(&empty, 0, sizeof(empty));
        if (!remaster_emerald_party_set(save, count - 1u, &empty)
            || !remaster_emerald_party_set_count(save, (uint8_t)(count - 1u)))
            return REMASTER_EMERALD_QOL_TRANSFER_INVALID;
    }

    return REMASTER_EMERALD_QOL_TRANSFER_OK;
}

RemasterEmeraldQolTransferResult
remaster_emerald_qol_quick_withdraw_current_box(
    RemasterEmeraldSave *save,
    uint8_t box_slot)
{
    RemasterEmeraldBoxPokemon boxed;
    RemasterEmeraldPartyPokemon party;
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[REMASTER_EMERALD_STAT_COUNT];
    uint8_t evs[REMASTER_EMERALD_STAT_COUNT];
    uint8_t count;
    uint8_t box;
    uint16_t species;

    if (save == 0
        || box_slot >= REMASTER_EMERALD_STORAGE_BOX_CAPACITY)
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    count = remaster_emerald_party_count(save);
    if (count >= REMASTER_EMERALD_PARTY_SIZE)
        return REMASTER_EMERALD_QOL_TRANSFER_PARTY_FULL;

    box = remaster_emerald_storage_current_box(save);
    if (!remaster_emerald_storage_get(save, box, box_slot, &boxed, 0))
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    species = remaster_emerald_box_pokemon_species(&boxed);
    if (species == 0)
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    memset(&party, 0, sizeof(party));
    party.box = boxed;
    party.level = remaster_emerald_level_from_experience(
        species,
        remaster_emerald_box_pokemon_experience(&boxed));
    party.mail = 0xFFu;
    remaster_emerald_box_pokemon_ivs(&boxed, ivs);
    remaster_emerald_box_pokemon_evs(&boxed, evs);
    if (!remaster_emerald_calculate_stats(
            species,
            party.level,
            remaster_emerald_box_pokemon_nature(&boxed),
            ivs,
            evs,
            &stats))
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    party.max_hp = stats.hp;
    party.hp = stats.hp;
    party.attack = stats.attack;
    party.defense = stats.defense;
    party.speed = stats.speed;
    party.sp_attack = stats.sp_attack;
    party.sp_defense = stats.sp_defense;

    if (!remaster_emerald_party_set(save, count, &party)
        || !remaster_emerald_party_set_count(save, (uint8_t)(count + 1u)))
        return REMASTER_EMERALD_QOL_TRANSFER_INVALID;

    memset(
        box_raw(save, box, box_slot),
        0,
        REMASTER_EMERALD_BOX_POKEMON_BYTES);
    return REMASTER_EMERALD_QOL_TRANSFER_OK;
}

int remaster_emerald_qol_swap_held_items(
    RemasterEmeraldBoxPokemon *a,
    RemasterEmeraldBoxPokemon *b)
{
    uint16_t item_a;
    uint16_t item_b;

    if (a == 0 || b == 0)
        return 0;

    item_a = remaster_emerald_box_pokemon_held_item(a);
    item_b = remaster_emerald_box_pokemon_held_item(b);
    if (item_is_mail(item_a) || item_is_mail(item_b))
        return 0;

    return remaster_emerald_box_pokemon_set_held_item(a, item_b)
        && remaster_emerald_box_pokemon_set_held_item(b, item_a);
}

RemasterEmeraldQolHeldItemResult remaster_emerald_qol_pc_give_held_item(
    RemasterEmeraldSave *save,
    uint8_t box_slot,
    uint16_t item_id)
{
    RemasterEmeraldBoxPokemon mon;
    RemasterEmeraldBoxPokemon changed;
    const RemasterEmeraldItemInfo *info;
    uint8_t box;

    if (save == 0
        || box_slot >= REMASTER_EMERALD_STORAGE_BOX_CAPACITY
        || item_id == ITEM_NONE)
        return REMASTER_EMERALD_QOL_HELD_ITEM_INVALID;

    box = remaster_emerald_storage_current_box(save);
    if (!remaster_emerald_storage_get(save, box, box_slot, &mon, 0)
        || remaster_emerald_box_pokemon_species(&mon) == 0)
        return REMASTER_EMERALD_QOL_HELD_ITEM_INVALID;
    if (remaster_emerald_qol_box_pokemon_is_egg(&mon))
        return REMASTER_EMERALD_QOL_HELD_ITEM_EGG;
    if (remaster_emerald_box_pokemon_held_item(&mon) != ITEM_NONE)
        return REMASTER_EMERALD_QOL_HELD_ITEM_ALREADY_HELD;
    if (item_is_mail(item_id))
        return REMASTER_EMERALD_QOL_HELD_ITEM_MAIL;

    info = remaster_emerald_item_info(item_id);
    if (info == 0
        || info->pocket == REMASTER_EMERALD_POCKET_NONE)
        return REMASTER_EMERALD_QOL_HELD_ITEM_INVALID;
    if (info->pocket == REMASTER_EMERALD_POCKET_KEY_ITEMS
        || info->importance != 0)
        return REMASTER_EMERALD_QOL_HELD_ITEM_UNHOLDABLE;
    if (remaster_emerald_bag_count(save, item_id) == 0)
        return REMASTER_EMERALD_QOL_HELD_ITEM_ITEM_MISSING;

    changed = mon;
    if (!remaster_emerald_box_pokemon_set_held_item(&changed, item_id)
        || !remaster_emerald_bag_remove(save, item_id, 1))
        return REMASTER_EMERALD_QOL_HELD_ITEM_ITEM_MISSING;

    if (!remaster_emerald_storage_set(save, box, box_slot, &changed)) {
        (void)remaster_emerald_bag_add(save, item_id, 1);
        return REMASTER_EMERALD_QOL_HELD_ITEM_INVALID;
    }

    return REMASTER_EMERALD_QOL_HELD_ITEM_OK;
}

RemasterEmeraldQolHeldItemResult remaster_emerald_qol_pc_take_held_item(
    RemasterEmeraldSave *save,
    uint8_t box_slot,
    uint16_t *out_item_id)
{
    RemasterEmeraldBoxPokemon mon;
    RemasterEmeraldBoxPokemon changed;
    uint8_t box;
    uint16_t item_id;

    if (save == 0 || box_slot >= REMASTER_EMERALD_STORAGE_BOX_CAPACITY)
        return REMASTER_EMERALD_QOL_HELD_ITEM_INVALID;

    box = remaster_emerald_storage_current_box(save);
    if (!remaster_emerald_storage_get(save, box, box_slot, &mon, 0)
        || remaster_emerald_box_pokemon_species(&mon) == 0)
        return REMASTER_EMERALD_QOL_HELD_ITEM_INVALID;
    if (remaster_emerald_qol_box_pokemon_is_egg(&mon))
        return REMASTER_EMERALD_QOL_HELD_ITEM_EGG;

    item_id = remaster_emerald_box_pokemon_held_item(&mon);
    if (item_id == ITEM_NONE)
        return REMASTER_EMERALD_QOL_HELD_ITEM_NOT_HOLDING;
    if (item_is_mail(item_id))
        return REMASTER_EMERALD_QOL_HELD_ITEM_MAIL;
    if (!remaster_emerald_bag_has_space(save, item_id, 1))
        return REMASTER_EMERALD_QOL_HELD_ITEM_BAG_FULL;

    changed = mon;
    if (!remaster_emerald_box_pokemon_set_held_item(&changed, ITEM_NONE)
        || !remaster_emerald_bag_add(save, item_id, 1))
        return REMASTER_EMERALD_QOL_HELD_ITEM_BAG_FULL;

    if (!remaster_emerald_storage_set(save, box, box_slot, &changed)) {
        (void)remaster_emerald_bag_remove(save, item_id, 1);
        return REMASTER_EMERALD_QOL_HELD_ITEM_INVALID;
    }

    if (out_item_id != 0)
        *out_item_id = item_id;
    return REMASTER_EMERALD_QOL_HELD_ITEM_OK;
}

static uint8_t *item_meta(RemasterEmeraldSave *save)
{
    return save->save_block1 + SB1_VANILLAPLUS_ITEM_META;
}

static void quick_items_sync_primary(RemasterEmeraldSave *save)
{
    uint8_t *meta = item_meta(save);
    const uint8_t count = meta[VANILLAPLUS_ITEM_META_QUICK_COUNT];
    write16(
        save->save_block1 + SB1_REGISTERED_ITEM,
        count == 0 ? (uint16_t)ITEM_NONE
                   : read16(meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS));
}

static int item_meta_valid(const uint8_t *meta)
{
    size_t i;

    if (read32(meta) != VANILLAPLUS_ITEM_META_MAGIC
        || meta[4] != VANILLAPLUS_ITEM_META_VERSION
        || meta[VANILLAPLUS_ITEM_META_QUICK_COUNT] > VANILLAPLUS_QUICK_ITEM_MAX)
        return 0;

    for (i = 0; i < VANILLAPLUS_ITEM_META_BAG_SORT_COUNT; ++i) {
        if (meta[VANILLAPLUS_ITEM_META_BAG_SORT_MODES + i]
            >= REMASTER_EMERALD_QOL_ITEM_SORT_COUNT)
            return 0;
    }
    return 1;
}

static uint8_t *ensure_item_meta(RemasterEmeraldSave *save)
{
    uint8_t *meta;
    uint16_t registered;

    if (save == 0)
        return 0;

    meta = item_meta(save);
    if (item_meta_valid(meta))
        return meta;

    registered = read16(save->save_block1 + SB1_REGISTERED_ITEM);
    memset(meta, 0, VANILLAPLUS_ITEM_META_BYTES);
    write32(meta, VANILLAPLUS_ITEM_META_MAGIC);
    meta[4] = VANILLAPLUS_ITEM_META_VERSION;
    if (registered != ITEM_NONE) {
        meta[VANILLAPLUS_ITEM_META_QUICK_COUNT] = 1;
        write16(meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS, registered);
    }
    return meta;
}

int remaster_emerald_qol_bag_sort(
    RemasterEmeraldSave *save,
    uint8_t pocket,
    RemasterEmeraldQolItemSortMode mode)
{
    RemasterEmeraldQolItemView view;
    uint8_t *meta;
    size_t count;
    const int index = bag_metadata_index(pocket);

    if (save == 0
        || index < 0
        || mode <= REMASTER_EMERALD_QOL_ITEM_SORT_NONE
        || mode >= REMASTER_EMERALD_QOL_ITEM_SORT_COUNT
        || pocket == REMASTER_EMERALD_POCKET_TM_HM
        || pocket == REMASTER_EMERALD_POCKET_BERRIES
        || !bag_view(pocket, &view))
        return 0;

    count = compact_item_slots(save, &view);
    sort_item_slots(save, &view, count, mode);
    meta = ensure_item_meta(save);
    if (meta == 0)
        return 0;
    meta[VANILLAPLUS_ITEM_META_BAG_SORT_MODES + (size_t)index] = (uint8_t)mode;
    return 1;
}

RemasterEmeraldQolItemSortMode remaster_emerald_qol_bag_sort_mode(
    RemasterEmeraldSave *save,
    uint8_t pocket)
{
    uint8_t *meta;
    const int index = bag_metadata_index(pocket);

    if (save == 0 || index < 0)
        return REMASTER_EMERALD_QOL_ITEM_SORT_NONE;
    meta = ensure_item_meta(save);
    if (meta == 0)
        return REMASTER_EMERALD_QOL_ITEM_SORT_NONE;
    return (RemasterEmeraldQolItemSortMode)
        meta[VANILLAPLUS_ITEM_META_BAG_SORT_MODES + (size_t)index];
}

int remaster_emerald_qol_bag_set_sort_mode(
    RemasterEmeraldSave *save,
    uint8_t pocket,
    RemasterEmeraldQolItemSortMode mode)
{
    uint8_t *meta;
    const int index = bag_metadata_index(pocket);

    if (save == 0
        || index < 0
        || mode < REMASTER_EMERALD_QOL_ITEM_SORT_NONE
        || mode >= REMASTER_EMERALD_QOL_ITEM_SORT_COUNT)
        return 0;

    meta = ensure_item_meta(save);
    if (meta == 0)
        return 0;
    meta[VANILLAPLUS_ITEM_META_BAG_SORT_MODES + (size_t)index] = (uint8_t)mode;
    return 1;
}

int remaster_emerald_qol_bag_auto_sort_enabled(
    RemasterEmeraldSave *save,
    uint8_t pocket)
{
    uint8_t *meta;
    const int index = bag_metadata_index(pocket);

    if (save == 0 || index < 0)
        return 0;
    meta = ensure_item_meta(save);
    return meta != 0
        && (meta[VANILLAPLUS_ITEM_META_AUTO_SORT_MASK] & (1u << index)) != 0;
}

int remaster_emerald_qol_bag_set_auto_sort_enabled(
    RemasterEmeraldSave *save,
    uint8_t pocket,
    int enabled)
{
    uint8_t *meta;
    const int index = bag_metadata_index(pocket);

    if (save == 0
        || index < 0
        || pocket == REMASTER_EMERALD_POCKET_TM_HM
        || pocket == REMASTER_EMERALD_POCKET_BERRIES)
        return 0;

    meta = ensure_item_meta(save);
    if (meta == 0)
        return 0;
    if (enabled)
        meta[VANILLAPLUS_ITEM_META_AUTO_SORT_MASK] |= (uint8_t)(1u << index);
    else
        meta[VANILLAPLUS_ITEM_META_AUTO_SORT_MASK] &= (uint8_t)~(1u << index);
    return 1;
}

int remaster_emerald_qol_bag_auto_sort(
    RemasterEmeraldSave *save,
    uint8_t pocket)
{
    const RemasterEmeraldQolItemSortMode mode =
        remaster_emerald_qol_bag_sort_mode(save, pocket);

    if (save == 0 || bag_metadata_index(pocket) < 0)
        return 0;
    if (!remaster_emerald_qol_bag_auto_sort_enabled(save, pocket)
        || mode == REMASTER_EMERALD_QOL_ITEM_SORT_NONE)
        return 1;
    return remaster_emerald_qol_bag_sort(save, pocket, mode);
}

int remaster_emerald_qol_pc_items_sort(
    RemasterEmeraldSave *save,
    RemasterEmeraldQolItemSortMode mode)
{
    RemasterEmeraldQolItemView view;
    size_t count;

    if (save == 0
        || mode <= REMASTER_EMERALD_QOL_ITEM_SORT_NONE
        || mode >= REMASTER_EMERALD_QOL_ITEM_SORT_COUNT)
        return 0;

    view.offset = SB1_PC_ITEMS;
    view.capacity = PC_ITEMS_COUNT;
    view.encrypted_quantity = 0;
    count = compact_item_slots(save, &view);
    sort_item_slots(save, &view, count, mode);
    return 1;
}

uint8_t remaster_emerald_qol_quick_item_count(RemasterEmeraldSave *save)
{
    uint8_t *meta = ensure_item_meta(save);
    return meta == 0 ? 0 : meta[VANILLAPLUS_ITEM_META_QUICK_COUNT];
}

uint16_t remaster_emerald_qol_quick_item_get(
    RemasterEmeraldSave *save,
    uint8_t slot)
{
    uint8_t *meta = ensure_item_meta(save);
    if (meta == 0 || slot >= meta[VANILLAPLUS_ITEM_META_QUICK_COUNT])
        return ITEM_NONE;
    return read16(
        meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS + (size_t)slot * 2u);
}

int remaster_emerald_qol_quick_item_register(
    RemasterEmeraldSave *save,
    uint16_t item_id)
{
    uint8_t *meta;
    uint8_t count;
    uint8_t i;

    if (save == 0 || item_id == ITEM_NONE)
        return 0;

    meta = ensure_item_meta(save);
    count = meta[VANILLAPLUS_ITEM_META_QUICK_COUNT];
    for (i = 0; i < count; ++i) {
        if (read16(
                meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS + (size_t)i * 2u)
            == item_id)
            return 1;
    }
    if (count >= VANILLAPLUS_QUICK_ITEM_MAX)
        return 0;

    write16(
        meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS + (size_t)count * 2u,
        item_id);
    meta[VANILLAPLUS_ITEM_META_QUICK_COUNT] = (uint8_t)(count + 1u);
    quick_items_sync_primary(save);
    return 1;
}

int remaster_emerald_qol_quick_item_unregister(
    RemasterEmeraldSave *save,
    uint16_t item_id)
{
    uint8_t *meta;
    uint8_t count;
    uint8_t i;

    if (save == 0)
        return 0;

    meta = ensure_item_meta(save);
    count = meta[VANILLAPLUS_ITEM_META_QUICK_COUNT];
    for (i = 0; i < count; ++i) {
        if (read16(
                meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS + (size_t)i * 2u)
            == item_id) {
            uint8_t j;
            for (j = i; j + 1u < count; ++j) {
                write16(
                    meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS
                        + (size_t)j * 2u,
                    read16(
                        meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS
                            + (size_t)(j + 1u) * 2u));
            }
            --count;
            meta[VANILLAPLUS_ITEM_META_QUICK_COUNT] = count;
            write16(
                meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS
                    + (size_t)count * 2u,
                ITEM_NONE);
            if (meta[VANILLAPLUS_ITEM_META_LAST_QUICK] >= count)
                meta[VANILLAPLUS_ITEM_META_LAST_QUICK] = 0;
            quick_items_sync_primary(save);
            return 1;
        }
    }
    return 0;
}

void remaster_emerald_qol_quick_items_prune(RemasterEmeraldSave *save)
{
    uint8_t *meta;
    uint16_t kept[VANILLAPLUS_QUICK_ITEM_MAX] = {0,0,0,0};
    uint8_t kept_count = 0;
    uint8_t count;
    uint8_t i;

    if (save == 0)
        return;

    meta = ensure_item_meta(save);
    count = meta[VANILLAPLUS_ITEM_META_QUICK_COUNT];
    for (i = 0; i < count; ++i) {
        const uint16_t item_id = read16(
            meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS + (size_t)i * 2u);
        if (item_id != ITEM_NONE
            && remaster_emerald_bag_count(save, item_id) != 0)
            kept[kept_count++] = item_id;
    }

    for (i = 0; i < VANILLAPLUS_QUICK_ITEM_MAX; ++i) {
        write16(
            meta + VANILLAPLUS_ITEM_META_QUICK_ITEMS + (size_t)i * 2u,
            kept[i]);
    }
    meta[VANILLAPLUS_ITEM_META_QUICK_COUNT] = kept_count;
    if (meta[VANILLAPLUS_ITEM_META_LAST_QUICK] >= kept_count)
        meta[VANILLAPLUS_ITEM_META_LAST_QUICK] = 0;
    quick_items_sync_primary(save);
}
