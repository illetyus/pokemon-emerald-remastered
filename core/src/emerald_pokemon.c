#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_items.h"

#include <string.h>

enum {
    BOX_PERSONALITY = 0,
    BOX_OT_ID = 4,
    BOX_NICKNAME = 8,
    BOX_LANGUAGE = 18,
    BOX_FLAGS = 19,
    BOX_OT_NAME = 20,
    BOX_MARKINGS = 27,
    BOX_CHECKSUM = 28,
    BOX_UNKNOWN = 30,
    BOX_SECURE = 32,

    SB1_PARTY_COUNT = 0x0234,
    SB1_PARTY = 0x0238,

    STORAGE_CURRENT_BOX = 0x0000,
    STORAGE_BOXES = 0x0004,

    PARTY_STATUS = 80,
    PARTY_LEVEL = 84,
    PARTY_MAIL = 85,
    PARTY_HP = 86,
    PARTY_MAX_HP = 88,
    PARTY_ATTACK = 90,
    PARTY_DEFENSE = 92,
    PARTY_SPEED = 94,
    PARTY_SP_ATTACK = 96,
    PARTY_SP_DEFENSE = 98,

    SPECIES_SHEDINJA = 303
};

/*
 * Gen III GetSubstruct mapping. Each row is personality % 24 and each
 * column is canonical substruct type 0..3. Values are physical slots.
 */
static const uint8_t kSubstructOrder[24][4] = {
    {0,1,2,3}, {0,1,3,2}, {0,2,1,3}, {0,3,1,2},
    {0,2,3,1}, {0,3,2,1}, {1,0,2,3}, {1,0,3,2},
    {2,0,1,3}, {3,0,1,2}, {2,0,3,1}, {3,0,2,1},
    {1,2,0,3}, {1,3,0,2}, {2,1,0,3}, {3,1,0,2},
    {2,3,0,1}, {3,2,0,1}, {1,2,3,0}, {1,3,2,0},
    {2,1,3,0}, {3,1,2,0}, {2,3,1,0}, {3,2,1,0}
};

#include "emerald_domain_catalog.inc"

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

static uint16_t sub_read16(
    const RemasterEmeraldBoxPokemon *pokemon,
    size_t type,
    size_t offset)
{
    if (pokemon == 0 || type >= 4 || offset + 2 > 12)
        return 0;
    return read16(&pokemon->substruct[type][offset]);
}

static uint32_t sub_read32(
    const RemasterEmeraldBoxPokemon *pokemon,
    size_t type,
    size_t offset)
{
    if (pokemon == 0 || type >= 4 || offset + 4 > 12)
        return 0;
    return read32(&pokemon->substruct[type][offset]);
}

static void sub_write16(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t type,
    size_t offset,
    uint16_t value)
{
    if (pokemon == 0 || type >= 4 || offset + 2 > 12)
        return;
    write16(&pokemon->substruct[type][offset], value);
}

static void sub_write32(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t type,
    size_t offset,
    uint32_t value)
{
    if (pokemon == 0 || type >= 4 || offset + 4 > 12)
        return;
    write32(&pokemon->substruct[type][offset], value);
}

uint16_t remaster_emerald_box_pokemon_checksum(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    uint32_t sum = 0;
    size_t type;
    size_t offset;

    if (pokemon == 0)
        return 0;

    for (type = 0; type < 4; ++type) {
        for (offset = 0; offset < 12; offset += 2)
            sum += read16(&pokemon->substruct[type][offset]);
    }

    return (uint16_t)sum;
}

int remaster_emerald_box_pokemon_decode(
    const uint8_t *raw,
    size_t raw_size,
    RemasterEmeraldBoxPokemon *out,
    int *out_checksum_valid)
{
    uint8_t physical[48];
    uint32_t key;
    size_t offset;
    size_t type;
    const uint8_t *order;

    if (raw == 0 || out == 0
        || raw_size < REMASTER_EMERALD_BOX_POKEMON_BYTES)
        return 0;

    memset(out, 0, sizeof(*out));

    out->personality = read32(raw + BOX_PERSONALITY);
    out->ot_id = read32(raw + BOX_OT_ID);
    memcpy(out->nickname, raw + BOX_NICKNAME, sizeof(out->nickname));
    out->language = raw[BOX_LANGUAGE];
    out->header_flags = raw[BOX_FLAGS];
    memcpy(out->ot_name, raw + BOX_OT_NAME, sizeof(out->ot_name));
    out->markings = raw[BOX_MARKINGS];
    out->checksum = read16(raw + BOX_CHECKSUM);
    out->unknown = read16(raw + BOX_UNKNOWN);

    key = out->personality ^ out->ot_id;
    for (offset = 0; offset < sizeof(physical); offset += 4) {
        const uint32_t word = read32(raw + BOX_SECURE + offset) ^ key;
        write32(physical + offset, word);
    }

    order = kSubstructOrder[out->personality % 24u];
    for (type = 0; type < 4; ++type) {
        memcpy(
            out->substruct[type],
            physical + (size_t)order[type] * 12u,
            12u);
    }

    if (out_checksum_valid != 0) {
        *out_checksum_valid =
            remaster_emerald_box_pokemon_checksum(out) == out->checksum;
    }

    return 1;
}

int remaster_emerald_box_pokemon_encode(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint8_t *out_raw,
    size_t out_size)
{
    uint8_t physical[48];
    uint32_t key;
    uint16_t checksum;
    size_t offset;
    size_t type;
    const uint8_t *order;

    if (pokemon == 0 || out_raw == 0
        || out_size < REMASTER_EMERALD_BOX_POKEMON_BYTES)
        return 0;

    memset(out_raw, 0, REMASTER_EMERALD_BOX_POKEMON_BYTES);
    memset(physical, 0, sizeof(physical));

    write32(out_raw + BOX_PERSONALITY, pokemon->personality);
    write32(out_raw + BOX_OT_ID, pokemon->ot_id);
    memcpy(out_raw + BOX_NICKNAME, pokemon->nickname, sizeof(pokemon->nickname));
    out_raw[BOX_LANGUAGE] = pokemon->language;
    out_raw[BOX_FLAGS] = pokemon->header_flags;
    memcpy(out_raw + BOX_OT_NAME, pokemon->ot_name, sizeof(pokemon->ot_name));
    out_raw[BOX_MARKINGS] = pokemon->markings;

    checksum = remaster_emerald_box_pokemon_checksum(pokemon);
    write16(out_raw + BOX_CHECKSUM, checksum);
    write16(out_raw + BOX_UNKNOWN, pokemon->unknown);

    order = kSubstructOrder[pokemon->personality % 24u];
    for (type = 0; type < 4; ++type) {
        memcpy(
            physical + (size_t)order[type] * 12u,
            pokemon->substruct[type],
            12u);
    }

    key = pokemon->personality ^ pokemon->ot_id;
    for (offset = 0; offset < sizeof(physical); offset += 4) {
        const uint32_t word = read32(physical + offset) ^ key;
        write32(out_raw + BOX_SECURE + offset, word);
    }

    return 1;
}

uint16_t remaster_emerald_box_pokemon_species(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return sub_read16(pokemon, 0, 0);
}

uint16_t remaster_emerald_box_pokemon_held_item(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return sub_read16(pokemon, 0, 2);
}

uint32_t remaster_emerald_box_pokemon_experience(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return sub_read32(pokemon, 0, 4);
}

uint8_t remaster_emerald_box_pokemon_friendship(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return pokemon == 0 ? 0 : pokemon->substruct[0][9];
}

uint8_t remaster_emerald_box_pokemon_nature(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return pokemon == 0 ? 0 : (uint8_t)(pokemon->personality % 25u);
}

uint8_t remaster_emerald_box_pokemon_ability_num(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    const uint32_t iv_word = sub_read32(pokemon, 3, 4);
    return (uint8_t)((iv_word >> 31u) & 1u);
}

uint8_t remaster_emerald_box_pokemon_met_level(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return (uint8_t)(sub_read16(pokemon, 3, 2) & 0x7Fu);
}

uint8_t remaster_emerald_box_pokemon_met_game(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return (uint8_t)((sub_read16(pokemon, 3, 2) >> 7u) & 0x0Fu);
}

uint8_t remaster_emerald_box_pokemon_pokeball(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return (uint8_t)((sub_read16(pokemon, 3, 2) >> 11u) & 0x0Fu);
}

uint8_t remaster_emerald_box_pokemon_ot_gender(
    const RemasterEmeraldBoxPokemon *pokemon)
{
    return (uint8_t)((sub_read16(pokemon, 3, 2) >> 15u) & 1u);
}

void remaster_emerald_box_pokemon_moves(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint16_t out_moves[4],
    uint8_t out_pp[4])
{
    size_t i;

    if (pokemon == 0)
        return;

    for (i = 0; i < 4; ++i) {
        if (out_moves != 0)
            out_moves[i] = sub_read16(pokemon, 1, i * 2u);
        if (out_pp != 0)
            out_pp[i] = pokemon->substruct[1][8u + i];
    }
}

void remaster_emerald_box_pokemon_evs(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint8_t out_evs[6])
{
    if (pokemon != 0 && out_evs != 0)
        memcpy(out_evs, pokemon->substruct[2], 6u);
}

void remaster_emerald_box_pokemon_ivs(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint8_t out_ivs[6])
{
    uint32_t word;
    size_t i;

    if (pokemon == 0 || out_ivs == 0)
        return;

    word = sub_read32(pokemon, 3, 4);
    for (i = 0; i < 6; ++i)
        out_ivs[i] = (uint8_t)((word >> (i * 5u)) & 31u);
}

int remaster_emerald_box_pokemon_set_species(
    RemasterEmeraldBoxPokemon *pokemon,
    uint16_t species)
{
    if (pokemon == 0)
        return 0;
    sub_write16(pokemon, 0, 0, species);
    if (species != 0)
        pokemon->header_flags |= 0x02u;
    else
        pokemon->header_flags &= (uint8_t)~0x02u;
    return 1;
}

int remaster_emerald_box_pokemon_set_held_item(
    RemasterEmeraldBoxPokemon *pokemon,
    uint16_t item_id)
{
    if (pokemon == 0)
        return 0;
    sub_write16(pokemon, 0, 2, item_id);
    return 1;
}

int remaster_emerald_box_pokemon_set_experience(
    RemasterEmeraldBoxPokemon *pokemon,
    uint32_t experience)
{
    if (pokemon == 0)
        return 0;
    sub_write32(pokemon, 0, 4, experience);
    return 1;
}

int remaster_emerald_box_pokemon_set_friendship(
    RemasterEmeraldBoxPokemon *pokemon,
    uint8_t friendship)
{
    if (pokemon == 0)
        return 0;
    pokemon->substruct[0][9] = friendship;
    return 1;
}

int remaster_emerald_box_pokemon_set_move(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t slot,
    uint16_t move_id,
    uint8_t pp)
{
    if (pokemon == 0 || slot >= 4)
        return 0;
    sub_write16(pokemon, 1, slot * 2u, move_id);
    pokemon->substruct[1][8u + slot] = pp;
    return 1;
}

int remaster_emerald_box_pokemon_set_ev(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t stat,
    uint8_t value)
{
    if (pokemon == 0 || stat >= 6)
        return 0;
    pokemon->substruct[2][stat] = value;
    return 1;
}

int remaster_emerald_box_pokemon_set_iv(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t stat,
    uint8_t value)
{
    uint32_t word;
    uint32_t mask;

    if (pokemon == 0 || stat >= 6 || value > 31)
        return 0;

    word = sub_read32(pokemon, 3, 4);
    mask = UINT32_C(31) << (stat * 5u);
    word = (word & ~mask) | ((uint32_t)value << (stat * 5u));
    sub_write32(pokemon, 3, 4, word);
    return 1;
}

int remaster_emerald_box_pokemon_set_ability_num(
    RemasterEmeraldBoxPokemon *pokemon,
    uint8_t ability_num)
{
    uint32_t word;

    if (pokemon == 0 || ability_num > 1)
        return 0;

    word = sub_read32(pokemon, 3, 4);
    word &= UINT32_C(0x7FFFFFFF);
    word |= (uint32_t)ability_num << 31u;
    sub_write32(pokemon, 3, 4, word);
    return 1;
}

int remaster_emerald_box_pokemon_set_pokeball(
    RemasterEmeraldBoxPokemon *pokemon,
    uint8_t pokeball)
{
    uint16_t origins;

    if (pokemon == 0 || pokeball > 15)
        return 0;

    origins = sub_read16(pokemon, 3, 2);
    origins &= (uint16_t)~UINT16_C(0x7800);
    origins |= (uint16_t)((uint16_t)pokeball << 11u);
    sub_write16(pokemon, 3, 2, origins);
    return 1;
}

static int decode_party_raw(
    const uint8_t *raw,
    RemasterEmeraldPartyPokemon *out,
    int *out_checksum_valid)
{
    if (!remaster_emerald_box_pokemon_decode(
            raw,
            REMASTER_EMERALD_BOX_POKEMON_BYTES,
            &out->box,
            out_checksum_valid))
        return 0;

    out->status = read32(raw + PARTY_STATUS);
    out->level = raw[PARTY_LEVEL];
    out->mail = raw[PARTY_MAIL];
    out->hp = read16(raw + PARTY_HP);
    out->max_hp = read16(raw + PARTY_MAX_HP);
    out->attack = read16(raw + PARTY_ATTACK);
    out->defense = read16(raw + PARTY_DEFENSE);
    out->speed = read16(raw + PARTY_SPEED);
    out->sp_attack = read16(raw + PARTY_SP_ATTACK);
    out->sp_defense = read16(raw + PARTY_SP_DEFENSE);
    return 1;
}

static int encode_party_raw(
    const RemasterEmeraldPartyPokemon *pokemon,
    uint8_t *raw)
{
    if (!remaster_emerald_box_pokemon_encode(
            &pokemon->box,
            raw,
            REMASTER_EMERALD_BOX_POKEMON_BYTES))
        return 0;

    write32(raw + PARTY_STATUS, pokemon->status);
    raw[PARTY_LEVEL] = pokemon->level;
    raw[PARTY_MAIL] = pokemon->mail;
    write16(raw + PARTY_HP, pokemon->hp);
    write16(raw + PARTY_MAX_HP, pokemon->max_hp);
    write16(raw + PARTY_ATTACK, pokemon->attack);
    write16(raw + PARTY_DEFENSE, pokemon->defense);
    write16(raw + PARTY_SPEED, pokemon->speed);
    write16(raw + PARTY_SP_ATTACK, pokemon->sp_attack);
    write16(raw + PARTY_SP_DEFENSE, pokemon->sp_defense);
    return 1;
}

uint8_t remaster_emerald_party_count(const RemasterEmeraldSave *save)
{
    uint32_t count;

    if (save == 0)
        return 0;

    count = read32(save->save_block1 + SB1_PARTY_COUNT);
    return count <= REMASTER_EMERALD_PARTY_SIZE
        ? (uint8_t)count
        : 0;
}

int remaster_emerald_party_set_count(
    RemasterEmeraldSave *save,
    uint8_t count)
{
    if (save == 0 || count > REMASTER_EMERALD_PARTY_SIZE)
        return 0;

    write32(save->save_block1 + SB1_PARTY_COUNT, count);
    return 1;
}

int remaster_emerald_party_get(
    const RemasterEmeraldSave *save,
    size_t slot,
    RemasterEmeraldPartyPokemon *out,
    int *out_checksum_valid)
{
    const size_t offset =
        SB1_PARTY + slot * REMASTER_EMERALD_PARTY_POKEMON_BYTES;

    if (save == 0 || out == 0 || slot >= REMASTER_EMERALD_PARTY_SIZE)
        return 0;

    return decode_party_raw(
        save->save_block1 + offset,
        out,
        out_checksum_valid);
}

int remaster_emerald_party_set(
    RemasterEmeraldSave *save,
    size_t slot,
    const RemasterEmeraldPartyPokemon *pokemon)
{
    const size_t offset =
        SB1_PARTY + slot * REMASTER_EMERALD_PARTY_POKEMON_BYTES;

    if (save == 0 || pokemon == 0 || slot >= REMASTER_EMERALD_PARTY_SIZE)
        return 0;

    return encode_party_raw(pokemon, save->save_block1 + offset);
}

static size_t storage_offset(size_t box, size_t slot)
{
    return STORAGE_BOXES
        + (box * REMASTER_EMERALD_STORAGE_BOX_CAPACITY + slot)
        * REMASTER_EMERALD_BOX_POKEMON_BYTES;
}

uint8_t remaster_emerald_storage_current_box(
    const RemasterEmeraldSave *save)
{
    if (save == 0)
        return 0;
    return save->pokemon_storage[STORAGE_CURRENT_BOX];
}

int remaster_emerald_storage_set_current_box(
    RemasterEmeraldSave *save,
    uint8_t box)
{
    if (save == 0 || box >= REMASTER_EMERALD_STORAGE_BOX_COUNT)
        return 0;
    save->pokemon_storage[STORAGE_CURRENT_BOX] = box;
    return 1;
}

int remaster_emerald_storage_get(
    const RemasterEmeraldSave *save,
    size_t box,
    size_t slot,
    RemasterEmeraldBoxPokemon *out,
    int *out_checksum_valid)
{
    size_t offset;

    if (save == 0 || out == 0
        || box >= REMASTER_EMERALD_STORAGE_BOX_COUNT
        || slot >= REMASTER_EMERALD_STORAGE_BOX_CAPACITY)
        return 0;

    offset = storage_offset(box, slot);
    return remaster_emerald_box_pokemon_decode(
        save->pokemon_storage + offset,
        REMASTER_EMERALD_BOX_POKEMON_BYTES,
        out,
        out_checksum_valid);
}

int remaster_emerald_storage_set(
    RemasterEmeraldSave *save,
    size_t box,
    size_t slot,
    const RemasterEmeraldBoxPokemon *pokemon)
{
    size_t offset;

    if (save == 0 || pokemon == 0
        || box >= REMASTER_EMERALD_STORAGE_BOX_COUNT
        || slot >= REMASTER_EMERALD_STORAGE_BOX_CAPACITY)
        return 0;

    offset = storage_offset(box, slot);
    return remaster_emerald_box_pokemon_encode(
        pokemon,
        save->pokemon_storage + offset,
        REMASTER_EMERALD_BOX_POKEMON_BYTES);
}

size_t remaster_emerald_species_info_count(void)
{
    return sizeof(kRemasterEmeraldSpeciesInfo)
        / sizeof(kRemasterEmeraldSpeciesInfo[0]);
}

const RemasterEmeraldSpeciesInfo *remaster_emerald_species_info(
    uint16_t species_id)
{
    if ((size_t)species_id >= remaster_emerald_species_info_count())
        return 0;
    return &kRemasterEmeraldSpeciesInfo[species_id];
}

size_t remaster_emerald_move_info_count(void)
{
    return sizeof(kRemasterEmeraldMoveInfo)
        / sizeof(kRemasterEmeraldMoveInfo[0]);
}

const RemasterEmeraldMoveInfo *remaster_emerald_move_info(
    uint16_t move_id)
{
    if ((size_t)move_id >= remaster_emerald_move_info_count())
        return 0;
    return &kRemasterEmeraldMoveInfo[move_id];
}

size_t remaster_emerald_item_info_count(void)
{
    return sizeof(kRemasterEmeraldItemInfo)
        / sizeof(kRemasterEmeraldItemInfo[0]);
}

const RemasterEmeraldItemInfo *remaster_emerald_item_info(
    uint16_t item_id)
{
    if ((size_t)item_id >= remaster_emerald_item_info_count())
        return 0;
    return &kRemasterEmeraldItemInfo[item_id];
}

const RemasterEmeraldEvolution *remaster_emerald_species_evolutions(
    uint16_t species_id)
{
    if ((size_t)species_id >= remaster_emerald_species_info_count())
        return 0;
    return kRemasterEmeraldEvolutionTable[species_id];
}

uint8_t remaster_emerald_species_ability(
    uint16_t species_id,
    uint8_t ability_num)
{
    const RemasterEmeraldSpeciesInfo *info =
        remaster_emerald_species_info(species_id);

    if (info == 0 || ability_num > 1)
        return 0;
    return info->abilities[ability_num];
}

uint32_t remaster_emerald_experience_for_level(
    uint8_t growth_rate,
    uint8_t level)
{
    uint32_t n = level;
    uint32_t cube;
    uint32_t square;

    if (level == 0)
        return 0;
    if (level == 1)
        return 1;
    if (level > 100)
        level = 100;

    n = level;
    square = n * n;
    cube = square * n;

    switch (growth_rate) {
    case 0: /* Medium Fast */
        return cube;
    case 1: /* Erratic */
        if (n <= 50)
            return (100u - n) * cube / 50u;
        if (n <= 68)
            return (150u - n) * cube / 100u;
        if (n <= 98)
            return ((1911u - 10u * n) / 3u) * cube / 500u;
        return (160u - n) * cube / 100u;
    case 2: /* Fluctuating */
        if (n <= 15)
            return (((n + 1u) / 3u + 24u) * cube) / 50u;
        if (n <= 36)
            return ((n + 14u) * cube) / 50u;
        return (((n / 2u) + 32u) * cube) / 50u;
    case 3: /* Medium Slow */
        return (6u * cube) / 5u
            - 15u * square
            + 100u * n
            - 140u;
    case 4: /* Fast */
        return (4u * cube) / 5u;
    case 5: /* Slow */
        return (5u * cube) / 4u;
    default:
        return cube;
    }
}

uint8_t remaster_emerald_level_from_experience(
    uint16_t species_id,
    uint32_t experience)
{
    const RemasterEmeraldSpeciesInfo *info =
        remaster_emerald_species_info(species_id);
    uint8_t level = 1;

    if (info == 0)
        return 0;

    while (level < 100
        && experience >= remaster_emerald_experience_for_level(
            info->growth_rate,
            (uint8_t)(level + 1u)))
        ++level;

    return level;
}

static uint16_t apply_nature(
    uint16_t stat,
    uint8_t nature,
    size_t non_hp_stat)
{
    const uint8_t increase = (uint8_t)(nature / 5u);
    const uint8_t decrease = (uint8_t)(nature % 5u);

    if (nature >= 25 || increase == decrease)
        return stat;
    if (increase == non_hp_stat)
        return (uint16_t)(((uint32_t)stat * 110u) / 100u);
    if (decrease == non_hp_stat)
        return (uint16_t)(((uint32_t)stat * 90u) / 100u);
    return stat;
}

int remaster_emerald_calculate_stats(
    uint16_t species_id,
    uint8_t level,
    uint8_t nature,
    const uint8_t ivs[6],
    const uint8_t evs[6],
    RemasterEmeraldCalculatedStats *out_stats)
{
    const RemasterEmeraldSpeciesInfo *info =
        remaster_emerald_species_info(species_id);
    uint8_t base[6];
    uint16_t raw[6];
    size_t i;

    if (info == 0 || ivs == 0 || evs == 0 || out_stats == 0
        || level == 0 || level > 100 || nature >= 25)
        return 0;

    base[0] = info->base_hp;
    base[1] = info->base_attack;
    base[2] = info->base_defense;
    base[3] = info->base_speed;
    base[4] = info->base_sp_attack;
    base[5] = info->base_sp_defense;

    if (species_id == SPECIES_SHEDINJA) {
        raw[0] = 1;
    } else {
        raw[0] = (uint16_t)(
            (((2u * base[0] + ivs[0] + evs[0] / 4u) * level) / 100u)
            + level + 10u);
    }

    for (i = 1; i < 6; ++i) {
        uint16_t stat = (uint16_t)(
            (((2u * base[i] + ivs[i] + evs[i] / 4u) * level) / 100u)
            + 5u);
        raw[i] = apply_nature(stat, nature, i - 1u);
    }

    out_stats->hp = raw[0];
    out_stats->attack = raw[1];
    out_stats->defense = raw[2];
    out_stats->speed = raw[3];
    out_stats->sp_attack = raw[4];
    out_stats->sp_defense = raw[5];
    return 1;
}
