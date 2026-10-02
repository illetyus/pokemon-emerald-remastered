#include "remaster/emerald_save.h"

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

    for (id = 0; id < 14; ++id) {
        const uint16_t physical = (uint16_t)((id + rotation) % 14u);
        uint8_t *sector =
            image
            + ((size_t)slot * 14u + physical)
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
        fprintf(stderr, "emerald_save_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    uint8_t *image =
        (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    RemasterEmeraldSave source;
    RemasterEmeraldSave decoded;
    RemasterEmeraldSave redecoded;
    RemasterEmeraldTime time;

    if (image == 0)
        return 1;

    memset(image, 0xff, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    memset(&source, 0, sizeof(source));

    source.save_block2[0] = 0x11;
    source.save_block1[0] = 0x22;
    source.save_block1[REMASTER_EMERALD_SAVE_BLOCK1_BYTES - 1u] = 0x23;
    source.pokemon_storage[0] = 0x33;
    source.pokemon_storage[REMASTER_EMERALD_STORAGE_BYTES - 1u] = 0x34;

    time.days = 321;
    time.hours = 5;
    time.minutes = 42;
    time.seconds = 17;
    remaster_emerald_save_set_local_time_offset(&source, time);

    time.days = 654;
    time.hours = 11;
    time.minutes = 9;
    time.seconds = 8;
    remaster_emerald_save_set_last_berry_update(&source, time);

    build_slot(image, 0, 8, 3, &source);
    build_slot(image, 1, 9, 6, &source);

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_OK,
            "two valid slots should decode"))
        return 1;

    if (!check(decoded.selected_slot == 1, "newest slot not selected"))
        return 1;
    if (!check(decoded.counter == 9, "counter mismatch"))
        return 1;
    if (!check(decoded.last_written_sector == 6, "rotation mismatch"))
        return 1;
    if (!check(decoded.save_block2[0] == 0x11, "SaveBlock2 mismatch"))
        return 1;
    if (!check(decoded.save_block1[0] == 0x22, "SaveBlock1 start mismatch"))
        return 1;
    if (!check(
            decoded.save_block1[REMASTER_EMERALD_SAVE_BLOCK1_BYTES - 1u] == 0x23,
            "SaveBlock1 tail mismatch"))
        return 1;
    if (!check(decoded.pokemon_storage[0] == 0x33, "storage start mismatch"))
        return 1;
    if (!check(
            decoded.pokemon_storage[REMASTER_EMERALD_STORAGE_BYTES - 1u] == 0x34,
            "storage tail mismatch"))
        return 1;

    time = remaster_emerald_save_get_local_time_offset(&decoded);
    if (!check(
            time.days == 321 && time.hours == 5
            && time.minutes == 42 && time.seconds == 17,
            "localTimeOffset mismatch"))
        return 1;

    time = remaster_emerald_save_get_last_berry_update(&decoded);
    if (!check(
            time.days == 654 && time.hours == 11
            && time.minutes == 9 && time.seconds == 8,
            "lastBerryTreeUpdate mismatch"))
        return 1;

    /* Damage newest slot sector id 4; decoder must fall back to older slot. */
    image[(14u + ((4u + 6u) % 14u)) * REMASTER_EMERALD_SECTOR_BYTES] ^= 0x80u;

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded) == REMASTER_EMERALD_SAVE_DEGRADED,
            "damaged newest slot should use valid backup"))
        return 1;
    if (!check(decoded.selected_slot == 0 && decoded.counter == 8, "backup selection mismatch"))
        return 1;

    /* Writing must increment counter, rotate, select parity slot and reload. */
    if (!check(
            remaster_emerald_save_encode_next(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &decoded),
            "encode_next failed"))
        return 1;

    if (!check(decoded.counter == 9, "encode counter did not increment"))
        return 1;
    if (!check(decoded.last_written_sector == 4, "encode rotation did not advance"))
        return 1;
    if (!check(decoded.selected_slot == 1, "encode slot parity mismatch"))
        return 1;

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &redecoded) == REMASTER_EMERALD_SAVE_OK,
            "rewritten image did not decode cleanly"))
        return 1;
    if (!check(redecoded.counter == 9 && redecoded.selected_slot == 1, "rewrite reload mismatch"))
        return 1;

    free(image);
    puts("Emerald save compatibility test passed.");
    return 0;
}
