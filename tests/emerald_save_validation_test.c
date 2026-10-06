#include "remaster/emerald_save.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Independent fixture geometry: compiled AGBCC evidence is recorded in
 * docs/R17_SAVE_COMPAT_CONTRACT.md. Do not derive checksums from core constants
 * or call the production checksum when constructing these fixtures. */
enum {
    IMAGE_BYTES = 0x20000,
    SECTOR_BYTES = 0x1000,
    SLOT_SECTORS = 14,
    ID_OFFSET = 0xFF4,
    CHECKSUM_OFFSET = 0xFF6,
    SIGNATURE_OFFSET = 0xFF8,
    COUNTER_OFFSET = 0xFFC
};

static const size_t kVanillaPlusSizes[14] = {
    0xF44, 0xF80, 0xF80, 0xF80, 0xF48,
    0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0
};
static const size_t kStockSizes[14] = {
    0xF2C, 0xF80, 0xF80, 0xF80, 0xF08,
    0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0
};
static unsigned checks;
static unsigned failures;

static void check(int condition, const char *name)
{
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "r17_save_sector_validation: %s\n", name);
    }
}

static void put16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static void put32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

static uint16_t fixture_checksum(const uint8_t *data, size_t size)
{
    size_t i;
    uint32_t sum = 0;
    for (i = 0; i < size; i += 4u) {
        sum += (uint32_t)data[i]
            | ((uint32_t)data[i + 1u] << 8u)
            | ((uint32_t)data[i + 2u] << 16u)
            | ((uint32_t)data[i + 3u] << 24u);
    }
    return (uint16_t)((sum & 0xFFFFu) + (sum >> 16u));
}

static uint8_t *physical_sector(uint8_t *image, unsigned slot, unsigned physical)
{
    return image + (slot * SLOT_SECTORS + physical) * SECTOR_BYTES;
}

static uint8_t *logical_sector(
    uint8_t *image, unsigned slot, unsigned rotation, unsigned id)
{
    return physical_sector(image, slot, (id + rotation) % SLOT_SECTORS);
}

static uint8_t fixture_byte(unsigned id, size_t offset, unsigned marker)
{
    return (uint8_t)(id * 41u + offset * 19u + (offset >> 7u) + marker);
}

static void build_slot(
    uint8_t *image, const size_t *sizes, unsigned slot,
    uint32_t counter, unsigned rotation, unsigned marker)
{
    unsigned id;
    for (id = 0; id < SLOT_SECTORS; ++id) {
        uint8_t *sector = logical_sector(image, slot, rotation, id);
        size_t i;
        memset(sector, 0xA5, SECTOR_BYTES);
        for (i = 0; i < sizes[id]; ++i)
            sector[i] = fixture_byte(id, i, marker);
        put16(sector + ID_OFFSET, (uint16_t)id);
        put16(sector + CHECKSUM_OFFSET, fixture_checksum(sector, sizes[id]));
        put32(sector + SIGNATURE_OFFSET, UINT32_C(0x08012025));
        put32(sector + COUNTER_OFFSET, counter);
    }
}

static int payload_matches(const RemasterEmeraldSave *save, unsigned marker)
{
    unsigned id;
    for (id = 0; id < SLOT_SECTORS; ++id) {
        const uint8_t *data;
        size_t i;
        if (id == 0)
            data = save->save_block2;
        else if (id <= 4)
            data = save->save_block1 + (id - 1u) * 0xF80u;
        else
            data = save->pokemon_storage + (id - 5u) * 0xF80u;
        for (i = 0; i < kVanillaPlusSizes[id]; ++i) {
            if (data[i] != fixture_byte(id, i, marker))
                return 0;
        }
    }
    return 1;
}

static void test_existing_decoder(uint8_t *image)
{
    RemasterEmeraldSave save;
    unsigned slot;
    unsigned rotation;
    unsigned physical;
    static const uint8_t checksum_vector[12] = {
        0xFF, 0xFF, 0xFF, 0xFF, 1, 0, 0, 0, 2, 0, 1, 0
    };

    check(REMASTER_EMERALD_SAVE_BLOCK2_BYTES == 0xF44
        && REMASTER_EMERALD_SAVE_BLOCK1_BYTES == 0x3DC8
        && REMASTER_EMERALD_STORAGE_BYTES == 0x83D0,
        "keep compiled pinned Vanilla+ geometry");
    check(fixture_checksum(checksum_vector, sizeof(checksum_vector)) == 3
        && remaster_emerald_checksum(checksum_vector, sizeof(checksum_vector)) == 3,
        "32-bit checksum overflow and 16-bit fold golden vector");

    for (slot = 0; slot < 2; ++slot) {
        for (rotation = 0; rotation < SLOT_SECTORS; ++rotation) {
            memset(image, 0xFF, IMAGE_BYTES);
            build_slot(image, kVanillaPlusSizes, slot, 40u + slot, rotation, 7);
            check(remaster_emerald_save_decode(image, IMAGE_BYTES, &save)
                    == REMASTER_EMERALD_SAVE_OK
                && save.selected_slot == slot && save.counter == 40u + slot
                && save.last_written_sector == rotation,
                "all physical rotations and either standalone slot");
            check(payload_matches(&save, 7),
                "every selected logical payload byte reconstructed in order");
        }
    }

    for (physical = 0; physical < SLOT_SECTORS; ++physical) {
        memset(image, 0xFF, IMAGE_BYTES);
        build_slot(image, kVanillaPlusSizes, 0, 8, 3, 7);
        build_slot(image, kVanillaPlusSizes, 1, 9, 6, 91);
        put32(physical_sector(image, 1, physical) + COUNTER_OFFSET, 10);
        check(remaster_emerald_save_decode(image, IMAGE_BYTES, &save)
                == REMASTER_EMERALD_SAVE_DEGRADED
            && save.selected_slot == 0 && save.counter == 8,
            "mixed counter at any physical position requires valid backup");
        check(payload_matches(&save, 7),
            "mixed-counter recovery must never reconstruct a hybrid payload");

        memset(image, 0xFF, IMAGE_BYTES);
        build_slot(image, kVanillaPlusSizes, 1, 9, 6, 91);
        put32(physical_sector(image, 1, physical) + COUNTER_OFFSET, 10);
        check(remaster_emerald_save_decode(image, IMAGE_BYTES, &save)
                == REMASTER_EMERALD_SAVE_CORRUPT,
            "mixed-counter standalone slot must be rejected");
    }
}

static void expect_validation(
    const uint8_t *image, RemasterEmeraldSaveFormat format,
    RemasterEmeraldSaveStatus status, unsigned slot, uint32_t counter,
    unsigned rotation, const char *name)
{
    RemasterEmeraldSaveValidation result;
    RemasterEmeraldSaveStatus actual;
    memset(&result, 0xA5, sizeof(result));
    actual = remaster_emerald_save_validate(image, IMAGE_BYTES, format, &result);
    check(actual == status && result.status == status && result.format == format
        && ((status != REMASTER_EMERALD_SAVE_OK
                && status != REMASTER_EMERALD_SAVE_DEGRADED)
            || (result.selected_slot == slot && result.counter == counter
                && result.last_written_sector == rotation)), name);
}

static void test_format_validation(
    uint8_t *image, const size_t *sizes, RemasterEmeraldSaveFormat format)
{
    unsigned slot;
    unsigned rotation;
    unsigned physical;
    unsigned variant;
    static const unsigned tail_ids[] = {0, 4, 13};
    static const uint32_t counters[][2] = {
        {8, 9}, {10, 9}, {UINT32_MAX, 0}, {0, UINT32_MAX},
        {17, 17}, {UINT32_MAX, 1}, {0, UINT32_MAX - 1u}, {1, UINT32_C(0x80000000)}
    };
    static const unsigned selected[] = {1, 0, 1, 0, 0, 0, 1, 1};

    for (slot = 0; slot < 2; ++slot) {
        for (rotation = 0; rotation < SLOT_SECTORS; ++rotation) {
            memset(image, 0xFF, IMAGE_BYTES);
            build_slot(image, sizes, slot, 40u + slot, rotation, 7);
            expect_validation(image, format, REMASTER_EMERALD_SAVE_OK,
                slot, 40u + slot, rotation, "explicit-format slot rotation matrix");
        }
    }
    for (variant = 0; variant < sizeof(selected) / sizeof(selected[0]); ++variant) {
        memset(image, 0xFF, IMAGE_BYTES);
        build_slot(image, sizes, 0, counters[variant][0], 3, 7);
        build_slot(image, sizes, 1, counters[variant][1], 6, 91);
        expect_validation(image, format, REMASTER_EMERALD_SAVE_OK,
            selected[variant], counters[variant][selected[variant]],
            selected[variant] == 0 ? 3 : 6,
            "counter ordering, exact MAX-to-zero wrap and existing slot-zero tie");
    }

    for (physical = 0; physical < SLOT_SECTORS; ++physical) {
        memset(image, 0xFF, IMAGE_BYTES);
        build_slot(image, sizes, 0, 8, 3, 7);
        build_slot(image, sizes, 1, 9, 6, 91);
        put32(physical_sector(image, 1, physical) + COUNTER_OFFSET, 10);
        expect_validation(image, format, REMASTER_EMERALD_SAVE_DEGRADED,
            0, 8, 3, "format-aware mixed-counter backup recovery");
        memset(image, 0xFF, SLOT_SECTORS * SECTOR_BYTES);
        expect_validation(image, format, REMASTER_EMERALD_SAVE_CORRUPT,
            0, 0, 0, "format-aware mixed-counter standalone rejection");
    }

    /* For every short section, mutate just inside/outside its checksum span. */
    for (variant = 0; variant < sizeof(tail_ids) / sizeof(tail_ids[0]); ++variant) {
        unsigned id = tail_ids[variant];
        uint8_t *sector;
        memset(image, 0xFF, IMAGE_BYTES);
        build_slot(image, sizes, 0, 8, 3, 7);
        build_slot(image, sizes, 1, 9, 6, 91);
        sector = logical_sector(image, 1, 6, id);
        sector[sizes[id] - 1u] ^= 1u;
        expect_validation(image, format, REMASTER_EMERALD_SAVE_DEGRADED,
            0, 8, 3, "last logical payload byte is checksum-covered");
        sector[sizes[id] - 1u] ^= 1u;
        sector[sizes[id]] ^= 1u;
        expect_validation(image, format, REMASTER_EMERALD_SAVE_OK,
            1, 9, 6, "first padding byte is outside checksum span");
    }

    /* ID bounds, duplicate/missing IDs, signature and checksum corruption. */
    for (variant = 0; variant < 5; ++variant) {
        uint8_t *sector;
        memset(image, 0xFF, IMAGE_BYTES);
        build_slot(image, sizes, 0, 8, 3, 7);
        build_slot(image, sizes, 1, 9, 6, 91);
        sector = logical_sector(image, 1, 6, 13);
        if (variant < 2)
            put16(sector + ID_OFFSET, variant == 0 ? 14 : UINT16_MAX);
        else if (variant == 2) {
            put16(sector + ID_OFFSET, 1);
            put16(sector + CHECKSUM_OFFSET, fixture_checksum(sector, sizes[1]));
        } else if (variant == 3)
            put32(sector + SIGNATURE_OFFSET, UINT32_C(0x08012024));
        else
            sector[0] ^= 1u;
        expect_validation(image, format, REMASTER_EMERALD_SAVE_DEGRADED,
            0, 8, 3, "damaged or missing section selects complete backup");
        memset(image, 0xFF, SLOT_SECTORS * SECTOR_BYTES);
        expect_validation(image, format, REMASTER_EMERALD_SAVE_CORRUPT,
            0, 0, 0, "damaged standalone slot is incomplete/corrupt");
    }

    memset(image, 0xFF, IMAGE_BYTES);
    build_slot(image, sizes, 0, 8, 3, 7);
    build_slot(image, sizes, 1, 9, 6, 91);
    for (variant = 0; variant < SLOT_SECTORS; ++variant) {
        slot = variant < 7 ? 0 : 1;
        rotation = slot == 0 ? 3 : 6;
        put32(logical_sector(image, slot, rotation, variant) + SIGNATURE_OFFSET, 0);
    }
    expect_validation(image, format, REMASTER_EMERALD_SAVE_CORRUPT,
        0, 0, 0, "complementary partial slots cannot be combined");

    /* Physical order need not be canonical: IDs, not array position, own data. */
    memset(image, 0xFF, IMAGE_BYTES);
    build_slot(image, sizes, 0, 8, 3, 7);
    {
        uint8_t temp[SECTOR_BYTES];
        uint8_t *a = logical_sector(image, 0, 3, 0);
        uint8_t *b = logical_sector(image, 0, 3, 5);
        memcpy(temp, a, sizeof(temp));
        memcpy(a, b, sizeof(temp));
        memcpy(b, temp, sizeof(temp));
    }
    expect_validation(image, format, REMASTER_EMERALD_SAVE_OK,
        0, 8, 8, "section ID controls ownership; ID zero position records rotation");

    /* Hall of Fame/Trainer Hill/Recorded Battle bytes never affect probing. */
    for (physical = 28; physical < 32; ++physical) {
        uint8_t *sector = image + physical * SECTOR_BYTES;
        memset(sector, 0, SECTOR_BYTES);
        put16(sector + ID_OFFSET, 0);
        put32(sector + SIGNATURE_OFFSET, UINT32_C(0x08012025));
        put32(sector + COUNTER_OFFSET, UINT32_MAX);
    }
    expect_validation(image, format, REMASTER_EMERALD_SAVE_OK,
        0, 8, 8, "special-sector footers cannot become main-slot candidates");
}

static void test_format_boundary(uint8_t *image)
{
    RemasterEmeraldSaveValidation result;
    uint8_t *original = (uint8_t *)malloc(IMAGE_BYTES);
    unsigned id;
    if (original == 0) {
        check(0, "read-only snapshot allocation");
        return;
    }
    check(REMASTER_EMERALD_STOCK_SAVE_BLOCK2_BYTES == 0xF2C
        && REMASTER_EMERALD_STOCK_SAVE_BLOCK1_BYTES == 0x3D88,
        "compiled stock geometry is separate from pinned Vanilla+");
    memset(image, 0xFF, IMAGE_BYTES);
    build_slot(image, kStockSizes, 0, 8, 3, 7);
    build_slot(image, kVanillaPlusSizes, 1, 9, 6, 91);
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_STOCK,
        REMASTER_EMERALD_SAVE_DEGRADED, 0, 8, 3,
        "stock request cannot select newer slot from another layout");
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,
        REMASTER_EMERALD_SAVE_DEGRADED, 1, 9, 6,
        "Vanilla+ request cannot reconstruct older slot from another layout");

    /* A candidate must use one layout for all its sections. */
    memset(image, 0xFF, IMAGE_BYTES);
    build_slot(image, kStockSizes, 0, 8, 3, 7);
    {
        uint8_t *sector = logical_sector(image, 0, 3, 4);
        size_t i;
        for (i = 0; i < kVanillaPlusSizes[4]; ++i)
            sector[i] = fixture_byte(4, i, 91);
        put16(sector + CHECKSUM_OFFSET,
            fixture_checksum(sector, kVanillaPlusSizes[4]));
    }
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_STOCK,
        REMASTER_EMERALD_SAVE_CORRUPT, 0, 0, 0,
        "stock cannot accept a slot with Vanilla+ section four");
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,
        REMASTER_EMERALD_SAVE_CORRUPT, 0, 0, 0,
        "Vanilla+ cannot accept a slot with stock section zero");

    memset(image, 0xFF, IMAGE_BYTES);
    build_slot(image, kStockSizes, 0, 8, 3, 7);
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_STOCK,
        REMASTER_EMERALD_SAVE_OK, 0, 8, 3, "stock checksum geometry accepted");
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,
        REMASTER_EMERALD_SAVE_CORRUPT, 0, 0, 0,
        "nonzero stock padding must not be checksummed as Vanilla+ payload");

    memset(image, 0xFF, IMAGE_BYTES);
    build_slot(image, kVanillaPlusSizes, 0, 8, 3, 7);
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_STOCK,
        REMASTER_EMERALD_SAVE_CORRUPT, 0, 0, 0,
        "nonzero Vanilla+ extensions must not be ignored as stock padding");

    memset(image, 0xFF, IMAGE_BYTES);
    build_slot(image, kStockSizes, 0, 8, 3, 7);
    for (id = 0; id <= 4; id += 4u) {
        memset(logical_sector(image, 0, 3, id) + kStockSizes[id], 0,
            kVanillaPlusSizes[id] - kStockSizes[id]);
    }
    memcpy(original, image, IMAGE_BYTES);
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_STOCK,
        REMASTER_EMERALD_SAVE_OK, 0, 8, 3, "ambiguous checksum fixture: explicit stock");
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,
        REMASTER_EMERALD_SAVE_OK, 0, 8, 3, "ambiguous checksum fixture: explicit Vanilla+");
    check(memcmp(image, original, IMAGE_BYTES) == 0, "validation is read-only");

    memset(&result, 0xA5, sizeof(result));
    check(remaster_emerald_save_validate(image, IMAGE_BYTES,
            (RemasterEmeraldSaveFormat)99, &result) == REMASTER_EMERALD_SAVE_CORRUPT
        && result.status == REMASTER_EMERALD_SAVE_CORRUPT
        && result.counter == 0, "unknown format cannot silently select a layout");
    check(remaster_emerald_save_validate(image, IMAGE_BYTES - 1u,
            REMASTER_EMERALD_SAVE_FORMAT_STOCK, &result) == REMASTER_EMERALD_SAVE_CORRUPT,
        "short image rejected before sector access");
    check(remaster_emerald_save_validate(image, IMAGE_BYTES + 1u,
            REMASTER_EMERALD_SAVE_FORMAT_STOCK, &result) == REMASTER_EMERALD_SAVE_CORRUPT,
        "oversize image rejected before sector access");
    check(remaster_emerald_save_validate(NULL, IMAGE_BYTES,
            REMASTER_EMERALD_SAVE_FORMAT_STOCK, &result) == REMASTER_EMERALD_SAVE_CORRUPT,
        "null image rejected");
    check(remaster_emerald_save_validate(image, IMAGE_BYTES,
            REMASTER_EMERALD_SAVE_FORMAT_STOCK, NULL) == REMASTER_EMERALD_SAVE_CORRUPT,
        "null validation result rejected");
    memset(image, 0xFF, IMAGE_BYTES);
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_STOCK,
        REMASTER_EMERALD_SAVE_EMPTY, 0, 0, 0, "erased stock image is empty");
    expect_validation(image, REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,
        REMASTER_EMERALD_SAVE_EMPTY, 0, 0, 0, "erased Vanilla+ image is empty");
    free(original);
}

int main(void)
{
    uint8_t *image = (uint8_t *)malloc(IMAGE_BYTES);
    if (image == 0)
        return 1;
    test_existing_decoder(image);
    test_format_validation(image, kStockSizes, REMASTER_EMERALD_SAVE_FORMAT_STOCK);
    test_format_validation(image, kVanillaPlusSizes, REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS);
    test_format_boundary(image);
    free(image);
    printf("R17 sector validation: %u checks, %u failures\n", checks, failures);
    return failures != 0;
}
