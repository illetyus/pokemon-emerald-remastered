#include "remaster/emerald_save.h"
#include "remaster/platform.h"

#include <stdio.h>
#include <string.h>

#ifdef R17_IO_BASELINE
typedef enum RemasterSaveReadResult {
    REMASTER_SAVE_READ_OK = 0, REMASTER_SAVE_READ_MISSING = 1, REMASTER_SAVE_READ_ERROR = 2
} RemasterSaveReadResult;
#define REMASTER_EMERALD_SAVE_IO_ERROR ((RemasterEmeraldSaveStatus)4)
#define REMASTER_EMERALD_SAVE_UNSUPPORTED ((RemasterEmeraldSaveStatus)5)
#define REMASTER_EMERALD_SAVE_FORMAT_UNSPECIFIED ((RemasterEmeraldSaveFormat)2)
/* The old API has no explicit format selection and no typed read result. */
static RemasterEmeraldSaveStatus remaster_emerald_save_load_platform_format(
    const char *slot, uint8_t *buffer, size_t size, RemasterEmeraldSaveFormat format,
    RemasterEmeraldSave *save)
{ (void)format; return remaster_emerald_save_load_platform(slot, buffer, size, save); }
#endif

static unsigned checks, failures, reads, legacy_reads, writes;
static int read_kind, fail_write;
static size_t advertised_size;
static uint8_t disk[131072], before_disk[131072], buffer[131074];
static RemasterEmeraldSave save, before_save, loaded;

static void check(int ok, const char *message)
{
    ++checks;
    if (!ok) { if (failures < 20) fprintf(stderr, "save IO: %s\n", message); ++failures; }
}
static unsigned get16(const uint8_t *p)
{ return (unsigned)p[0] | ((unsigned)p[1] << 8); }
static void put16(uint8_t *p, unsigned value)
{ p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t *p, uint32_t value)
{ put16(p, value & 0xFFFFu); put16(p + 2, value >> 16); }
static size_t span(unsigned id, int stock)
{ return id == 0 ? (stock ? 0xF2Cu : 0xF44u) : id == 4 ? (stock ? 0xF08u : 0xF48u) : id == 13 ? 0x7D0u : 3968u; }
static uint16_t checksum(const uint8_t *p, size_t n)
{
    size_t i; uint32_t value = 0;
    for (i = 0; i < n; i += 4) value += (uint32_t)get16(p + i) | ((uint32_t)get16(p + i + 2) << 16);
    return (uint16_t)((value >> 16) + value);
}
static uint8_t *sector(unsigned slot, unsigned id, unsigned rotation)
{ return disk + (slot * 14u + (id + rotation) % 14u) * 4096u; }
static void fixture(int stock)
{
    unsigned slot, id; size_t i;
    for (i = 0; i < sizeof(disk); ++i) disk[i] = (uint8_t)(i * 17u + 13u);
    for (slot = 0; slot < 2; ++slot) for (id = 0; id < 14; ++id) {
        uint8_t *p = sector(slot, id, slot ? 3 : 11); const size_t n = span(id, stock);
        for (i = 0; i < n; ++i) p[i] = (uint8_t)(i * 7u + id * 19u + slot * 31u + 23u);
        if (id == 4) {
            memset(p + 0x758, 0, 21); put32(p + 0x758, UINT32_C(0x35504956)); p[0x75C] = 1;
        }
        put16(p + 4084, id); put16(p + 4086, checksum(p, n));
        put32(p + 4088, UINT32_C(0x08012025)); put32(p + 4092, slot ? 7u : 8u);
    }
    read_kind = REMASTER_SAVE_READ_OK; advertised_size = sizeof(disk);
    reads = legacy_reads = writes = 0; fail_write = 0;
    buffer[0] = 0xA5; buffer[sizeof(buffer) - 1] = 0x5A;
}
static RemasterSaveReadResult typed_read(void *user, const char *slot, uint8_t *p, size_t capacity, size_t *size)
{
    (void)user; (void)slot; ++reads; *size = advertised_size;
    if (read_kind == REMASTER_SAVE_READ_OK && advertised_size <= capacity && advertised_size <= sizeof(disk))
        memcpy(p, disk, advertised_size);
    if (read_kind == REMASTER_SAVE_READ_ERROR) memset(p, 0xEC, 8); /* partial read before failure */
    return (RemasterSaveReadResult)read_kind;
}
static int legacy_read(void *user, const char *slot, uint8_t *p, size_t capacity, size_t *size)
{
    ++legacy_reads;
    return typed_read(user, slot, p, capacity, size) == REMASTER_SAVE_READ_OK ? 1 : 0;
}
static int write_file(void *user, const char *slot, const uint8_t *p, size_t size)
{
    (void)user; (void)slot; ++writes;
    check(size == sizeof(disk), "full image write");
    check(!memcmp(&save, &before_save, sizeof(save)), "write observes uncommitted complete wrapper");
    if (fail_write) return 0;
    memcpy(disk, p, size); advertised_size = size; read_kind = REMASTER_SAVE_READ_OK; return 1;
}
static void install(int typed, int legacy, int writer)
{
    RemasterPlatformVTable v; memset(&v, 0, sizeof(v));
#ifdef R17_IO_BASELINE
    if (typed || legacy) v.save_read = legacy_read;
#else
    if (typed) v.save_read_result = typed_read;
    if (legacy) v.save_read = legacy_read;
#endif
    if (writer) v.save_write = write_file;
    remaster_platform_install(&v);
}
static RemasterEmeraldSaveStatus load(int stock)
{
    return remaster_emerald_save_load_platform_format("fixture", buffer + 1, sizeof(disk),
        stock ? REMASTER_EMERALD_SAVE_FORMAT_STOCK : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS, &loaded);
}
static void current(int stock)
{
    check(load(stock) == REMASTER_EMERALD_SAVE_OK, "explicit source format loads");
    check(loaded.source_is_stock == stock && loaded.counter == 8 && loaded.last_written_sector == 11 && loaded.selected_slot == 0,
        "format provenance and source checkpoint selected");
    save = loaded;
}
static void snapshot(void)
{ before_save = save; memcpy(before_disk, disk, sizeof(disk)); writes = 0; }
static void rejected(void)
{
    snapshot();
    check(!remaster_emerald_save_store_platform("fixture", buffer + 1, sizeof(disk), &save), "unsafe store rejected");
    check(writes == 0, "rejected store never reaches writer");
    check(!memcmp(&save, &before_save, sizeof(save)), "rejected store retains complete wrapper and pending edits");
    check(!memcmp(disk, before_disk, sizeof(disk)), "rejected store retains committed file");
    check(buffer[0] == 0xA5 && buffer[sizeof(buffer) - 1] == 0x5A, "read capacity canaries");
}
static void result_matrix(int stock)
{
    const size_t sizes[] = {0, 1, 65536, 131071, 131073, 262144}; unsigned i;
    fixture(stock); install(1, 1, 1); current(stock);
    check(legacy_reads == 0, "typed callback takes precedence over legacy bool");
    read_kind = REMASTER_SAVE_READ_MISSING;
    check(load(stock) == REMASTER_EMERALD_SAVE_EMPTY && loaded.source_is_stock == stock && loaded.counter == 0,
        "only explicit MISSING initializes correct-format EMPTY");
    rejected(); /* existing loaded game disappearing is not a new-save request */
    for (i = 2; i <= 3; ++i) {
        read_kind = (int)i;
        check(load(stock) == REMASTER_EMERALD_SAVE_IO_ERROR, "ERROR/unknown callback result never means EMPTY");
        rejected();
    }
    read_kind = REMASTER_SAVE_READ_OK;
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        advertised_size = sizes[i];
        check(load(stock) == REMASTER_EMERALD_SAVE_UNSUPPORTED, "wrong-size found file is UNSUPPORTED"); rejected();
    }
    advertised_size = sizeof(disk); install(0, 0, 1);
    check(load(stock) == REMASTER_EMERALD_SAVE_IO_ERROR, "missing reader is IO_ERROR"); rejected();
    remaster_platform_install(0);
    check(load(stock) == REMASTER_EMERALD_SAVE_IO_ERROR, "missing platform is IO_ERROR"); rejected();
    fixture(stock); install(0, 1, 1); current(stock);
    read_kind = REMASTER_SAVE_READ_ERROR;
    check(load(stock) == REMASTER_EMERALD_SAVE_IO_ERROR, "legacy false is ERROR, never MISSING"); rejected();
    read_kind = REMASTER_SAVE_READ_MISSING;
    check(load(stock) == REMASTER_EMERALD_SAVE_IO_ERROR, "ambiguous legacy missing cannot authorize EMPTY"); rejected();
}
static void corrupt_matrix(int stock)
{
    unsigned id, kind, slot;
    for (id = 0; id < 14; ++id) for (kind = 0; kind < 4; ++kind) {
        fixture(stock); install(1, 0, 1); current(stock);
        for (slot = 0; slot < 2; ++slot) {
            uint8_t *p = sector(slot, id, slot ? 3 : 11);
            if (kind == 0) p[0] ^= 0x80; /* checksum */
            else if (kind == 1) put16(p + 4084, 14); /* incomplete ID set */
            else if (kind == 2) put32(p + 4088, 0); /* signature */
            else put32(p + 4092, 42); /* mixed counters */
        }
        check(load(stock) == REMASTER_EMERALD_SAVE_CORRUPT, "no fabricated cross-slot recovery"); rejected();
    }
    for (kind = 0; kind < 3; ++kind) {
        fixture(stock); install(1, 0, 1); current(stock);
        memset(disk, kind == 0 ? 0xFF : kind == 1 ? 0 : 0x77, sizeof(disk));
        check(load(stock) == REMASTER_EMERALD_SAVE_CORRUPT, "found signatureless/blank file never treated as missing"); rejected();
    }
}
static void checkpoint_and_recovery(int stock)
{
    unsigned kind, attempt; uint8_t backup_slot[14 * 4096];
    for (kind = 0; kind < 3; ++kind) {
        fixture(stock); install(1, 0, 1); current(stock);
        if (kind == 0) ++save.counter;
        else if (kind == 1) ++save.last_written_sector;
        else save.selected_slot ^= 1;
        rejected();
    }
    fixture(stock); install(1, 0, 1); current(stock); save.status = REMASTER_EMERALD_SAVE_EMPTY; rejected();
    fixture(stock); install(1, 0, 1);
    sector(0, 3, 11)[0] ^= 0x80;
    check(load(stock) == REMASTER_EMERALD_SAVE_DEGRADED && loaded.counter == 7 && loaded.selected_slot == 1,
        "independently valid backup selected");
    save = loaded; save.save_block1[100] ^= 0x41;
    memcpy(backup_slot, disk + 14 * 4096, sizeof(backup_slot));
    before_save = save; memcpy(before_disk, disk, sizeof(disk)); fail_write = 1;
    for (attempt = 0; attempt < 3; ++attempt) {
        check(!remaster_emerald_save_store_platform("fixture", buffer + 1, sizeof(disk), &save), "write failure reported");
        check(!memcmp(&save, &before_save, sizeof(save)) && !memcmp(disk, before_disk, sizeof(disk)), "write failure transactional");
    }
    fail_write = 0;
    check(remaster_emerald_save_store_platform("fixture", buffer + 1, sizeof(disk), &save), "valid backup can write normal next slot");
    check(!memcmp(backup_slot, disk + 14 * 4096, sizeof(backup_slot)), "backup slot remains untouched");
    check(!memcmp(before_disk + 28 * 4096, disk + 28 * 4096, 4 * 4096), "special sectors survive degraded write");
    check(load(stock) == REMASTER_EMERALD_SAVE_OK && loaded.counter == 8 && loaded.save_block1[100] == save.save_block1[100], "successful retry reloads pending state");
    fixture(stock); install(1, 0, 1); read_kind = REMASTER_SAVE_READ_MISSING;
    memset(&save, 0, sizeof(save)); save.source_is_stock = (uint8_t)stock;
    save.save_block1[100] = 0x31; before_save = save;
    check(remaster_emerald_save_store_platform("new", buffer + 1, sizeof(disk), &save) && save.counter == 1, "explicit new save creation");
    check(load(stock) == REMASTER_EMERALD_SAVE_OK && loaded.save_block1[100] == 0x31, "new image reloads in explicit format");
}
static void unsupported_and_arguments(void)
{
    unsigned version; size_t calls;
    const RemasterEmeraldSaveStatus rejected_statuses[] = {
        REMASTER_EMERALD_SAVE_CORRUPT, REMASTER_EMERALD_SAVE_IO_ERROR,
        REMASTER_EMERALD_SAVE_UNSUPPORTED, (RemasterEmeraldSaveStatus)6
    };
    for (version = 0; version < 256; ++version) if (version != 1) {
        uint8_t *p;
        fixture(0); install(1, 0, 1); current(0);
        p = sector(0, 4, 11); p[0x75C] = (uint8_t)version; put16(p + 4086, checksum(p, span(4, 0)));
        check(load(0) == REMASTER_EMERALD_SAVE_UNSUPPORTED, "unknown production VP5 version reported"); rejected();
    }
    fixture(1); install(1, 0, 1);
    sector(0, 4, 11)[0x75C] = 2; put16(sector(0, 4, 11) + 4086, checksum(sector(0, 4, 11), span(4, 1)));
    check(load(1) == REMASTER_EMERALD_SAVE_OK, "stock does not reinterpret unrelated VP5-shaped bytes");
    calls = reads;
    check(remaster_emerald_save_load_platform_format("fixture", buffer + 1, sizeof(disk), REMASTER_EMERALD_SAVE_FORMAT_UNSPECIFIED, &loaded)
        == REMASTER_EMERALD_SAVE_UNSUPPORTED && reads == calls, "unknown format rejected before read");
    check(remaster_emerald_save_load_platform_format("fixture", 0, sizeof(disk), REMASTER_EMERALD_SAVE_FORMAT_STOCK, &loaded)
        == REMASTER_EMERALD_SAVE_CORRUPT && reads == calls, "null scratch rejected before read");
    check(remaster_emerald_save_load_platform_format("fixture", buffer + 1, sizeof(disk) - 1, REMASTER_EMERALD_SAVE_FORMAT_STOCK, &loaded)
        == REMASTER_EMERALD_SAVE_CORRUPT && reads == calls, "short scratch rejected before read");
    check(remaster_emerald_save_load_platform_format("fixture", buffer + 1, sizeof(disk), REMASTER_EMERALD_SAVE_FORMAT_STOCK, 0)
        == REMASTER_EMERALD_SAVE_CORRUPT && reads == calls, "null destination rejected before read");
    fixture(0); install(1, 0, 1); current(0); save.source_is_stock = 2; rejected();
    save.source_is_stock = 0; save.status = REMASTER_EMERALD_SAVE_IO_ERROR; rejected();
    save.status = REMASTER_EMERALD_SAVE_UNSUPPORTED; rejected();
    save.status = REMASTER_EMERALD_SAVE_CORRUPT; rejected();
    save.status = REMASTER_EMERALD_SAVE_OK; save.save_block1[0x35DC] = 2; rejected();
    for (version = 0; version < sizeof(rejected_statuses) / sizeof(rejected_statuses[0]); ++version) {
        fixture(0); install(1, 0, 1); current(0); save.status = rejected_statuses[version]; snapshot();
        check(!remaster_emerald_save_encode_next(disk, sizeof(disk), &save), "raw export rejects unusable save status");
        check(!memcmp(&save, &before_save, sizeof(save)) && !memcmp(disk, before_disk, sizeof(disk)), "rejected raw export retains metadata and bytes");
    }
    remaster_platform_install(0);
}
int main(void)
{
    int stock;
    for (stock = 0; stock < 2; ++stock) { result_matrix(stock); corrupt_matrix(stock); checkpoint_and_recovery(stock); }
    unsupported_and_arguments();
    printf("R17 safe platform IO: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
