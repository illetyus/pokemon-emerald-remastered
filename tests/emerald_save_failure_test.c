#include "remaster/emerald_save.h"
#include "remaster/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum ReadMode {
    READ_MISSING = 0,
    READ_OK = 1,
    READ_ERROR = 2,
    READ_WRONG_SIZE = 3
} ReadMode;

typedef struct FailurePlatform {
    uint8_t image[REMASTER_EMERALD_SAVE_IMAGE_BYTES];
    size_t size;
    ReadMode read_mode;
    int write_ok;
    int writes;
} FailurePlatform;

static int save_read(
    void *userdata,
    const char *slot,
    uint8_t *buffer,
    size_t capacity,
    size_t *out_size)
{
    FailurePlatform *state = (FailurePlatform *)userdata;
    (void)slot;

    if (state == 0 || buffer == 0 || out_size == 0)
        return REMASTER_SAVE_READ_ERROR;

    if (state->read_mode == READ_MISSING)
        return REMASTER_SAVE_READ_MISSING;

    if (state->read_mode == READ_ERROR)
        return REMASTER_SAVE_READ_ERROR;

    if (state->read_mode == READ_WRONG_SIZE) {
        const size_t short_size = REMASTER_EMERALD_SAVE_IMAGE_BYTES / 2u;
        if (capacity < short_size)
            return REMASTER_SAVE_READ_ERROR;
        memcpy(buffer, state->image, short_size);
        *out_size = short_size;
        return REMASTER_SAVE_READ_OK;
    }

    if (capacity < state->size)
        return REMASTER_SAVE_READ_ERROR;

    memcpy(buffer, state->image, state->size);
    *out_size = state->size;
    return REMASTER_SAVE_READ_OK;
}

static int save_write(
    void *userdata,
    const char *slot,
    const uint8_t *buffer,
    size_t size)
{
    FailurePlatform *state = (FailurePlatform *)userdata;
    (void)slot;

    if (state == 0 || buffer == 0
        || size != REMASTER_EMERALD_SAVE_IMAGE_BYTES)
        return 0;

    state->writes++;
    if (!state->write_ok)
        return 0;

    memcpy(state->image, buffer, size);
    state->size = size;
    state->read_mode = READ_OK;
    return 1;
}

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_save_failure_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    FailurePlatform state;
    RemasterPlatformVTable platform;
    RemasterEmeraldSave save;
    RemasterEmeraldSave loaded;
    uint8_t *scratch;
    uint32_t before_counter;
    uint16_t before_last_sector;
    uint8_t before_selected_slot;
    RemasterEmeraldSaveStatus before_status;

    memset(&state, 0, sizeof(state));
    memset(&platform, 0, sizeof(platform));
    memset(&save, 0, sizeof(save));
    memset(&loaded, 0, sizeof(loaded));

    platform.userdata = &state;
    platform.save_read = save_read;
    platform.save_write = save_write;
    remaster_platform_install(&platform);

    scratch = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (scratch == 0)
        return 1;

    state.read_mode = READ_ERROR;
    if (!check(
            remaster_emerald_save_load_platform(
                "compat",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &loaded) == REMASTER_EMERALD_SAVE_CORRUPT,
            "I/O read failure must not look like a missing save"))
        return 1;

    state.read_mode = READ_MISSING;
    if (!check(
            remaster_emerald_save_load_platform(
                "compat",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &loaded) == REMASTER_EMERALD_SAVE_EMPTY,
            "missing save must remain EMPTY"))
        return 1;

    state.write_ok = 1;
    save.save_block1[0] = 0xA5;
    if (!check(
            remaster_emerald_save_store_platform(
                "compat",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &save),
            "new save creation failed"))
        return 1;

    if (!check(
            save.counter == 1u
            && save.selected_slot == 1u
            && save.status == REMASTER_EMERALD_SAVE_OK,
            "successful save metadata mismatch"))
        return 1;

    before_counter = save.counter;
    before_last_sector = save.last_written_sector;
    before_selected_slot = save.selected_slot;
    before_status = save.status;

    state.read_mode = READ_ERROR;
    state.write_ok = 1;
    state.writes = 0;
    if (!check(
            !remaster_emerald_save_store_platform(
                "compat",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &save),
            "read I/O error must block overwrite"))
        return 1;
    if (!check(state.writes == 0, "read error unexpectedly wrote a save"))
        return 1;
    if (!check(
            save.counter == before_counter
            && save.last_written_sector == before_last_sector
            && save.selected_slot == before_selected_slot
            && save.status == before_status,
            "read error mutated in-memory save metadata"))
        return 1;

    state.read_mode = READ_WRONG_SIZE;
    state.writes = 0;
    if (!check(
            !remaster_emerald_save_store_platform(
                "compat",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &save),
            "wrong-size existing save must not be overwritten"))
        return 1;
    if (!check(state.writes == 0, "wrong-size save unexpectedly overwritten"))
        return 1;

    state.read_mode = READ_OK;
    state.size = REMASTER_EMERALD_SAVE_IMAGE_BYTES;
    state.write_ok = 0;
    state.writes = 0;
    before_counter = save.counter;
    before_last_sector = save.last_written_sector;
    before_selected_slot = save.selected_slot;
    before_status = save.status;

    if (!check(
            !remaster_emerald_save_store_platform(
                "compat",
                scratch,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &save),
            "write failure must be reported"))
        return 1;
    if (!check(state.writes == 1, "write failure path did not attempt write"))
        return 1;
    if (!check(
            save.counter == before_counter
            && save.last_written_sector == before_last_sector
            && save.selected_slot == before_selected_slot
            && save.status == before_status,
            "write failure did not roll back save metadata"))
        return 1;

    free(scratch);
    remaster_platform_install(0);
    puts("R17 platform failure semantics passed.");
    return 0;
}
