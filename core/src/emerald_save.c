#include "remaster/emerald_save.h"
#include "remaster/platform.h"

#include <string.h>

enum {
    SECTOR_ID_SAVEBLOCK2 = 0,
    SECTOR_ID_SAVEBLOCK1_START = 1,
    SECTOR_ID_SAVEBLOCK1_END = 4,
    SECTOR_ID_STORAGE_START = 5,
    SECTOR_ID_STORAGE_END = 13,

    FOOTER_ID_OFFSET = 4084,
    FOOTER_CHECKSUM_OFFSET = 4086,
    FOOTER_SIGNATURE_OFFSET = 4088,
    FOOTER_COUNTER_OFFSET = 4092
};

static const uint32_t kSectorSignature = UINT32_C(0x08012025);

typedef struct SlotProbe {
    int has_signature;
    int valid;
    uint32_t counter;
    uint16_t valid_ids;
    uint16_t id0_physical_index;
} SlotProbe;

static uint16_t emerald_read_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}

static uint32_t emerald_read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8u)
        | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
}

static void emerald_write_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8u) & 0xffu);
}

static void emerald_write_u32_le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8u) & 0xffu);
    p[2] = (uint8_t)((value >> 16u) & 0xffu);
    p[3] = (uint8_t)((value >> 24u) & 0xffu);
}

static size_t logical_sector_size(uint16_t id, RemasterEmeraldSaveFormat format)
{
    if (id >= REMASTER_EMERALD_MAIN_SECTORS
        || (format != REMASTER_EMERALD_SAVE_FORMAT_STOCK
            && format != REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS))
        return 0;

    if (id == SECTOR_ID_SAVEBLOCK2)
        return format == REMASTER_EMERALD_SAVE_FORMAT_STOCK
            ? REMASTER_EMERALD_STOCK_SAVE_BLOCK2_BYTES
            : REMASTER_EMERALD_SAVE_BLOCK2_BYTES;
    if (id == SECTOR_ID_SAVEBLOCK1_END) {
        const size_t block1_size = format == REMASTER_EMERALD_SAVE_FORMAT_STOCK
            ? REMASTER_EMERALD_STOCK_SAVE_BLOCK1_BYTES
            : REMASTER_EMERALD_SAVE_BLOCK1_BYTES;
        return block1_size - 3u * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    }
    if (id == SECTOR_ID_STORAGE_END)
        return REMASTER_EMERALD_STORAGE_BYTES
            - 8u * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    return REMASTER_EMERALD_SECTOR_DATA_BYTES;
}

uint16_t remaster_emerald_checksum(const uint8_t *data, size_t size)
{
    size_t i;
    uint32_t checksum = 0;

    if (data == 0 || (size % 4u) != 0u)
        return 0;

    for (i = 0; i < size; i += 4u)
        checksum += emerald_read_u32_le(data + i);

    return (uint16_t)((checksum >> 16u) + checksum);
}

static SlotProbe probe_slot(
    const uint8_t *image, uint8_t slot, RemasterEmeraldSaveFormat format)
{
    SlotProbe result;
    uint16_t physical;
    int counter_coherent = 1;

    memset(&result, 0, sizeof(result));

    for (physical = 0; physical < REMASTER_EMERALD_MAIN_SECTORS; ++physical) {
        const size_t sector_index =
            (size_t)slot * REMASTER_EMERALD_MAIN_SECTORS + physical;
        const uint8_t *sector =
            image + sector_index * REMASTER_EMERALD_SECTOR_BYTES;
        const uint32_t signature =
            emerald_read_u32_le(sector + FOOTER_SIGNATURE_OFFSET);
        uint16_t id;
        size_t size;
        uint16_t expected;
        uint16_t actual;
        uint32_t counter;

        if (signature != kSectorSignature)
            continue;

        result.has_signature = 1;
        id = emerald_read_u16_le(sector + FOOTER_ID_OFFSET);

        if (id >= REMASTER_EMERALD_MAIN_SECTORS)
            continue;

        size = logical_sector_size(id, format);
        expected = emerald_read_u16_le(sector + FOOTER_CHECKSUM_OFFSET);
        actual = remaster_emerald_checksum(sector, size);

        if (expected != actual)
            continue;

        counter = emerald_read_u32_le(sector + FOOTER_COUNTER_OFFSET);
        if (result.valid_ids == 0)
            result.counter = counter;
        else if (result.counter != counter)
            counter_coherent = 0;
        result.valid_ids |= (uint16_t)(1u << id);

        if (id == 0)
            result.id0_physical_index = physical;
    }

    /* Explicit R17 hardening: the source normal writer emits one counter for
     * all 14 sections. Its original reader did not perform this coherence test. */
    result.valid = counter_coherent
        && result.valid_ids == (uint16_t)((1u << REMASTER_EMERALD_MAIN_SECTORS) - 1u);

    return result;
}

static int counter_is_newer(uint32_t a, uint32_t b)
{
    if ((a == UINT32_MAX && b == 0u) || (a == 0u && b == UINT32_MAX))
        return (uint32_t)(a + 1u) > (uint32_t)(b + 1u);

    return a > b;
}

static void copy_sector_payload(
    RemasterEmeraldSave *save,
    uint16_t id,
    const uint8_t *sector,
    RemasterEmeraldSaveFormat format)
{
    size_t size = logical_sector_size(id, format);

    if (id == SECTOR_ID_SAVEBLOCK2) {
        memcpy(save->save_block2, sector, size);
        return;
    }

    if (id >= SECTOR_ID_SAVEBLOCK1_START
        && id <= SECTOR_ID_SAVEBLOCK1_END) {
        size_t offset =
            (size_t)(id - SECTOR_ID_SAVEBLOCK1_START)
            * REMASTER_EMERALD_SECTOR_DATA_BYTES;
        memcpy(save->save_block1 + offset, sector, size);
        return;
    }

    if (id >= SECTOR_ID_STORAGE_START && id <= SECTOR_ID_STORAGE_END) {
        size_t offset =
            (size_t)(id - SECTOR_ID_STORAGE_START)
            * REMASTER_EMERALD_SECTOR_DATA_BYTES;
        memcpy(save->pokemon_storage + offset, sector, size);
    }
}

static int reconstruct_selected_slot(
    const uint8_t *image,
    uint8_t slot,
    RemasterEmeraldSave *save,
    RemasterEmeraldSaveFormat format)
{
    uint16_t physical;
    uint16_t copied = 0;

    for (physical = 0; physical < REMASTER_EMERALD_MAIN_SECTORS; ++physical) {
        const size_t sector_index =
            (size_t)slot * REMASTER_EMERALD_MAIN_SECTORS + physical;
        const uint8_t *sector =
            image + sector_index * REMASTER_EMERALD_SECTOR_BYTES;
        uint16_t id;
        size_t size;

        if (emerald_read_u32_le(sector + FOOTER_SIGNATURE_OFFSET) != kSectorSignature)
            continue;

        id = emerald_read_u16_le(sector + FOOTER_ID_OFFSET);
        if (id >= REMASTER_EMERALD_MAIN_SECTORS)
            continue;

        size = logical_sector_size(id, format);
        if (emerald_read_u16_le(sector + FOOTER_CHECKSUM_OFFSET)
            != remaster_emerald_checksum(sector, size))
            continue;

        copy_sector_payload(save, id, sector, format);
        copied |= (uint16_t)(1u << id);

        if (id == 0)
            save->last_written_sector = physical;
    }

    return copied
        == (uint16_t)((1u << REMASTER_EMERALD_MAIN_SECTORS) - 1u);
}

RemasterEmeraldSaveStatus remaster_emerald_save_validate(
    const uint8_t *image,
    size_t image_size,
    RemasterEmeraldSaveFormat format,
    RemasterEmeraldSaveValidation *out_validation)
{
    SlotProbe slots[2];
    int selected = -1;
    int other_bad = 0;

    if (out_validation == 0)
        return REMASTER_EMERALD_SAVE_CORRUPT;

    memset(out_validation, 0, sizeof(*out_validation));
    out_validation->status = REMASTER_EMERALD_SAVE_CORRUPT;
    if (image == 0 || image_size != REMASTER_EMERALD_SAVE_IMAGE_BYTES
        || (format != REMASTER_EMERALD_SAVE_FORMAT_STOCK
            && format != REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS))
        return out_validation->status;
    out_validation->format = format;

    slots[0] = probe_slot(image, 0, format);
    slots[1] = probe_slot(image, 1, format);

    if (slots[0].valid && slots[1].valid)
        selected = counter_is_newer(slots[1].counter, slots[0].counter) ? 1 : 0;
    else if (slots[0].valid) {
        selected = 0;
        other_bad = slots[1].has_signature;
    } else if (slots[1].valid) {
        selected = 1;
        other_bad = slots[0].has_signature;
    } else {
        if (!slots[0].has_signature && !slots[1].has_signature) {
            out_validation->status = REMASTER_EMERALD_SAVE_EMPTY;
            return out_validation->status;
        }

        return out_validation->status;
    }

    out_validation->selected_slot = (uint8_t)selected;
    out_validation->counter = slots[selected].counter;
    out_validation->last_written_sector = slots[selected].id0_physical_index;
    out_validation->status =
        other_bad ? REMASTER_EMERALD_SAVE_DEGRADED : REMASTER_EMERALD_SAVE_OK;
    return out_validation->status;
}

RemasterEmeraldSaveStatus remaster_emerald_save_decode_format(
    const uint8_t *image,
    size_t image_size,
    RemasterEmeraldSaveFormat format,
    RemasterEmeraldSave *out_save)
{
    RemasterEmeraldSaveValidation validation;
    RemasterEmeraldSaveStatus status;

    if (out_save == 0)
        return REMASTER_EMERALD_SAVE_CORRUPT;

    memset(out_save, 0, sizeof(*out_save));
    status = remaster_emerald_save_validate(image, image_size,
        format, &validation);
    if (status != REMASTER_EMERALD_SAVE_OK
        && status != REMASTER_EMERALD_SAVE_DEGRADED) {
        out_save->status = status;
        return status;
    }

    out_save->selected_slot = validation.selected_slot;
    out_save->counter = validation.counter;
    out_save->last_written_sector = validation.last_written_sector;

    out_save->source_is_stock =
        (uint8_t)(format == REMASTER_EMERALD_SAVE_FORMAT_STOCK);

    if (!reconstruct_selected_slot(image, validation.selected_slot, out_save, format)) {
        out_save->status = REMASTER_EMERALD_SAVE_CORRUPT;
        return out_save->status;
    }

    out_save->status = status;

    return out_save->status;
}

RemasterEmeraldSaveStatus remaster_emerald_save_decode(
    const uint8_t *image,
    size_t image_size,
    RemasterEmeraldSave *out_save)
{
    if (image == 0 || out_save == 0
        || image_size != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
        return REMASTER_EMERALD_SAVE_CORRUPT;
    return remaster_emerald_save_decode_format(image, image_size,
        REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS, out_save);
}

size_t remaster_emerald_save_block1_offset(
    const RemasterEmeraldSave *save, size_t vanillaplus_offset)
{
    /* AGBCC: 16 ObjectEvents grow from 0x24 to 0x28 in the fork. */
    if (save != 0 && save->source_is_stock && vanillaplus_offset >= 0xCB0u)
        return vanillaplus_offset - 0x40u;
    return vanillaplus_offset;
}

static const uint8_t *logical_payload(
    const RemasterEmeraldSave *save,
    uint16_t id)
{
    if (id == SECTOR_ID_SAVEBLOCK2)
        return save->save_block2;

    if (id >= SECTOR_ID_SAVEBLOCK1_START
        && id <= SECTOR_ID_SAVEBLOCK1_END) {
        return save->save_block1
            + (size_t)(id - SECTOR_ID_SAVEBLOCK1_START)
            * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    }

    return save->pokemon_storage
        + (size_t)(id - SECTOR_ID_STORAGE_START)
        * REMASTER_EMERALD_SECTOR_DATA_BYTES;
}

/* Preparing scratch bytes is separate from committing caller-visible state. */
typedef struct SaveWritePlan {
    uint32_t counter;
    uint16_t last_written_sector;
    uint8_t selected_slot;
} SaveWritePlan;

static int writable_save_status(RemasterEmeraldSaveStatus status)
{
    return status == REMASTER_EMERALD_SAVE_EMPTY
        || status == REMASTER_EMERALD_SAVE_OK
        || status == REMASTER_EMERALD_SAVE_DEGRADED;
}

static int prepare_next_image(
    uint8_t *image,
    size_t image_size,
    const RemasterEmeraldSave *save,
    SaveWritePlan *plan)
{
    uint16_t id;
    uint16_t next_rotation;
    uint32_t next_counter;
    uint8_t next_slot;
    RemasterEmeraldSaveFormat format;

    if (image == 0 || save == 0 || save->source_is_stock > 1u
        || !writable_save_status(save->status)
        || image_size != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
        return 0;

    format = save->source_is_stock
        ? REMASTER_EMERALD_SAVE_FORMAT_STOCK
        : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    next_rotation =
        (uint16_t)((save->last_written_sector + 1u)
            % REMASTER_EMERALD_MAIN_SECTORS);
    next_counter = save->counter + 1u;
    next_slot = (uint8_t)(next_counter % 2u);

    for (id = 0; id < REMASTER_EMERALD_MAIN_SECTORS; ++id) {
        const uint16_t physical =
            (uint16_t)((id + next_rotation)
                % REMASTER_EMERALD_MAIN_SECTORS);
        const size_t sector_index =
            (size_t)next_slot * REMASTER_EMERALD_MAIN_SECTORS + physical;
        uint8_t *sector =
            image + sector_index * REMASTER_EMERALD_SECTOR_BYTES;
        const uint8_t *payload = logical_payload(save, id);
        const size_t size = logical_sector_size(id, format);

        memset(sector, 0, REMASTER_EMERALD_SECTOR_BYTES);
        memcpy(sector, payload, size);

        emerald_write_u16_le(sector + FOOTER_ID_OFFSET, id);
        emerald_write_u16_le(
            sector + FOOTER_CHECKSUM_OFFSET,
            remaster_emerald_checksum(payload, size));
        emerald_write_u32_le(sector + FOOTER_SIGNATURE_OFFSET, kSectorSignature);
        emerald_write_u32_le(sector + FOOTER_COUNTER_OFFSET, next_counter);
    }

    plan->counter = next_counter;
    plan->last_written_sector = next_rotation;
    plan->selected_slot = next_slot;

    return 1;
}

static void commit_write_plan(RemasterEmeraldSave *save, const SaveWritePlan *plan)
{
    save->counter = plan->counter;
    save->last_written_sector = plan->last_written_sector;
    save->selected_slot = plan->selected_slot;
    save->status = REMASTER_EMERALD_SAVE_OK;
}

int remaster_emerald_save_encode_next(
    uint8_t *image,
    size_t image_size,
    RemasterEmeraldSave *save)
{
    SaveWritePlan plan;
    if (!prepare_next_image(image, image_size, save, &plan))
        return 0;
    commit_write_plan(save, &plan);
    return 1;
}

static RemasterEmeraldTime read_time(
    const RemasterEmeraldSave *save,
    size_t offset)
{
    RemasterEmeraldTime value = {0, 0, 0, 0};

    if (save == 0)
        return value;

    value.days = (int16_t)emerald_read_u16_le(save->save_block2 + offset);
    value.hours = (int8_t)save->save_block2[offset + 2u];
    value.minutes = (int8_t)save->save_block2[offset + 3u];
    value.seconds = (int8_t)save->save_block2[offset + 4u];
    return value;
}

static void write_time(
    RemasterEmeraldSave *save,
    size_t offset,
    RemasterEmeraldTime value)
{
    if (save == 0)
        return;

    emerald_write_u16_le(save->save_block2 + offset, (uint16_t)value.days);
    save->save_block2[offset + 2u] = (uint8_t)value.hours;
    save->save_block2[offset + 3u] = (uint8_t)value.minutes;
    save->save_block2[offset + 4u] = (uint8_t)value.seconds;
}

RemasterEmeraldTime remaster_emerald_save_get_local_time_offset(
    const RemasterEmeraldSave *save)
{
    return read_time(save, 0x98u);
}

RemasterEmeraldTime remaster_emerald_save_get_last_berry_update(
    const RemasterEmeraldSave *save)
{
    return read_time(save, 0xA0u);
}

void remaster_emerald_save_set_local_time_offset(
    RemasterEmeraldSave *save,
    RemasterEmeraldTime value)
{
    write_time(save, 0x98u, value);
}

void remaster_emerald_save_set_last_berry_update(
    RemasterEmeraldSave *save,
    RemasterEmeraldTime value)
{
    write_time(save, 0xA0u, value);
}


static RemasterSaveReadResult read_platform_image(
    const RemasterPlatformVTable *platform,
    const char *slot_name,
    uint8_t *image,
    size_t *size)
{
    RemasterSaveReadResult result;
    *size = 0;
    if (platform == 0)
        return REMASTER_SAVE_READ_ERROR;
    if (platform->save_read_result != 0) {
        result = platform->save_read_result(platform->userdata, slot_name,
            image, REMASTER_EMERALD_SAVE_IMAGE_BYTES, size);
        if (result == REMASTER_SAVE_READ_OK)
            return result;
        *size = 0;
        return result == REMASTER_SAVE_READ_MISSING
            ? REMASTER_SAVE_READ_MISSING : REMASTER_SAVE_READ_ERROR;
    }
    if (platform->save_read != 0
        && platform->save_read(platform->userdata, slot_name,
            image, REMASTER_EMERALD_SAVE_IMAGE_BYTES, size) > 0)
        return REMASTER_SAVE_READ_OK;
    *size = 0;
    return REMASTER_SAVE_READ_ERROR;
}

static int vp_metadata_supported(const uint8_t *metadata)
{
    return emerald_read_u32_le(metadata) != UINT32_C(0x35504956)
        || metadata[4] == 1;
}

static int image_metadata_supported(
    const uint8_t *image, const RemasterEmeraldSaveValidation *validation)
{
    size_t sector;
    if (validation->format == REMASTER_EMERALD_SAVE_FORMAT_STOCK)
        return 1;
    /* Production SB1 VP5 lies in logical section 4, at 0x35D8-3*0xF80. */
    sector = (size_t)validation->selected_slot * 14u
        + (4u + validation->last_written_sector) % 14u;
    return vp_metadata_supported(image + sector * 4096u + 0x758u);
}

static RemasterEmeraldSaveStatus clear_load_result(
    RemasterEmeraldSave *save, RemasterEmeraldSaveStatus status,
    RemasterEmeraldSaveFormat format)
{
    if (save != 0) {
        memset(save, 0, sizeof(*save));
        save->status = status;
        save->source_is_stock = (uint8_t)(format == REMASTER_EMERALD_SAVE_FORMAT_STOCK);
    }
    return status;
}

RemasterEmeraldSaveStatus remaster_emerald_save_load_platform_format(
    const char *slot_name,
    uint8_t *scratch_image,
    size_t scratch_size,
    RemasterEmeraldSaveFormat format,
    RemasterEmeraldSave *out_save)
{
    const RemasterPlatformVTable *platform = remaster_platform_get();
    size_t size = 0;
    RemasterSaveReadResult read_result;
    RemasterEmeraldSaveValidation validation;
    RemasterEmeraldSaveStatus status;

    if (out_save == 0 || scratch_image == 0
        || scratch_size < REMASTER_EMERALD_SAVE_IMAGE_BYTES)
        return clear_load_result(out_save, REMASTER_EMERALD_SAVE_CORRUPT, format);
    if (format != REMASTER_EMERALD_SAVE_FORMAT_STOCK
        && format != REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS)
        return clear_load_result(out_save, REMASTER_EMERALD_SAVE_UNSUPPORTED, format);

    read_result = read_platform_image(platform,
        slot_name != 0 ? slot_name : "emerald", scratch_image, &size);
    if (read_result == REMASTER_SAVE_READ_MISSING)
        return clear_load_result(out_save, REMASTER_EMERALD_SAVE_EMPTY, format);
    if (read_result != REMASTER_SAVE_READ_OK)
        return clear_load_result(out_save, REMASTER_EMERALD_SAVE_IO_ERROR, format);
    if (size != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
        return clear_load_result(out_save, REMASTER_EMERALD_SAVE_UNSUPPORTED, format);

    status = remaster_emerald_save_validate(scratch_image, size, format, &validation);
    /* Raw flash probing calls signatureless slots EMPTY. A found filesystem
     * image is not confirmed absence and cannot authorize destructive repair. */
    if (status != REMASTER_EMERALD_SAVE_OK && status != REMASTER_EMERALD_SAVE_DEGRADED)
        return clear_load_result(out_save, REMASTER_EMERALD_SAVE_CORRUPT, format);
    if (!image_metadata_supported(scratch_image, &validation))
        return clear_load_result(out_save, REMASTER_EMERALD_SAVE_UNSUPPORTED, format);
    return remaster_emerald_save_decode_format(scratch_image, size, format, out_save);
}

RemasterEmeraldSaveStatus remaster_emerald_save_load_platform(
    const char *slot_name,
    uint8_t *scratch_image,
    size_t scratch_size,
    RemasterEmeraldSave *out_save)
{
    return remaster_emerald_save_load_platform_format(slot_name, scratch_image,
        scratch_size, REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS, out_save);
}

int remaster_emerald_save_store_platform(
    const char *slot_name,
    uint8_t *scratch_image,
    size_t scratch_size,
    RemasterEmeraldSave *save)
{
    const RemasterPlatformVTable *platform = remaster_platform_get();
    size_t size = 0;
    SaveWritePlan plan;
    RemasterSaveReadResult read_result;
    RemasterEmeraldSaveValidation validation;
    RemasterEmeraldSaveStatus status;
    RemasterEmeraldSaveFormat format;

    if (save == 0 || scratch_image == 0
        || scratch_size < REMASTER_EMERALD_SAVE_IMAGE_BYTES
        || platform == 0 || platform->save_write == 0
        || save->source_is_stock > 1
        || !writable_save_status(save->status))
        return 0;
    format = save->source_is_stock ? REMASTER_EMERALD_SAVE_FORMAT_STOCK
        : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    if (!save->source_is_stock && !vp_metadata_supported(save->save_block1 + 0x35D8))
        return 0;

    read_result = read_platform_image(platform,
        slot_name != 0 ? slot_name : "emerald", scratch_image, &size);
    if (read_result == REMASTER_SAVE_READ_MISSING) {
        if (save->status != REMASTER_EMERALD_SAVE_EMPTY || save->counter != 0
            || save->last_written_sector != 0 || save->selected_slot != 0)
            return 0;
        memset(scratch_image, 0xff, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    } else if (read_result == REMASTER_SAVE_READ_OK) {
        if (size != REMASTER_EMERALD_SAVE_IMAGE_BYTES
            || save->status == REMASTER_EMERALD_SAVE_EMPTY)
            return 0;
        status = remaster_emerald_save_validate(scratch_image, size, format, &validation);
        if ((status != REMASTER_EMERALD_SAVE_OK && status != REMASTER_EMERALD_SAVE_DEGRADED)
            || !image_metadata_supported(scratch_image, &validation)
            || validation.counter != save->counter
            || validation.selected_slot != save->selected_slot
            || validation.last_written_sector != save->last_written_sector)
            return 0;
    } else {
        return 0;
    }

    if (!prepare_next_image(
            scratch_image,
            REMASTER_EMERALD_SAVE_IMAGE_BYTES,
            save,
            &plan))
        return 0;

    if (!platform->save_write(
            platform->userdata,
            slot_name != 0 ? slot_name : "emerald",
            scratch_image,
            REMASTER_EMERALD_SAVE_IMAGE_BYTES))
        return 0;

    commit_write_plan(save, &plan);
    return 1;
}
