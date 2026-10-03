#include "remaster/emerald_save.h"
#include "remaster/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MemorySave {
    uint8_t image[REMASTER_EMERALD_SAVE_IMAGE_BYTES];
    size_t size;
    int exists;
    int writes;
} MemorySave;

static int save_read(
    void *userdata,
    const char *slot,
    uint8_t *buffer,
    size_t capacity,
    size_t *out_size)
{
    MemorySave *memory = (MemorySave *)userdata;
    (void)slot;

    if (memory == 0 || !memory->exists || buffer == 0 || out_size == 0)
        return 0;

    if (capacity < memory->size)
        return 0;

    memcpy(buffer, memory->image, memory->size);
    *out_size = memory->size;
    return 1;
}

static int save_write(
    void *userdata,
    const char *slot,
    const uint8_t *buffer,
    size_t size)
{
    MemorySave *memory = (MemorySave *)userdata;
    (void)slot;

    if (memory == 0 || buffer == 0
        || size != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
        return 0;

    memcpy(memory->image, buffer, size);
    memory->size = size;
    memory->exists = 1;
    memory->writes++;
    return 1;
}

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_save_platform_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    MemorySave memory;
    RemasterPlatformVTable platform;
    RemasterEmeraldSave save;
    RemasterEmeraldSave loaded;
    RemasterEmeraldSave reloaded;
    uint8_t *scratch;
    size_t special_offset =
        30u * REMASTER_EMERALD_SECTOR_BYTES + 123u;

    memset(&memory, 0, sizeof(memory));
    memset(&platform, 0, sizeof(platform));
    memset(&save, 0, sizeof(save));

    platform.userdata = &memory;
    platform.save_read = save_read;
    platform.save_write = save_write;
    remaster_platform_install(&platform);

    scratch = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (scratch == 0)
        return 1;

    if (!check(
            remaster_emerald_save_load_platform(
                "vanillaplus",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &loaded) == REMASTER_EMERALD_SAVE_EMPTY,
            "missing file must report EMPTY"))
        return 1;

    save.save_block2[0] = 0x44;
    save.save_block1[0] = 0x55;
    save.pokemon_storage[0] = 0x66;

    if (!check(
            remaster_emerald_save_store_platform(
                "vanillaplus",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &save),
            "first platform store failed"))
        return 1;

    if (!check(memory.writes == 1, "first write count mismatch"))
        return 1;
    if (!check(save.counter == 1 && save.selected_slot == 1, "first save slot/counter mismatch"))
        return 1;

    if (!check(
            remaster_emerald_save_load_platform(
                "vanillaplus",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &loaded) == REMASTER_EMERALD_SAVE_OK,
            "stored file did not reload"))
        return 1;

    if (!check(
            loaded.save_block2[0] == 0x44
            && loaded.save_block1[0] == 0x55
            && loaded.pokemon_storage[0] == 0x66,
            "stored payload mismatch"))
        return 1;

    /*
     * Sectors 28-31 are outside the alternating 14-sector main-save slots.
     * A normal game save must preserve them byte-for-byte.
     */
    memory.image[special_offset] = 0xA7;
    loaded.save_block1[10] = 0x99;

    if (!check(
            remaster_emerald_save_store_platform(
                "vanillaplus",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &loaded),
            "second platform store failed"))
        return 1;

    if (!check(memory.image[special_offset] == 0xA7, "special sector was overwritten"))
        return 1;

    if (!check(
            remaster_emerald_save_load_platform(
                "vanillaplus",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &reloaded) == REMASTER_EMERALD_SAVE_OK,
            "second save did not reload"))
        return 1;

    if (!check(
            reloaded.counter == 2
            && reloaded.selected_slot == 0
            && reloaded.save_block1[10] == 0x99,
            "second save state mismatch"))
        return 1;

    free(scratch);
    remaster_platform_install(0);
    puts("Emerald platform save I/O test passed.");
    return 0;
}
