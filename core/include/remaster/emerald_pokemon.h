#ifndef REMASTER_EMERALD_POKEMON_H
#define REMASTER_EMERALD_POKEMON_H

#include "remaster/emerald_save.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_BOX_POKEMON_BYTES = 80,
    REMASTER_EMERALD_PARTY_POKEMON_BYTES = 100,
    REMASTER_EMERALD_PARTY_SIZE = 6,
    REMASTER_EMERALD_STORAGE_BOX_COUNT = 14,
    REMASTER_EMERALD_STORAGE_BOX_CAPACITY = 30,
    REMASTER_EMERALD_MAX_MOVES = 4,
    REMASTER_EMERALD_STAT_COUNT = 6,
    REMASTER_EMERALD_EVOLUTIONS_PER_SPECIES = 5
};

typedef struct RemasterEmeraldBoxPokemon {
    uint32_t personality;
    uint32_t ot_id;
    uint8_t nickname[10];
    uint8_t language;
    uint8_t header_flags;
    uint8_t ot_name[7];
    uint8_t markings;
    uint16_t checksum;
    uint16_t unknown;
    /*
     * Canonical plaintext substruct order 0..3. Keeping all 48 bytes preserves
     * ribbons/origin bits and other fields not yet interpreted by R11.
     */
    uint8_t substruct[4][12];
} RemasterEmeraldBoxPokemon;

typedef struct RemasterEmeraldPartyPokemon {
    RemasterEmeraldBoxPokemon box;
    uint32_t status;
    uint8_t level;
    uint8_t mail;
    uint16_t hp;
    uint16_t max_hp;
    uint16_t attack;
    uint16_t defense;
    uint16_t speed;
    uint16_t sp_attack;
    uint16_t sp_defense;
} RemasterEmeraldPartyPokemon;

typedef struct RemasterEmeraldSpeciesInfo {
    uint16_t species_id;
    uint8_t base_hp;
    uint8_t base_attack;
    uint8_t base_defense;
    uint8_t base_speed;
    uint8_t base_sp_attack;
    uint8_t base_sp_defense;
    uint8_t type1;
    uint8_t type2;
    uint8_t catch_rate;
    uint8_t exp_yield;
    uint8_t ev_yield[6];
    uint16_t item_common;
    uint16_t item_rare;
    uint8_t gender_ratio;
    uint8_t egg_cycles;
    uint8_t friendship;
    uint8_t growth_rate;
    uint8_t egg_groups[2];
    uint8_t abilities[2];
} RemasterEmeraldSpeciesInfo;

typedef struct RemasterEmeraldMoveInfo {
    uint16_t move_id;
    uint8_t effect;
    uint8_t power;
    uint8_t type;
    uint8_t accuracy;
    uint8_t pp;
    uint8_t secondary_effect_chance;
    uint8_t target;
    int8_t priority;
    uint8_t flags;
} RemasterEmeraldMoveInfo;

typedef struct RemasterEmeraldEvolution {
    uint16_t method;
    uint16_t param;
    uint16_t target_species;
} RemasterEmeraldEvolution;

typedef struct RemasterEmeraldCalculatedStats {
    uint16_t hp;
    uint16_t attack;
    uint16_t defense;
    uint16_t speed;
    uint16_t sp_attack;
    uint16_t sp_defense;
} RemasterEmeraldCalculatedStats;

int remaster_emerald_box_pokemon_decode(
    const uint8_t *raw,
    size_t raw_size,
    RemasterEmeraldBoxPokemon *out,
    int *out_checksum_valid);

int remaster_emerald_box_pokemon_encode(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint8_t *out_raw,
    size_t out_size);

uint16_t remaster_emerald_box_pokemon_checksum(
    const RemasterEmeraldBoxPokemon *pokemon);

uint16_t remaster_emerald_box_pokemon_species(
    const RemasterEmeraldBoxPokemon *pokemon);
uint16_t remaster_emerald_box_pokemon_held_item(
    const RemasterEmeraldBoxPokemon *pokemon);
uint32_t remaster_emerald_box_pokemon_experience(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_box_pokemon_friendship(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_box_pokemon_nature(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_box_pokemon_ability_num(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_box_pokemon_met_level(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_box_pokemon_met_game(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_box_pokemon_pokeball(
    const RemasterEmeraldBoxPokemon *pokemon);
uint8_t remaster_emerald_box_pokemon_ot_gender(
    const RemasterEmeraldBoxPokemon *pokemon);

void remaster_emerald_box_pokemon_moves(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint16_t out_moves[4],
    uint8_t out_pp[4]);
void remaster_emerald_box_pokemon_evs(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint8_t out_evs[6]);
void remaster_emerald_box_pokemon_ivs(
    const RemasterEmeraldBoxPokemon *pokemon,
    uint8_t out_ivs[6]);

int remaster_emerald_box_pokemon_set_species(
    RemasterEmeraldBoxPokemon *pokemon,
    uint16_t species);
int remaster_emerald_box_pokemon_set_held_item(
    RemasterEmeraldBoxPokemon *pokemon,
    uint16_t item_id);
int remaster_emerald_box_pokemon_set_experience(
    RemasterEmeraldBoxPokemon *pokemon,
    uint32_t experience);
int remaster_emerald_box_pokemon_set_friendship(
    RemasterEmeraldBoxPokemon *pokemon,
    uint8_t friendship);
int remaster_emerald_box_pokemon_set_move(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t slot,
    uint16_t move_id,
    uint8_t pp);
int remaster_emerald_box_pokemon_set_ev(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t stat,
    uint8_t value);
int remaster_emerald_box_pokemon_set_iv(
    RemasterEmeraldBoxPokemon *pokemon,
    size_t stat,
    uint8_t value);
int remaster_emerald_box_pokemon_set_ability_num(
    RemasterEmeraldBoxPokemon *pokemon,
    uint8_t ability_num);

uint8_t remaster_emerald_party_count(const RemasterEmeraldSave *save);
int remaster_emerald_party_set_count(
    RemasterEmeraldSave *save,
    uint8_t count);
int remaster_emerald_party_get(
    const RemasterEmeraldSave *save,
    size_t slot,
    RemasterEmeraldPartyPokemon *out,
    int *out_checksum_valid);
int remaster_emerald_party_set(
    RemasterEmeraldSave *save,
    size_t slot,
    const RemasterEmeraldPartyPokemon *pokemon);

uint8_t remaster_emerald_storage_current_box(
    const RemasterEmeraldSave *save);
int remaster_emerald_storage_set_current_box(
    RemasterEmeraldSave *save,
    uint8_t box);
int remaster_emerald_storage_get(
    const RemasterEmeraldSave *save,
    size_t box,
    size_t slot,
    RemasterEmeraldBoxPokemon *out,
    int *out_checksum_valid);
int remaster_emerald_storage_set(
    RemasterEmeraldSave *save,
    size_t box,
    size_t slot,
    const RemasterEmeraldBoxPokemon *pokemon);

size_t remaster_emerald_species_info_count(void);
const RemasterEmeraldSpeciesInfo *remaster_emerald_species_info(
    uint16_t species_id);

size_t remaster_emerald_move_info_count(void);
const RemasterEmeraldMoveInfo *remaster_emerald_move_info(
    uint16_t move_id);

const RemasterEmeraldEvolution *remaster_emerald_species_evolutions(
    uint16_t species_id);

uint8_t remaster_emerald_species_ability(
    uint16_t species_id,
    uint8_t ability_num);

uint32_t remaster_emerald_experience_for_level(
    uint8_t growth_rate,
    uint8_t level);

uint8_t remaster_emerald_level_from_experience(
    uint16_t species_id,
    uint32_t experience);

int remaster_emerald_calculate_stats(
    uint16_t species_id,
    uint8_t level,
    uint8_t nature,
    const uint8_t ivs[6],
    const uint8_t evs[6],
    RemasterEmeraldCalculatedStats *out_stats);

#ifdef __cplusplus
}
#endif

#endif
