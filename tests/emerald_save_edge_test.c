#include "remaster/emerald_save.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    FOOTER_ID_OFFSET = 4084,
    FOOTER_CHECKSUM_OFFSET = 4086,
    FOOTER_SIGNATURE_OFFSET = 4088,
    FOOTER_COUNTER_OFFSET = 4092
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
        fprintf(stderr, "emerald_save_edge_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    uint8_t *image;
    RemasterEmeraldSave payload;
    RemasterEmeraldSave decoded;
    size_t special_offset =
        31u * REMASTER_EMERALD_SECTOR_BYTES + 317u;

    image = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (image == 0)
        return 1;

    memset(&payload, 0, sizeof(payload));
    payload.save_block1[0] = 0x17;
    payload.save_block2[0] = 0x23;
    payload.pokemon_storage[0] = 0x42;

    memset(&decoded, 0xA5, sizeof(decoded));
    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES - 1u,
                &decoded) == REMASTER_EMERALD_SAVE_CORRUPT,
            "short image must be rejected"))
        return 1;
    if (!check(
            decoded.status == REMASTER_EMERALD_SAVE_CORRUPT
            && decoded.counter == 0
            && decoded.last_written_sector == 0
            && decoded.selected_slot == 0,
            "short decode must normalize output state"))
        return 1;

    memset(image, 0xFF, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    memset(&decoded, 0, sizeof(decoded));
    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_EMPTY,
            "all-erased image must be EMPTY"))
        return 1;

    build_slot(image, 0, 20, 4, &payload);
    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_OK,
            "one valid slot plus empty backup must be OK"))
        return 1;

    /* Signed-but-bad backup means recoverable DEGRADED, not EMPTY. */
    put32(
        image
            + 14u * REMASTER_EMERALD_SECTOR_BYTES
            + FOOTER_SIGNATURE_OFFSET,
        kSignature);
    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_DEGRADED,
            "valid slot plus corrupt signed backup must be DEGRADED"))
        return 1;

    memset(image, 0xFF, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    put32(image + FOOTER_SIGNATURE_OFFSET, kSignature);
    put32(
        image
            + 14u * REMASTER_EMERALD_SECTOR_BYTES
            + FOOTER_SIGNATURE_OFFSET,
        kSignature);
    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_CORRUPT,
            "two corrupt signed slots must be CORRUPT"))
        return 1;

    /*
     * Emerald treats counter 0 as newer than UINT32_MAX after wrap.
     * Test both physical slot orientations.
     */
    memset(image, 0xFF, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    build_slot(image, 0, UINT32_MAX, 2, &payload);
    build_slot(image, 1, 0, 8, &payload);
    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_OK
            && decoded.counter == 0u
            && decoded.selected_slot == 1u
            && decoded.last_written_sector == 8u,
            "wrapped counter did not select counter 0"))
        return 1;

    memset(image, 0xFF, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    build_slot(image, 0, 0, 5, &payload);
    build_slot(image, 1, UINT32_MAX, 11, &payload);
    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_OK
            && decoded.counter == 0u
            && decoded.selected_slot == 0u
            && decoded.last_written_sector == 5u,
            "wrapped counter orientation changed selection"))
        return 1;

    /* Writer must wrap counter and rotation exactly like Emerald. */
    memset(image, 0xFF, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    image[special_offset] = 0x6D;
    payload.counter = UINT32_MAX;
    payload.last_written_sector = REMASTER_EMERALD_MAIN_SECTORS - 1u;
    payload.selected_slot = 1u;
    payload.status = REMASTER_EMERALD_SAVE_OK;

    if (!check(
            remaster_emerald_save_encode_next(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &payload),
            "wrapped encode failed"))
        return 1;

    if (!check(
            payload.counter == 0u
            && payload.last_written_sector == 0u
            && payload.selected_slot == 0u,
            "wrapped encode metadata mismatch"))
        return 1;

    if (!check(
            image[special_offset] == 0x6D,
            "normal write modified special sector 31"))
        return 1;

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_OK
            && decoded.counter == 0u
            && decoded.selected_slot == 0u,
            "wrapped encoded image did not reload"))
        return 1;

    free(image);
    puts("R17 save edge matrix passed.");
    return 0;
}
