#include "remaster/emerald_items.h"
#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_qol.h"
#include "remaster/emerald_quest.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    FLAG_SYS_POKEMON_GET = 0x0860,
    FLAG_DEFEATED_RIVAL_ROUTE103 = 0x0082,
    ITEM_POTION = 13,
    ITEM_ANTIDOTE = 14,
    MOVE_TACKLE = 33,
    PARTY_SPECIES = 25,
    STORAGE_SPECIES = 277
};

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_save_domain_roundtrip_test: %s\n", message);
        return 0;
    }
    return 1;
}

static RemasterEmeraldBoxPokemon make_box(
    uint16_t species,
    uint8_t level,
    uint32_t personality)
{
    RemasterEmeraldBoxPokemon mon;
    const RemasterEmeraldSpeciesInfo *info;

    memset(&mon, 0, sizeof(mon));
    mon.personality = personality;
    mon.ot_id = UINT32_C(0x12345678);
    memcpy(mon.nickname, "COMPAT", 6);
    memcpy(mon.ot_name, "PLAYER", 6);

    if (!remaster_emerald_box_pokemon_set_species(&mon, species))
        return mon;

    info = remaster_emerald_species_info(species);
    if (info != 0) {
        remaster_emerald_box_pokemon_set_experience(
            &mon,
            remaster_emerald_experience_for_level(
                info->growth_rate,
                level));
    }

    remaster_emerald_box_pokemon_set_held_item(&mon, ITEM_POTION);
    remaster_emerald_box_pokemon_set_move(&mon, 0, MOVE_TACKLE, 35);
    remaster_emerald_box_pokemon_set_iv(&mon, 0, 31);
    remaster_emerald_box_pokemon_set_iv(&mon, 1, 30);
    remaster_emerald_box_pokemon_set_ev(&mon, 0, 100);
    remaster_emerald_box_pokemon_set_ev(&mon, 1, 80);
    return mon;
}

int main(void)
{
    uint8_t *image;
    uint8_t special_before[4 * REMASTER_EMERALD_SECTOR_BYTES];
    RemasterEmeraldSave source;
    RemasterEmeraldSave decoded;
    RemasterEmeraldOverworldState world;
    RemasterEmeraldOverworldState observed_world;
    RemasterEmeraldPartyPokemon party;
    RemasterEmeraldPartyPokemon observed_party;
    RemasterEmeraldBoxPokemon storage;
    RemasterEmeraldBoxPokemon observed_storage;
    const RemasterEmeraldQuestObjective *objective;
    int checksum_ok = 0;
    size_t i;
    const size_t special_offset =
        28u * REMASTER_EMERALD_SECTOR_BYTES;

    image = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (image == 0)
        return 1;

    memset(image, 0xFF, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    memset(&source, 0, sizeof(source));
    memset(&decoded, 0, sizeof(decoded));
    memset(&world, 0, sizeof(world));
    memset(&observed_world, 0, sizeof(observed_world));
    memset(&party, 0, sizeof(party));

    /*
     * Non-main sectors represent Hall of Fame / Trainer Hill / Recorded
     * Battle data. Normal game saves must preserve them byte-for-byte.
     */
    for (i = 0; i < sizeof(special_before); ++i) {
        special_before[i] = (uint8_t)((i * 37u + 11u) & 0xFFu);
        image[special_offset + i] = special_before[i];
    }

    /* Use a real non-zero Emerald encryption key before bag operations. */
    source.save_block2[0xAC] = 0x78;
    source.save_block2[0xAD] = 0x56;
    source.save_block2[0xAE] = 0x34;
    source.save_block2[0xAF] = 0x12;

    memcpy(source.save_block2, "PLAYER", 6);
    source.save_block2[6] = 0xFF;
    source.save_block2[0x0A] = 0x78;
    source.save_block2[0x0B] = 0x56;
    source.save_block2[0x0C] = 0x34;
    source.save_block2[0x0D] = 0x12;

    world.player_x = 17;
    world.player_y = 9;
    world.map_group = 0;
    world.map_num = 9;
    world.warp_id = 1;
    world.warp_x = -1;
    world.warp_y = -1;
    world.map_layout_id = 12;
    world.saved_music = 321;
    world.weather = 1;
    world.weather_cycle_stage = 2;
    world.flash_level = 0;
    world.party_count = 1;
    world.money = 654321;
    world.coins = 777;
    world.registered_item = ITEM_POTION;

    if (!check(
            remaster_emerald_overworld_set(&source, &world),
            "failed to seed overworld state"))
        return 1;

    if (!check(
            remaster_emerald_flag_set(
                &source,
                FLAG_SYS_POKEMON_GET,
                1)
            && remaster_emerald_flag_set(
                &source,
                FLAG_DEFEATED_RIVAL_ROUTE103,
                1),
            "failed to seed quest flags"))
        return 1;

    if (!check(
            remaster_emerald_bag_add(&source, ITEM_POTION, 7)
            && remaster_emerald_bag_add(&source, ITEM_ANTIDOTE, 3),
            "failed to seed bag"))
        return 1;

    if (!check(
            remaster_emerald_qol_quick_item_register(
                &source,
                ITEM_POTION)
            && remaster_emerald_qol_quick_item_register(
                &source,
                ITEM_ANTIDOTE),
            "failed to seed Vanilla+ quick-item metadata"))
        return 1;

    party.box = make_box(PARTY_SPECIES, 20, 101);
    party.level = 20;
    party.mail = 0xFF;
    party.hp = 50;
    party.max_hp = 50;
    party.attack = 30;
    party.defense = 25;
    party.speed = 40;
    party.sp_attack = 35;
    party.sp_defense = 30;

    if (!check(
            remaster_emerald_party_set(&source, 0, &party)
            && remaster_emerald_party_set_count(&source, 1),
            "failed to seed party"))
        return 1;

    storage = make_box(STORAGE_SPECIES, 15, 202);
    if (!check(
            remaster_emerald_storage_set_current_box(&source, 4)
            && remaster_emerald_storage_set(
                &source,
                4,
                6,
                &storage),
            "failed to seed PC storage"))
        return 1;

    objective = remaster_emerald_quest_active(&source);
    if (!check(
            objective != 0
            && objective->id == REMASTER_EMERALD_QUEST_RETURN_TO_BIRCH,
            "seeded story state did not derive expected objective"))
        return 1;

    if (!check(
            remaster_emerald_save_encode_next(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &source),
            "failed to encode full-domain save"))
        return 1;

    if (!check(
            memcmp(
                image + special_offset,
                special_before,
                sizeof(special_before)) == 0,
            "full-domain write modified special sectors"))
        return 1;

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_OK,
            "full-domain save failed to decode"))
        return 1;

    if (!check(
            memcmp(
                source.save_block2,
                decoded.save_block2,
                REMASTER_EMERALD_SAVE_BLOCK2_BYTES) == 0
            && memcmp(
                source.save_block1,
                decoded.save_block1,
                REMASTER_EMERALD_SAVE_BLOCK1_BYTES) == 0
            && memcmp(
                source.pokemon_storage,
                decoded.pokemon_storage,
                REMASTER_EMERALD_STORAGE_BYTES) == 0,
            "authoritative gameplay payload changed across round-trip"))
        return 1;

    if (!check(
            remaster_emerald_overworld_get(
                &decoded,
                &observed_world)
            && observed_world.player_x == world.player_x
            && observed_world.player_y == world.player_y
            && observed_world.map_group == world.map_group
            && observed_world.map_num == world.map_num
            && observed_world.money == world.money
            && observed_world.coins == world.coins,
            "world state changed across round-trip"))
        return 1;

    objective = remaster_emerald_quest_active(&decoded);
    if (!check(
            objective != 0
            && objective->id == REMASTER_EMERALD_QUEST_RETURN_TO_BIRCH,
            "derived quest objective changed across round-trip"))
        return 1;

    if (!check(
            remaster_emerald_party_count(&decoded) == 1
            && remaster_emerald_party_get(
                &decoded,
                0,
                &observed_party,
                &checksum_ok)
            && checksum_ok
            && remaster_emerald_box_pokemon_species(
                &observed_party.box) == PARTY_SPECIES
            && remaster_emerald_box_pokemon_held_item(
                &observed_party.box) == ITEM_POTION,
            "party state changed across round-trip"))
        return 1;

    if (!check(
            remaster_emerald_storage_current_box(&decoded) == 4
            && remaster_emerald_storage_get(
                &decoded,
                4,
                6,
                &observed_storage,
                &checksum_ok)
            && checksum_ok
            && remaster_emerald_box_pokemon_species(
                &observed_storage) == STORAGE_SPECIES,
            "PC storage changed across round-trip"))
        return 1;

    if (!check(
            remaster_emerald_bag_count(
                &decoded,
                ITEM_POTION) == 7
            && remaster_emerald_bag_count(
                &decoded,
                ITEM_ANTIDOTE) == 3,
            "bag state changed across round-trip"))
        return 1;

    if (!check(
            remaster_emerald_qol_quick_item_count(&decoded) == 2
            && remaster_emerald_qol_quick_item_get(
                &decoded,
                0) == ITEM_POTION
            && remaster_emerald_qol_quick_item_get(
                &decoded,
                1) == ITEM_ANTIDOTE,
            "Vanilla+ quick-item metadata changed across round-trip"))
        return 1;

    free(image);
    puts("R17 full-domain save round-trip passed.");
    return 0;
}
