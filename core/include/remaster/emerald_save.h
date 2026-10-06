#ifndef REMASTER_EMERALD_SAVE_H
#define REMASTER_EMERALD_SAVE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_SAVE_IMAGE_BYTES = 128 * 1024,
    REMASTER_EMERALD_SECTOR_BYTES = 4096,
    REMASTER_EMERALD_SECTOR_DATA_BYTES = 3968,
    REMASTER_EMERALD_MAIN_SECTORS = 14,
    REMASTER_EMERALD_SAVE_BLOCK2_BYTES = 0x0F44,
    REMASTER_EMERALD_SAVE_BLOCK1_BYTES = 0x3DC8,
    REMASTER_EMERALD_STORAGE_BYTES = 0x83D0,
    REMASTER_EMERALD_STOCK_SAVE_BLOCK2_BYTES = 0x0F2C,
    REMASTER_EMERALD_STOCK_SAVE_BLOCK1_BYTES = 0x3D88
};

typedef enum RemasterEmeraldSaveStatus {
    REMASTER_EMERALD_SAVE_EMPTY = 0,
    REMASTER_EMERALD_SAVE_OK = 1,
    REMASTER_EMERALD_SAVE_DEGRADED = 2,
    REMASTER_EMERALD_SAVE_CORRUPT = 3
} RemasterEmeraldSaveStatus;

typedef enum RemasterEmeraldSaveFormat {
    REMASTER_EMERALD_SAVE_FORMAT_STOCK = 0,
    REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS = 1
} RemasterEmeraldSaveFormat;

/* Read-only sector/slot validation, without gameplay-domain reconstruction.
 * Selection fields are meaningful only for OK or DEGRADED results. */
typedef struct RemasterEmeraldSaveValidation {
    RemasterEmeraldSaveStatus status;
    RemasterEmeraldSaveFormat format;
    uint32_t counter;
    uint16_t last_written_sector;
    uint8_t selected_slot;
} RemasterEmeraldSaveValidation;

typedef struct RemasterEmeraldTime {
    int16_t days;
    int8_t hours;
    int8_t minutes;
    int8_t seconds;
} RemasterEmeraldTime;

typedef struct RemasterEmeraldSave {
    RemasterEmeraldSaveStatus status;
    uint32_t counter;
    uint16_t last_written_sector;
    uint8_t selected_slot;

    uint8_t save_block2[REMASTER_EMERALD_SAVE_BLOCK2_BYTES];
    uint8_t save_block1[REMASTER_EMERALD_SAVE_BLOCK1_BYTES];
    uint8_t pokemon_storage[REMASTER_EMERALD_STORAGE_BYTES];

    /* Runtime provenance only; never serialized. Zero retains legacy Vanilla+. */
    uint8_t source_is_stock;
} RemasterEmeraldSave;

uint16_t remaster_emerald_checksum(const uint8_t *data, size_t size);

/* Callers must select the source format explicitly: zero-filled tails can
 * satisfy both checksum geometries. This does not import stock domain state. */
RemasterEmeraldSaveStatus remaster_emerald_save_validate(
    const uint8_t *image,
    size_t image_size,
    RemasterEmeraldSaveFormat format,
    RemasterEmeraldSaveValidation *out_validation);

/* Explicit source layout, retained without converting persistent bytes. */
RemasterEmeraldSaveStatus remaster_emerald_save_decode_format(
    const uint8_t *image,
    size_t image_size,
    RemasterEmeraldSaveFormat format,
    RemasterEmeraldSave *out_save);

/* Maps a measured Vanilla+ SB1 field offset after the ObjectEvent array to
 * its stock location. Shared fields before that boundary are unchanged. */
size_t remaster_emerald_save_block1_offset(
    const RemasterEmeraldSave *save, size_t vanillaplus_offset);

/* The established gameplay decoder uses the pinned Vanilla+ layout. */
RemasterEmeraldSaveStatus remaster_emerald_save_decode(
    const uint8_t *image,
    size_t image_size,
    RemasterEmeraldSave *out_save);

int remaster_emerald_save_encode_next(
    uint8_t *image,
    size_t image_size,
    RemasterEmeraldSave *save);

RemasterEmeraldSaveStatus remaster_emerald_save_load_platform(
    const char *slot_name,
    uint8_t *scratch_image,
    size_t scratch_size,
    RemasterEmeraldSave *out_save);

int remaster_emerald_save_store_platform(
    const char *slot_name,
    uint8_t *scratch_image,
    size_t scratch_size,
    RemasterEmeraldSave *save);

RemasterEmeraldTime remaster_emerald_save_get_local_time_offset(
    const RemasterEmeraldSave *save);

RemasterEmeraldTime remaster_emerald_save_get_last_berry_update(
    const RemasterEmeraldSave *save);

void remaster_emerald_save_set_local_time_offset(
    RemasterEmeraldSave *save,
    RemasterEmeraldTime value);

void remaster_emerald_save_set_last_berry_update(
    RemasterEmeraldSave *save,
    RemasterEmeraldTime value);

#ifdef __cplusplus
}
#endif

#endif
