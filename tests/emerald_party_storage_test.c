#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_save.h"

#include <stdio.h>
#include <string.h>

enum {
    SB1_PARTY_COUNT = 0x0234,
    SB1_PARTY = 0x0238,
    STORAGE_BOXES = 0x0004
};

static const uint8_t kKnownBoxPokemon[REMASTER_EMERALD_BOX_POKEMON_BYTES] = {
    0x11,0x00,0x00,0x00,0xD4,0xC3,0xB2,0xA1,
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,
    0x02,0x06,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0xA5,
    0x52,0x5B,0xEF,0xBE,
    0xC4,0xC1,0xB1,0xA5,0xC0,0xC5,0xB5,0xA9,
    0xCC,0xC9,0xB9,0xAD,0xD7,0xF7,0x27,0x0B,
    0xE4,0x80,0xD7,0x26,0x2A,0x7D,0x1F,0x7F,
    0xE4,0xC3,0xE7,0xA1,0x90,0xC3,0x8B,0xA1,
    0xE6,0xE2,0xBD,0xB8,0xD0,0xC3,0xED,0xA1,
    0xBD,0x95,0x86,0xB3,0xCA,0x85,0xB2,0xA1
};

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_party_storage_test: %s\n", message);
        return 0;
    }
    return 1;
}

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldPartyPokemon party;
    RemasterEmeraldBoxPokemon boxed;
    int valid = 0;
    uint8_t expected_party[REMASTER_EMERALD_PARTY_POKEMON_BYTES];
    const size_t last_box_index =
        (REMASTER_EMERALD_STORAGE_BOX_COUNT - 1u)
        * REMASTER_EMERALD_STORAGE_BOX_CAPACITY
        + (REMASTER_EMERALD_STORAGE_BOX_CAPACITY - 1u);
    const size_t last_storage_offset =
        STORAGE_BOXES
        + last_box_index * REMASTER_EMERALD_BOX_POKEMON_BYTES;

    memset(&save, 0, sizeof(save));
    memset(&party, 0, sizeof(party));
    memset(&boxed, 0, sizeof(boxed));
    memset(expected_party, 0, sizeof(expected_party));

    if (!check(
            REMASTER_EMERALD_BOX_POKEMON_BYTES == 80
            && REMASTER_EMERALD_PARTY_POKEMON_BYTES == 100
            && REMASTER_EMERALD_PARTY_SIZE == 6
            && REMASTER_EMERALD_STORAGE_BOX_COUNT == 14
            && REMASTER_EMERALD_STORAGE_BOX_CAPACITY == 30,
            "R11 binary geometry constants mismatch"))
        return 1;

    if (!check(
            last_storage_offset + REMASTER_EMERALD_BOX_POKEMON_BYTES
                == 0x8344,
            "storage box payload must end exactly at box-name offset 0x8344"))
        return 1;

    save.save_block1[SB1_PARTY_COUNT] = 1;
    memcpy(
        save.save_block1 + SB1_PARTY,
        kKnownBoxPokemon,
        sizeof(kKnownBoxPokemon));

    memcpy(expected_party, kKnownBoxPokemon, sizeof(kKnownBoxPokemon));
    put32(expected_party + 80, UINT32_C(0x00000008));
    expected_party[84] = 42;
    expected_party[85] = 0xFF;
    put16(expected_party + 86, 73);
    put16(expected_party + 88, 100);
    put16(expected_party + 90, 81);
    put16(expected_party + 92, 65);
    put16(expected_party + 94, 91);
    put16(expected_party + 96, 102);
    put16(expected_party + 98, 77);

    memcpy(
        save.save_block1 + SB1_PARTY + 80,
        expected_party + 80,
        20);

    if (!check(
            remaster_emerald_party_count(&save) == 1,
            "party count offset mismatch"))
        return 1;

    if (!check(
            remaster_emerald_party_get(&save, 0, &party, &valid)
                && valid,
            "party slot 0 failed to decode"))
        return 1;

    if (!check(
            party.status == UINT32_C(0x00000008)
            && party.level == 42
            && party.mail == 0xFF
            && party.hp == 73
            && party.max_hp == 100
            && party.attack == 81
            && party.defense == 65
            && party.speed == 91
            && party.sp_attack == 102
            && party.sp_defense == 77,
            "party extension fields mismatch"))
        return 1;

    if (!check(
            remaster_emerald_party_set(&save, 1, &party),
            "party slot 1 write failed"))
        return 1;

    if (!check(
            memcmp(
                save.save_block1
                    + SB1_PARTY
                    + REMASTER_EMERALD_PARTY_POKEMON_BYTES,
                expected_party,
                sizeof(expected_party)) == 0,
            "party slot byte encoding mismatch"))
        return 1;

    if (!check(
            remaster_emerald_party_set_count(&save, 2)
            && remaster_emerald_party_count(&save) == 2,
            "party count setter mismatch"))
        return 1;

    if (!check(
            !remaster_emerald_party_set_count(
                &save,
                REMASTER_EMERALD_PARTY_SIZE + 1u),
            "party count must reject values above six"))
        return 1;

    save.pokemon_storage[0] = 13;
    memcpy(
        save.pokemon_storage + last_storage_offset,
        kKnownBoxPokemon,
        sizeof(kKnownBoxPokemon));

    if (!check(
            remaster_emerald_storage_current_box(&save) == 13,
            "current PC box offset mismatch"))
        return 1;

    if (!check(
            remaster_emerald_storage_get(
                &save,
                13,
                29,
                &boxed,
                &valid)
            && valid
            && remaster_emerald_box_pokemon_species(&boxed) == 21,
            "last PC storage slot failed to decode"))
        return 1;

    if (!check(
            remaster_emerald_storage_set(
                &save,
                0,
                0,
                &boxed),
            "first PC storage slot write failed"))
        return 1;

    if (!check(
            memcmp(
                save.pokemon_storage + STORAGE_BOXES,
                kKnownBoxPokemon,
                sizeof(kKnownBoxPokemon)) == 0,
            "PC storage slot byte encoding mismatch"))
        return 1;

    if (!check(
            save.pokemon_storage[0] == 13,
            "PC storage write must not overwrite currentBox/padding"))
        return 1;

    if (!check(
            remaster_emerald_storage_set_current_box(&save, 4)
            && remaster_emerald_storage_current_box(&save) == 4,
            "current PC box setter mismatch"))
        return 1;

    if (!check(
            !remaster_emerald_storage_set_current_box(
                &save,
                REMASTER_EMERALD_STORAGE_BOX_COUNT),
            "current PC box setter must reject box 14"))
        return 1;

    puts("R11 party/storage geometry regression passed.");
    return 0;
}
