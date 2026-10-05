#include "remaster/emerald_pokemon.h"

#include <stdio.h>
#include <string.h>

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
        fprintf(stderr, "emerald_pokemon_codec_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldBoxPokemon mon;
    uint8_t encoded[REMASTER_EMERALD_BOX_POKEMON_BYTES];
    int checksum_valid = 0;
    uint8_t ivs[6];
    uint8_t evs[6];
    uint16_t moves[4];
    uint8_t pp[4];
    size_t i;

    memset(&mon, 0, sizeof(mon));
    memset(encoded, 0, sizeof(encoded));

    if (!check(
            remaster_emerald_box_pokemon_decode(
                kKnownBoxPokemon,
                sizeof(kKnownBoxPokemon),
                &mon,
                &checksum_valid),
            "known encrypted BoxPokemon failed to decode"))
        return 1;

    if (!check(checksum_valid, "known BoxPokemon checksum should validate"))
        return 1;

    if (!check(
            mon.personality == UINT32_C(0x00000011)
            && mon.ot_id == UINT32_C(0xA1B2C3D4),
            "header identity mismatch"))
        return 1;

    if (!check(
            remaster_emerald_box_pokemon_species(&mon) == 21
            && remaster_emerald_box_pokemon_held_item(&mon) == 95
            && remaster_emerald_box_pokemon_experience(&mon)
                == UINT32_C(0x12345678)
            && remaster_emerald_box_pokemon_friendship(&mon) == 70,
            "substruct-0 decode mismatch"))
        return 1;

    remaster_emerald_box_pokemon_moves(&mon, moves, pp);
    if (!check(
            moves[0] == 33
            && moves[1] == 85
            && moves[2] == 85
            && moves[3] == 57
            && pp[0] == 35
            && pp[1] == 33
            && pp[2] == 15
            && pp[3] == 25,
            "substruct-1 move/PP decode mismatch"))
        return 1;

    remaster_emerald_box_pokemon_evs(&mon, evs);
    for (i = 0; i < 6; ++i) {
        if (!check(
                evs[i] == (uint8_t)(i + 1u),
                "substruct-2 EV decode mismatch"))
            return 1;
    }

    remaster_emerald_box_pokemon_ivs(&mon, ivs);
    if (!check(
            ivs[0] == 1
            && ivs[1] == 25
            && ivs[2] == 16
            && ivs[3] == 10
            && ivs[4] == 22
            && ivs[5] == 3,
            "substruct-3 IV decode mismatch"))
        return 1;

    if (!check(
            remaster_emerald_box_pokemon_met_level(&mon) == 21
            && remaster_emerald_box_pokemon_met_game(&mon) == 5
            && remaster_emerald_box_pokemon_pokeball(&mon) == 5
            && remaster_emerald_box_pokemon_ot_gender(&mon) == 1
            && remaster_emerald_box_pokemon_ability_num(&mon) == 1
            && remaster_emerald_box_pokemon_nature(&mon) == 17,
            "origins/ability/nature decode mismatch"))
        return 1;

    if (!check(
            remaster_emerald_box_pokemon_encode(
                &mon,
                encoded,
                sizeof(encoded)),
            "known BoxPokemon failed to re-encode"))
        return 1;

    if (!check(
            memcmp(encoded, kKnownBoxPokemon, sizeof(encoded)) == 0,
            "BoxPokemon byte-for-byte round-trip mismatch"))
        return 1;

    /*
     * Every Gen III personality permutation must encode/decode canonical
     * substructs without losing any bytes.
     */
    for (i = 0; i < 24; ++i) {
        RemasterEmeraldBoxPokemon generated;
        RemasterEmeraldBoxPokemon decoded;
        uint8_t raw[REMASTER_EMERALD_BOX_POKEMON_BYTES];
        int valid = 0;
        size_t substruct;
        size_t byte_index;

        memset(&generated, 0, sizeof(generated));
        generated.personality = (uint32_t)i;
        generated.ot_id = UINT32_C(0x10203040) + (uint32_t)i;
        generated.language = 2;
        generated.has_species = 1;

        for (substruct = 0; substruct < 4; ++substruct) {
            for (byte_index = 0; byte_index < 12; ++byte_index) {
                generated.substruct[substruct][byte_index] =
                    (uint8_t)(1u + substruct * 20u + byte_index);
            }
        }

        if (!check(
                remaster_emerald_box_pokemon_encode(
                    &generated,
                    raw,
                    sizeof(raw)),
                "permutation encode failed"))
            return 1;

        memset(&decoded, 0, sizeof(decoded));
        if (!check(
                remaster_emerald_box_pokemon_decode(
                    raw,
                    sizeof(raw),
                    &decoded,
                    &valid)
                && valid,
                "permutation decode/checksum failed"))
            return 1;

        if (!check(
                memcmp(
                    generated.substruct,
                    decoded.substruct,
                    sizeof(generated.substruct)) == 0,
                "personality permutation changed canonical substruct bytes"))
            return 1;
    }

    {
        uint8_t corrupt[REMASTER_EMERALD_BOX_POKEMON_BYTES];
        RemasterEmeraldBoxPokemon decoded;
        int valid = 1;

        memcpy(corrupt, kKnownBoxPokemon, sizeof(corrupt));
        corrupt[40] ^= 0x01;

        if (!check(
                remaster_emerald_box_pokemon_decode(
                    corrupt,
                    sizeof(corrupt),
                    &decoded,
                    &valid),
                "corrupt BoxPokemon should remain structurally decodable"))
            return 1;

        if (!check(!valid, "corrupt secure data must fail checksum validation"))
            return 1;
    }

    puts("R11 BoxPokemon codec regression passed.");
    return 0;
}
