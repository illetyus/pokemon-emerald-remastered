#include "remaster/emerald_save.h"
#include "remaster/platform.h"

#include <stdio.h>
#include <string.h>

/* Independent production-layout fixtures: no encode API or core checksum is
 * used to construct the image that is imported before each export. */
static unsigned checks, failures;
static uint8_t image[131072], before[131072], scratch[131072], failed_image[131072];
static RemasterEmeraldSave live, expected, imported, before_save;
static void check(int ok, const char *what)
{
    ++checks;
    if (!ok) { ++failures; fprintf(stderr, "export: %s\n", what); }
}
static uint16_t get16(const uint8_t *p)
{ return (uint16_t)((unsigned)p[0] | ((unsigned)p[1] << 8)); }
static uint32_t get32(const uint8_t *p)
{ return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16); }
static void put16(uint8_t *p, uint16_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v)
{ put16(p, (uint16_t)v); put16(p + 2, (uint16_t)(v >> 16)); }
static uint16_t sum(const uint8_t *p, size_t n)
{
    uint32_t value = 0; size_t i;
    for (i = 0; i < n; i += 4) value += get32(p + i);
    return (uint16_t)((value >> 16) + value);
}
static size_t span(unsigned id, int stock)
{
    if (id == 0) return stock ? 0xF2Cu : 0xF44u;
    if (id == 4) return stock ? 0xF08u : 0xF48u;
    return id == 13 ? 0x7D0u : 0xF80u;
}
static uint8_t *payload(RemasterEmeraldSave *save, unsigned id)
{
    if (id == 0) return save->save_block2;
    if (id <= 4) return save->save_block1 + (id - 1u) * 0xF80u;
    return save->pokemon_storage + (id - 5u) * 0xF80u;
}
static void seed_payload(int stock)
{
    size_t i;
    memset(&expected, 0, sizeof(expected));
    for (i = 0; i < (stock ? 0xF2Cu : 0xF44u); ++i)
        expected.save_block2[i] = (uint8_t)(i * 13u + 17u);
    for (i = 0; i < (stock ? 0x3D88u : 0x3DC8u); ++i)
        expected.save_block1[i] = (uint8_t)(i * 19u + 31u);
    for (i = 0; i < 0x83D0u; ++i)
        expected.pokemon_storage[i] = (uint8_t)(i * 23u + 47u);
    /* Source VP5 and its neighboring bytes must be opaque to export. */
    if (!stock) {
        put32(expected.save_block1 + 0x35D8, UINT32_C(0x35504956));
        expected.save_block1[0x35DC] = 1;
    }
}
static void slot(unsigned slot_id, uint32_t counter, unsigned rotation, int stock, int older)
{
    unsigned id; size_t i;
    for (id = 0; id < 14; ++id) {
        uint8_t *s = image + (slot_id * 14u + (id + rotation) % 14u) * 4096u;
        size_t n = span(id, stock);
        memset(s, 0xC7, 4096); /* Deliberately nonzero out-of-checksum padding. */
        memcpy(s, payload(&expected, id), n);
        if (older) for (i = 0; i < n; ++i) s[i] ^= 0x21u;
        put16(s + 4084, (uint16_t)id); put16(s + 4086, sum(s, n));
        put32(s + 4088, UINT32_C(0x08012025)); put32(s + 4092, counter);
    }
}
static void fixture(int stock, uint32_t counter, unsigned rotation)
{
    size_t i; unsigned selected = (unsigned)(counter % 2u);
    seed_payload(stock);
    for (i = 0; i < sizeof(image); ++i) image[i] = (uint8_t)(i * 7u + 9u);
    slot(selected, counter, rotation, stock, 0);
    slot(selected ^ 1u, counter - 1u, (rotation + 8u) % 14u, stock, 1);
    check(remaster_emerald_save_decode_format(image, sizeof(image),
        stock ? REMASTER_EMERALD_SAVE_FORMAT_STOCK : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,
        &live) == REMASTER_EMERALD_SAVE_OK, "fixture imports both source layouts");
    check(live.counter == counter && live.selected_slot == selected
        && live.last_written_sector == rotation, "fixture selects current source slot");
    check(!memcmp(live.save_block1, expected.save_block1, sizeof(expected.save_block1))
        && !memcmp(live.save_block2, expected.save_block2, sizeof(expected.save_block2))
        && !memcmp(live.pokemon_storage, expected.pokemon_storage, sizeof(expected.pokemon_storage)),
        "initial logical blocks match independent payload");
}
static void verify_export(int stock, uint32_t counter, unsigned rotation)
{
    unsigned id; size_t i;
    unsigned target = (unsigned)(counter % 2u);
    check(!memcmp(image + (target ^ 1u) * 14u * 4096u,
        before + (target ^ 1u) * 14u * 4096u, 14u * 4096u), "non-written main slot unchanged");
    check(!memcmp(image + 28u * 4096u, before + 28u * 4096u, 4u * 4096u),
        "all special sectors 28-31 preserved byte for byte");
    for (id = 0; id < 14; ++id) {
        const uint8_t *s = image + (target * 14u + (id + rotation) % 14u) * 4096u;
        size_t n = span(id, stock); int padding_zero = 1;
        check(get16(s + 4084) == id && get32(s + 4088) == UINT32_C(0x08012025)
            && get32(s + 4092) == counter, "source footer ID/signature/counter and rotation");
        check(!memcmp(s, payload(&live, id), n) && get16(s + 4086) == sum(s, n),
            "source-format payload spans and independent checksum");
        for (i = n; i < 4084; ++i) if (s[i] != 0) padding_zero = 0;
        check(padding_zero, "written sector padding canonicalized like HandleWriteSector");
    }
    check(remaster_emerald_save_decode_format(image, sizeof(image),
        stock ? REMASTER_EMERALD_SAVE_FORMAT_STOCK : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,
        &imported) == REMASTER_EMERALD_SAVE_OK, "exported image reimports");
    check(imported.counter == counter && imported.selected_slot == target
        && imported.last_written_sector == rotation && imported.source_is_stock == (unsigned)stock,
        "round-trip metadata and runtime source format");
    check(!memcmp(imported.save_block1, live.save_block1, sizeof(live.save_block1))
        && !memcmp(imported.save_block2, live.save_block2, sizeof(live.save_block2))
        && !memcmp(imported.pokemon_storage, live.pokemon_storage, sizeof(live.pokemon_storage)),
        "all logical block bytes survive export without migration");
}
static void roundtrips(void)
{
    const uint32_t counters[] = {0, 1, 100, 101, UINT32_MAX - 1u, UINT32_MAX};
    unsigned stock, rotation, c, pass;
    for (stock = 0; stock < 2; ++stock) for (c = 0; c < 6; ++c)
        for (rotation = 0; rotation < 14; ++rotation) {
            fixture((int)stock, counters[c], rotation);
            for (pass = 0; pass < 3; ++pass) {
                uint32_t next = live.counter + 1u;
                unsigned next_rotation = (live.last_written_sector + 1u) % 14u;
                memcpy(before, image, sizeof(image));
                memcpy(&before_save, &live, sizeof(live));
                check(remaster_emerald_save_encode_next(image, sizeof(image), &live), "encode both source formats");
                check(live.counter == next && live.selected_slot == next % 2u
                    && live.last_written_sector == next_rotation && live.status == REMASTER_EMERALD_SAVE_OK,
                    "successful image export advances metadata exactly once");
                check(!memcmp(live.save_block1, before_save.save_block1, sizeof(live.save_block1))
                    && !memcmp(live.save_block2, before_save.save_block2, sizeof(live.save_block2))
                    && !memcmp(live.pokemon_storage, before_save.pokemon_storage, sizeof(live.pokemon_storage)),
                    "encode does not transform logical domains or VP5 neighbors");
                verify_export((int)stock, next, next_rotation);
            }
        }
}
typedef struct MemoryFile { int exists, fail, reads, writes; } MemoryFile;
static MemoryFile memory;
static RemasterSaveReadResult read_file(void *user, const char *name, uint8_t *buffer, size_t capacity, size_t *size)
{
    MemoryFile *file = (MemoryFile *)user;
    (void)name; ++file->reads;
    if (!file->exists) return REMASTER_SAVE_READ_MISSING;
    if (capacity < sizeof(image)) return REMASTER_SAVE_READ_ERROR;
    memcpy(buffer, image, sizeof(image)); *size = sizeof(image); return REMASTER_SAVE_READ_OK;
}
static int write_file(void *user, const char *name, const uint8_t *buffer, size_t size)
{
    MemoryFile *file = (MemoryFile *)user;
    (void)name; ++file->writes;
    check(size == sizeof(image), "write submits entire compatible image");
    check(!memcmp(&live, &before_save, sizeof(live)), "callback observes pre-commit runtime metadata");
    if (file->fail) return 0;
    memcpy(image, buffer, size); file->exists = 1; return 1;
}
static void transactions(void)
{
    RemasterPlatformVTable platform;
    unsigned stock, c, failure;
    const uint32_t counters[] = {10, UINT32_MAX};
    memset(&platform, 0, sizeof(platform)); platform.userdata = &memory;
    platform.save_read_result = read_file; platform.save_write = write_file;
    remaster_platform_install(&platform);
    for (stock = 0; stock < 2; ++stock) for (c = 0; c < 2; ++c) {
        uint32_t next; unsigned rotation;
        fixture((int)stock, counters[c], 13);
        /* Keep DEGRADED caller status and pending domain edits across failure. */
        live.status = REMASTER_EMERALD_SAVE_DEGRADED;
        live.save_block1[123] ^= 0x81u;
        memcpy(&before_save, &live, sizeof(live)); memcpy(before, image, sizeof(image));
        memset(&memory, 0, sizeof(memory)); memory.exists = 1; memory.fail = 1;
        for (failure = 0; failure < 3; ++failure) {
            check(!remaster_emerald_save_store_platform("source", scratch, sizeof(scratch), &live), "failed callback returns failure");
            check(!memcmp(&live, &before_save, sizeof(live)), "failed write leaves complete caller wrapper unchanged");
            check(!memcmp(image, before, sizeof(image)), "rejected mock write retains committed image");
            if (failure == 0) memcpy(failed_image, scratch, sizeof(scratch));
            else check(!memcmp(failed_image, scratch, sizeof(scratch)), "retries prepare identical image/counter/rotation");
        }
        memory.fail = 0; next = live.counter + 1u; rotation = (live.last_written_sector + 1u) % 14u;
        check(remaster_emerald_save_store_platform("source", scratch, sizeof(scratch), &live), "retry succeeds for each source format");
        check(live.counter == next && live.last_written_sector == rotation
            && live.selected_slot == next % 2u && live.status == REMASTER_EMERALD_SAVE_OK,
            "successful callback commits metadata once");
        check(memory.writes == 4 && memory.reads == 4, "one read/write per attempt");
        verify_export((int)stock, next, rotation);
    }
    /* Failure before the first successful image creation also cannot advance state. */
    memset(&live, 0, sizeof(live)); memcpy(&before_save, &live, sizeof(live));
    memset(&memory, 0, sizeof(memory)); memory.fail = 1;
    check(!remaster_emerald_save_store_platform("new", scratch, sizeof(scratch), &live)
        && !memcmp(&live, &before_save, sizeof(live)) && !memory.exists, "failed first write keeps EMPTY metadata");
    memory.fail = 0;
    check(remaster_emerald_save_store_platform("new", scratch, sizeof(scratch), &live)
        && live.counter == 1 && live.selected_slot == 1, "first successful creation counter is one");
    remaster_platform_install(0);
}
static void rejected_arguments(void)
{
    fixture(0, 100, 1); memcpy(before, image, sizeof(image)); memcpy(&before_save, &live, sizeof(live));
    check(!remaster_emerald_save_encode_next(image, sizeof(image) - 1u, &live), "reject short export image");
    check(!remaster_emerald_save_encode_next(0, sizeof(image), &live), "reject null image");
    check(!remaster_emerald_save_encode_next(image, sizeof(image), 0), "reject null save");
    check(!memcmp(&live, &before_save, sizeof(live)) && !memcmp(image, before, sizeof(image)), "invalid parameters mutate nothing");
    check(!remaster_emerald_save_store_platform("source", scratch, sizeof(scratch), &live)
        && !memcmp(&live, &before_save, sizeof(live)), "missing platform rejects without metadata advance");
}
int main(void)
{
    roundtrips(); transactions(); rejected_arguments();
    printf("R17 export/transaction regression: %u checks, %u failures.\n", checks, failures);
    return failures ? 1 : 0;
}
