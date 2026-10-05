#include "remaster/emerald_quest.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    FOOTER_ID_OFFSET = 4084,
    FOOTER_CHECKSUM_OFFSET = 4086,
    FOOTER_SIGNATURE_OFFSET = 4088,
    FOOTER_COUNTER_OFFSET = 4092,
    FLAG_SYS_POKEMON_GET = 0x0860,
    FLAG_DEFEATED_RIVAL_ROUTE103 = 0x0082,
    FLAG_SYS_POKEDEX_GET = 0x0861
};

static const uint32_t kSignature = UINT32_C(0x08012025);

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

static size_t sector_size(uint16_t id)
{
    if (id == 0)
        return REMASTER_EMERALD_SAVE_BLOCK2_BYTES;
    if (id >= 1 && id <= 3)
        return REMASTER_EMERALD_SECTOR_DATA_BYTES;
    if (id == 4)
        return REMASTER_EMERALD_SAVE_BLOCK1_BYTES
            - 3u * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    if (id >= 5 && id <= 12)
        return REMASTER_EMERALD_SECTOR_DATA_BYTES;
    if (id == 13)
        return REMASTER_EMERALD_STORAGE_BYTES
            - 8u * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    return 0;
}

static const uint8_t *payload_for(
    const RemasterEmeraldSave *save,
    uint16_t id)
{
    if (id == 0)
        return save->save_block2;
    if (id <= 4)
        return save->save_block1
            + (size_t)(id - 1u) * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    return save->pokemon_storage
        + (size_t)(id - 5u) * REMASTER_EMERALD_SECTOR_DATA_BYTES;
}

static void build_slot(
    uint8_t *image,
    uint8_t slot,
    uint32_t counter,
    uint16_t rotation,
    const RemasterEmeraldSave *save)
{
    uint16_t id;

    for (id = 0; id < REMASTER_EMERALD_MAIN_SECTORS; ++id) {
        const uint16_t physical =
            (uint16_t)((id + rotation) % REMASTER_EMERALD_MAIN_SECTORS);
        uint8_t *sector =
            image
            + ((size_t)slot * REMASTER_EMERALD_MAIN_SECTORS + physical)
            * REMASTER_EMERALD_SECTOR_BYTES;
        const uint8_t *payload = payload_for(save, id);
        const size_t size = sector_size(id);

        memset(sector, 0, REMASTER_EMERALD_SECTOR_BYTES);
        memcpy(sector, payload, size);
        put16(sector + FOOTER_ID_OFFSET, id);
        put16(
            sector + FOOTER_CHECKSUM_OFFSET,
            remaster_emerald_checksum(payload, size));
        put32(sector + FOOTER_SIGNATURE_OFFSET, kSignature);
        put32(sector + FOOTER_COUNTER_OFFSET, counter);
    }
}

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_quest_save_test: %s\n", message);
        return 0;
    }

    return 1;
}

int main(void)
{
    uint8_t *image;
    RemasterEmeraldSave initial;
    RemasterEmeraldSave live;
    RemasterEmeraldSave reloaded;
    const RemasterEmeraldQuestObjective *active;

    if (!check(
            REMASTER_EMERALD_SAVE_BLOCK2_BYTES == 0x0F44
            && REMASTER_EMERALD_SAVE_BLOCK1_BYTES == 0x3DC8
            && REMASTER_EMERALD_STORAGE_BYTES == 0x83D0,
            "R10 must not change Emerald save geometry"))
        return 1;

    image = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (image == 0)
        return 1;

    memset(image, 0xff, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    memset(&initial, 0, sizeof(initial));
    memset(&live, 0, sizeof(live));
    memset(&reloaded, 0, sizeof(reloaded));

    if (!check(
            remaster_emerald_flag_set(&initial, FLAG_SYS_POKEMON_GET, 1)
            && remaster_emerald_flag_set(
                &initial,
                FLAG_DEFEATED_RIVAL_ROUTE103,
                1),
            "failed to seed story flags"))
        return 1;

    build_slot(image, 0, 60, 3, &initial);
    build_slot(image, 1, 61, 7, &initial);

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &live) == REMASTER_EMERALD_SAVE_OK,
            "failed to decode story save"))
        return 1;

    active = remaster_emerald_quest_active(&live);
    if (!check(
            active != 0
            && active->id == REMASTER_EMERALD_QUEST_RETURN_TO_BIRCH,
            "decoded save should derive RETURN_TO_BIRCH"))
        return 1;

    if (!check(
            remaster_emerald_flag_set(&live, FLAG_SYS_POKEDEX_GET, 1),
            "failed to advance story flag"))
        return 1;

    active = remaster_emerald_quest_active(&live);
    if (!check(
            active != 0
            && active->id == REMASTER_EMERALD_QUEST_VISIT_PETALBURG_GYM,
            "story mutation should derive VISIT_PETALBURG_GYM"))
        return 1;

    if (!check(
            remaster_emerald_save_encode_next(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &live),
            "failed to encode advanced story save"))
        return 1;

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &reloaded) == REMASTER_EMERALD_SAVE_OK,
            "advanced story save failed to reload"))
        return 1;

    active = remaster_emerald_quest_active(&reloaded);
    if (!check(
            active != 0
            && active->id == REMASTER_EMERALD_QUEST_VISIT_PETALBURG_GYM,
            "derived objective changed across save round-trip"))
        return 1;

    free(image);
    puts("R10 quest/save round-trip regression passed.");
    return 0;
}
