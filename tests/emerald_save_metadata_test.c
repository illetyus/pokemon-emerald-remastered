#include "remaster/emerald_qol.h"
#include "remaster/emerald_items.h"

#include <stdio.h>
#include <string.h>

/* Independent measured layout and packed source metadata, not core offsets. */
enum { PROD = 0x35D8, LEGACY = 0x3598, BYTES = 21, REGISTERED = 0x496 };
static unsigned checks, failures;
static RemasterEmeraldSave save, expected, decoded;
static uint8_t image[131072], old_image[131072];

#ifdef R17_METADATA_BASELINE
enum {
    REMASTER_EMERALD_QOL_METADATA_INVALID,
    REMASTER_EMERALD_QOL_METADATA_NOT_APPLICABLE,
    REMASTER_EMERALD_QOL_METADATA_CURRENT,
    REMASTER_EMERALD_QOL_METADATA_RECOVERED,
    REMASTER_EMERALD_QOL_METADATA_NO_LEGACY,
    REMASTER_EMERALD_QOL_METADATA_CONFLICT,
    REMASTER_EMERALD_QOL_METADATA_UNSUPPORTED
};
/* The pre-I4 implementation has no explicit recovery operation. */
static int remaster_emerald_qol_recover_legacy_item_metadata(RemasterEmeraldSave *s)
{ (void)s; return REMASTER_EMERALD_QOL_METADATA_NO_LEGACY; }
#endif

static void check(int ok, const char *message)
{
    ++checks;
    if (!ok) {
        if (failures < 20) fprintf(stderr, "metadata: %s\n", message);
        ++failures;
    }
}
static unsigned get16(const uint8_t *p)
{ return (unsigned)p[0] | ((unsigned)p[1] << 8); }
static void put16(uint8_t *p, unsigned value)
{ p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t *p, uint32_t value)
{ put16(p, value & 0xFFFFu); put16(p + 2, value >> 16); }
static void metadata(uint8_t *p)
{
    memset(p, 0, BYTES);
    put32(p, UINT32_C(0x35504956));
    p[4] = 1; p[5] = 1; p[6] = 3; p[11] = 2;
    put16(p + 12, 259); put16(p + 14, 260); p[20] = 1;
}
static void seed(int stock)
{
    size_t i;
    memset(&save, 0, sizeof(save));
    save.status = REMASTER_EMERALD_SAVE_OK;
    save.counter = 12; save.last_written_sector = 6;
    save.source_is_stock = (uint8_t)stock;
    for (i = 0; i < sizeof(save.save_block1); ++i)
        save.save_block1[i] = (uint8_t)(i * 17u + 23u);
    for (i = 0; i < sizeof(save.save_block2); ++i)
        save.save_block2[i] = (uint8_t)(i * 13u + 41u);
    for (i = 0; i < sizeof(save.pokemon_storage); ++i)
        save.pokemon_storage[i] = (uint8_t)(i * 7u + 31u);
    memset(save.save_block1 + 0x560, 0, 30u * 4u);
    put32(save.save_block2 + 0xAC, 0);
    put16(save.save_block1 + REGISTERED, 259);
}
static void domains_equal(const RemasterEmeraldSave *a, const RemasterEmeraldSave *b)
{
    size_t i;
    for (i = 0; i < sizeof(a->save_block1); ++i)
        check(a->save_block1[i] == b->save_block1[i], "exact SB1 mutation boundary");
    check(!memcmp(a->save_block2, b->save_block2, sizeof(a->save_block2)), "SB2 unchanged");
    check(!memcmp(a->pokemon_storage, b->pokemon_storage, sizeof(a->pokemon_storage)), "storage unchanged");
}
static void unchanged(void)
{ check(!memcmp(&save, &expected, sizeof(save)), "rejection is byte-for-byte transactional"); }
static uint16_t checksum(const uint8_t *p, size_t n)
{
    uint32_t sum = 0; size_t i;
    for (i = 0; i < n; i += 4)
        sum += (uint32_t)get16(p + i) | ((uint32_t)get16(p + i + 2) << 16);
    return (uint16_t)((sum >> 16) + sum);
}
static void fixture(void)
{
    unsigned id, slot; size_t i;
    for (i = 0; i < sizeof(image); ++i) image[i] = (uint8_t)(i * 11u + 19u);
    for (slot = 0; slot < 2; ++slot) for (id = 0; id < 14; ++id) {
        uint8_t *sector = image + (slot * 14u + (id + 6u) % 14u) * 4096u;
        const uint8_t *data = id == 0 ? save.save_block2 : id <= 4
            ? save.save_block1 + (id - 1u) * 3968u
            : save.pokemon_storage + (id - 5u) * 3968u;
        const size_t n = id == 0 ? (save.source_is_stock ? 0xF2Cu : 0xF44u)
            : id == 4 ? (save.source_is_stock ? 0xF08u : 0xF48u)
            : id == 13 ? 0x7D0u : 3968u;
        memcpy(sector, data, n);
        put16(sector + 0xFF4, id); put16(sector + 0xFF6, checksum(data, n));
        put32(sector + 0xFF8, UINT32_C(0x08012025));
        put32(sector + 0xFFC, slot ? 11u : 12u);
    }
}
static void roundtrip(void)
{
    const int stock = save.source_is_stock;
    const RemasterEmeraldSaveFormat format = stock ? REMASTER_EMERALD_SAVE_FORMAT_STOCK
        : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    fixture(); memcpy(old_image, image, sizeof(image));
    check(remaster_emerald_save_decode_format(image, sizeof(image), format, &decoded)
        == REMASTER_EMERALD_SAVE_OK, "independent input fixture imports");
    check(remaster_emerald_save_encode_next(image, sizeof(image), &save), "metadata export succeeds");
    check(!memcmp(image, old_image, 14u * 4096u), "other main slot retained");
    check(!memcmp(image + 28u * 4096u, old_image + 28u * 4096u, 4u * 4096u), "special sectors retained");
    check(remaster_emerald_save_decode_format(image, sizeof(image), format, &decoded)
        == REMASTER_EMERALD_SAVE_OK, "metadata export reimports");
    check(!memcmp(decoded.save_block1, save.save_block1, stock ? 0x3D88u : 0x3DC8u), "all source SB1 bytes roundtrip");
    check(!memcmp(decoded.save_block2, save.save_block2, stock ? 0xF2Cu : 0xF44u), "all source SB2 bytes roundtrip");
    check(!memcmp(decoded.pokemon_storage, save.pokemon_storage, 0x83D0u), "all storage bytes roundtrip");
}
static void production(void)
{
    unsigned invalid;
    seed(0); metadata(save.save_block1 + PROD); metadata(save.save_block1 + LEGACY);
    put16(save.save_block1 + LEGACY + 12, 261); expected = save;
    check(remaster_emerald_qol_quick_item_count(&save) == 2, "production count");
    check(remaster_emerald_qol_quick_item_get(&save, 0) == 259, "production wins over legacy");
    check(remaster_emerald_qol_quick_item_get(&save, 1) == 260, "production second item");
    check(remaster_emerald_qol_quick_item_get(&save, 4) == 0, "slot bound");
    check(remaster_emerald_qol_bag_sort_mode(&save, REMASTER_EMERALD_POCKET_ITEMS) == REMASTER_EMERALD_QOL_ITEM_SORT_QUANTITY, "production sort mode");
    check(remaster_emerald_qol_bag_auto_sort_enabled(&save, REMASTER_EMERALD_POCKET_ITEMS), "production auto sort");
    unchanged();
    check(remaster_emerald_qol_bag_set_sort_mode(&save, REMASTER_EMERALD_POCKET_ITEMS, REMASTER_EMERALD_QOL_ITEM_SORT_VALUE), "set production sort");
    expected.save_block1[PROD + 6] = 4; domains_equal(&save, &expected);
    check(remaster_emerald_qol_quick_item_register(&save, 261), "register third");
    expected.save_block1[PROD + 11] = 3; put16(expected.save_block1 + PROD + 16, 261);
    domains_equal(&save, &expected); roundtrip();
    check(remaster_emerald_qol_quick_item_count(&decoded) == 3, "production quick items survive roundtrip");
    check(remaster_emerald_qol_bag_sort_mode(&decoded, REMASTER_EMERALD_POCKET_ITEMS) == REMASTER_EMERALD_QOL_ITEM_SORT_VALUE, "production sort preference survives roundtrip");
    for (invalid = 0; invalid < 3; ++invalid) {
        seed(0); metadata(save.save_block1 + LEGACY);
        memset(save.save_block1 + PROD, 0, BYTES);
        if (invalid) { metadata(save.save_block1 + PROD); save.save_block1[PROD + (invalid == 1 ? 11 : 6)] = 5; }
        expected = save; memset(expected.save_block1 + PROD, 0, BYTES);
        put32(expected.save_block1 + PROD, UINT32_C(0x35504956));
        expected.save_block1[PROD + 4] = 1; expected.save_block1[PROD + 11] = 1;
        put16(expected.save_block1 + PROD + 12, 259);
        check(remaster_emerald_qol_quick_item_count(&save) == 1, "source lazy reset seeds registered item, never legacy");
        domains_equal(&save, &expected);
    }
}
static void stock_boundary(void)
{
    seed(1); metadata(save.save_block1 + PROD); metadata(save.save_block1 + LEGACY); expected = save;
    check(remaster_emerald_qol_quick_item_count(&save) == 1, "stock runtime starts from primary");
    check(remaster_emerald_qol_quick_item_register(&save, 260), "stock runtime second item");
    check(remaster_emerald_qol_bag_set_sort_mode(&save, REMASTER_EMERALD_POCKET_ITEMS, REMASTER_EMERALD_QOL_ITEM_SORT_NAME), "stock runtime sort setting");
    check(remaster_emerald_qol_bag_set_auto_sort_enabled(&save, REMASTER_EMERALD_POCKET_ITEMS, 1), "stock runtime auto sort");
    check(remaster_emerald_qol_bag_sort_mode(&save, REMASTER_EMERALD_POCKET_ITEMS) == REMASTER_EMERALD_QOL_ITEM_SORT_NAME, "stock runtime setting reads back");
    check(remaster_emerald_qol_quick_item_count(&save) == 2, "stock runtime multiple item count");
    domains_equal(&save, &expected);
    check(remaster_emerald_qol_quick_item_unregister(&save, 259), "stock primary update");
    put16(expected.save_block1 + REGISTERED, 260); domains_equal(&save, &expected);
    roundtrip();
    check(remaster_emerald_qol_quick_item_count(&decoded) == 1 && remaster_emerald_qol_quick_item_get(&decoded, 0) == 260, "stock roundtrip retains source primary, not runtime extensions");
    check(remaster_emerald_qol_bag_sort_mode(&decoded, REMASTER_EMERALD_POCKET_ITEMS) == REMASTER_EMERALD_QOL_ITEM_SORT_NONE, "stock runtime preference is not serialized");
}
static void unsupported(void)
{
    unsigned version;
    for (version = 0; version <= 255; ++version) if (version != 1) {
        seed(0); metadata(save.save_block1 + PROD); save.save_block1[PROD + 4] = (uint8_t)version;
        check(remaster_emerald_bag_add(&save, 13, 2), "prepare real item bag");
        check(remaster_emerald_bag_add(&save, 14, 1), "prepare unsorted bag");
        expected = save;
        check(remaster_emerald_qol_quick_item_count(&save) == 0, "opaque version count fallback");
        check(remaster_emerald_qol_quick_item_get(&save, 0) == 0, "opaque version item fallback");
        check(remaster_emerald_qol_bag_sort_mode(&save, REMASTER_EMERALD_POCKET_ITEMS) == REMASTER_EMERALD_QOL_ITEM_SORT_NONE, "opaque version mode fallback");
        check(!remaster_emerald_qol_bag_auto_sort_enabled(&save, REMASTER_EMERALD_POCKET_ITEMS), "opaque version auto sort fallback");
        check(!remaster_emerald_qol_bag_set_sort_mode(&save, REMASTER_EMERALD_POCKET_ITEMS, REMASTER_EMERALD_QOL_ITEM_SORT_NAME), "opaque version cannot set mode");
        check(!remaster_emerald_qol_bag_set_auto_sort_enabled(&save, REMASTER_EMERALD_POCKET_ITEMS, 1), "opaque version cannot set auto sort");
        check(!remaster_emerald_qol_bag_sort(&save, REMASTER_EMERALD_POCKET_ITEMS, REMASTER_EMERALD_QOL_ITEM_SORT_NAME), "opaque version rejects before reordering gameplay bag");
        check(!remaster_emerald_qol_quick_item_register(&save, 261), "opaque version cannot register");
        check(!remaster_emerald_qol_quick_item_unregister(&save, 259), "opaque version cannot unregister");
        remaster_emerald_qol_quick_items_prune(&save); unchanged();
        check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_UNSUPPORTED, "opaque destination recovery rejection"); unchanged();
        if (version == 2) roundtrip();
    }
}
static void recovery(void)
{
    unsigned fill, invalid;
    for (fill = 0; fill <= 1; ++fill) {
        seed(0); memset(save.save_block1 + PROD, fill ? 0xFF : 0, BYTES);
        metadata(save.save_block1 + LEGACY); expected = save;
        memcpy(expected.save_block1 + PROD, save.save_block1 + LEGACY, BYTES);
        check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_RECOVERED, "explicit blank-destination recovery");
        domains_equal(&save, &expected);
        expected = save;
        check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_CURRENT, "recovery idempotent/current wins"); unchanged();
        roundtrip();
    }
    seed(0); metadata(save.save_block1 + LEGACY); expected = save;
    check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_CONFLICT, "nonblank destination cannot be overwritten"); unchanged();
    metadata(save.save_block1 + PROD); put16(save.save_block1 + PROD + 12, 261); expected = save;
    check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_CURRENT, "different valid production metadata wins over legacy"); unchanged();
    for (invalid = 0; invalid < BYTES; ++invalid) {
        seed(0); memset(save.save_block1 + PROD, 0, BYTES); metadata(save.save_block1 + LEGACY);
        save.save_block1[PROD + invalid] = 0xFF; expected = save;
        check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_CONFLICT, "mixed blank destination cannot be overwritten"); unchanged();
    }
    for (invalid = 0; invalid < 4; ++invalid) {
        seed(0); memset(save.save_block1 + PROD, 0, BYTES); metadata(save.save_block1 + LEGACY);
        save.save_block1[LEGACY + (invalid == 0 ? 0 : invalid == 1 ? 4 : invalid == 2 ? 11 : 6)] = invalid == 1 ? 2 : 5;
        expected = save;
        check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_NO_LEGACY, "only valid version-1 legacy metadata recoverable"); unchanged();
    }
    seed(1); metadata(save.save_block1 + LEGACY); expected = save;
    check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_NOT_APPLICABLE, "stock recovery never manufactures VP5"); unchanged();
    seed(0); save.source_is_stock = 2; expected = save;
    check(remaster_emerald_qol_quick_item_count(&save) == 0 && !remaster_emerald_qol_quick_item_register(&save, 260), "invalid source provenance rejected");
    check(remaster_emerald_qol_recover_legacy_item_metadata(&save) == REMASTER_EMERALD_QOL_METADATA_INVALID, "invalid source recovery rejected"); unchanged();
    check(remaster_emerald_qol_recover_legacy_item_metadata(0) == REMASTER_EMERALD_QOL_METADATA_INVALID, "null recovery");
    check(remaster_emerald_qol_quick_item_count(0) == 0 && !remaster_emerald_qol_quick_item_register(0, 1), "null queries/registration");
    remaster_emerald_qol_quick_items_prune(0);
}
int main(void)
{
    production(); stock_boundary(); unsupported(); recovery();
    printf("R17 metadata boundary: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
