#include "remaster/emerald_rtc.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *status_name(RemasterEmeraldSaveStatus status)
{
    switch (status) {
    case REMASTER_EMERALD_SAVE_EMPTY:
        return "empty";
    case REMASTER_EMERALD_SAVE_OK:
        return "ok";
    case REMASTER_EMERALD_SAVE_DEGRADED:
        return "degraded";
    case REMASTER_EMERALD_SAVE_CORRUPT:
    default:
        return "corrupt";
    }
}

static int read_image(const char *path, uint8_t *image)
{
    FILE *file;
    size_t size;

    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "cannot open input: %s\n", path);
        return 0;
    }

    size = fread(image, 1, REMASTER_EMERALD_SAVE_IMAGE_BYTES, file);

    if (size != REMASTER_EMERALD_SAVE_IMAGE_BYTES) {
        fprintf(
            stderr,
            "input must be exactly %u bytes; read %lu\n",
            (unsigned)REMASTER_EMERALD_SAVE_IMAGE_BYTES,
            (unsigned long)size);
        fclose(file);
        return 0;
    }

    if (fgetc(file) != EOF) {
        fprintf(
            stderr,
            "input is larger than %u bytes\n",
            (unsigned)REMASTER_EMERALD_SAVE_IMAGE_BYTES);
        fclose(file);
        return 0;
    }

    if (ferror(file)) {
        fprintf(stderr, "failed while reading input: %s\n", path);
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

static int write_image(
    const char *path,
    const uint8_t *image)
{
    FILE *file;
    size_t written;

    file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "cannot open output: %s\n", path);
        return 0;
    }

    written = fwrite(
        image,
        1,
        REMASTER_EMERALD_SAVE_IMAGE_BYTES,
        file);

    if (written != REMASTER_EMERALD_SAVE_IMAGE_BYTES || fclose(file) != 0) {
        fprintf(stderr, "failed to write output: %s\n", path);
        return 0;
    }

    return 1;
}

static void print_warp(
    const char *name,
    const RemasterEmeraldWarpState *warp)
{
    printf(
        "%s=%d,%d,%d,%d,%d\n",
        name,
        (int)warp->map_group,
        (int)warp->map_num,
        (int)warp->warp_id,
        (int)warp->x,
        (int)warp->y);
}

static int print_summary(
    const RemasterEmeraldSave *save)
{
    RemasterEmeraldOverworldState state;
    RemasterEmeraldWarpState continue_warp;
    RemasterEmeraldWarpState dynamic_warp;
    RemasterEmeraldWarpState heal_warp;
    RemasterEmeraldWarpState escape_warp;
    RemasterEmeraldTime local_offset;
    RemasterEmeraldTime last_berry;

    memset(&state, 0, sizeof(state));
    memset(&continue_warp, 0, sizeof(continue_warp));
    memset(&dynamic_warp, 0, sizeof(dynamic_warp));
    memset(&heal_warp, 0, sizeof(heal_warp));
    memset(&escape_warp, 0, sizeof(escape_warp));

    if (!remaster_emerald_overworld_get(save, &state)
        || !remaster_emerald_continue_game_warp_get(save, &continue_warp)
        || !remaster_emerald_dynamic_warp_get(save, &dynamic_warp)
        || !remaster_emerald_last_heal_warp_get(save, &heal_warp)
        || !remaster_emerald_escape_warp_get(save, &escape_warp)) {
        fprintf(stderr, "failed to decode overworld state\n");
        return 0;
    }

    local_offset =
        remaster_emerald_save_get_local_time_offset(save);
    last_berry =
        remaster_emerald_save_get_last_berry_update(save);

    printf("status=%s\n", status_name(save->status));
    printf("counter=%lu\n", (unsigned long)save->counter);
    printf("selected_slot=%u\n", (unsigned)save->selected_slot);
    printf(
        "last_written_sector=%u\n",
        (unsigned)save->last_written_sector);

    printf(
        "player=%d,%d\n",
        (int)state.player_x,
        (int)state.player_y);
    printf(
        "map=%d,%d\n",
        (int)state.map_group,
        (int)state.map_num);
    printf(
        "location_warp=%d,%d,%d\n",
        (int)state.warp_id,
        (int)state.warp_x,
        (int)state.warp_y);
    printf(
        "layout=%u\n",
        (unsigned)state.map_layout_id);
    printf(
        "saved_music=%u\n",
        (unsigned)state.saved_music);
    printf(
        "weather=%u\n",
        (unsigned)state.weather);
    printf(
        "weather_cycle_stage=%u\n",
        (unsigned)state.weather_cycle_stage);
    printf(
        "flash_level=%u\n",
        (unsigned)state.flash_level);
    printf(
        "party_count=%u\n",
        (unsigned)state.party_count);
    printf(
        "money=%lu\n",
        (unsigned long)state.money);
    printf(
        "coins=%u\n",
        (unsigned)state.coins);
    printf(
        "registered_item=%u\n",
        (unsigned)state.registered_item);

    print_warp("continue_game_warp", &continue_warp);
    print_warp("dynamic_warp", &dynamic_warp);
    print_warp("last_heal_warp", &heal_warp);
    print_warp("escape_warp", &escape_warp);

    printf(
        "local_time_offset=%d,%d,%d,%d\n",
        (int)local_offset.days,
        (int)local_offset.hours,
        (int)local_offset.minutes,
        (int)local_offset.seconds);
    printf(
        "last_berry_update=%d,%d,%d,%d\n",
        (int)last_berry.days,
        (int)last_berry.hours,
        (int)last_berry.minutes,
        (int)last_berry.seconds);

    return 1;
}

static int equivalent_state(
    const RemasterEmeraldSave *before,
    const RemasterEmeraldSave *after)
{
    if (before == NULL || after == NULL)
        return 0;

    /*
     * All R1 gameplay/RTC/warp state lives inside these authoritative
     * payloads. Compare bytes rather than C structs so compiler padding
     * cannot affect verification.
     */
    return memcmp(
        before->save_block1,
        after->save_block1,
        REMASTER_EMERALD_SAVE_BLOCK1_BYTES) == 0
        && memcmp(
            before->save_block2,
            after->save_block2,
            REMASTER_EMERALD_SAVE_BLOCK2_BYTES) == 0
        && memcmp(
            before->pokemon_storage,
            after->pokemon_storage,
            REMASTER_EMERALD_STORAGE_BYTES) == 0;
}

static void usage(const char *program)
{
    fprintf(
        stderr,
        "usage: %s <input.sav> [--rewrite <output.sav>]\n",
        program);
}

int main(int argc, char **argv)
{
    uint8_t *image;
    RemasterEmeraldSave save;
    RemasterEmeraldSave rewritten;
    RemasterEmeraldSaveStatus status;
    const char *rewrite_path = NULL;
    int result = 1;

    if (argc != 2 && argc != 4) {
        usage(argv[0]);
        return 2;
    }

    if (argc == 4) {
        if (strcmp(argv[2], "--rewrite") != 0) {
            usage(argv[0]);
            return 2;
        }
        rewrite_path = argv[3];

        if (strcmp(argv[1], rewrite_path) == 0) {
            fprintf(
                stderr,
                "refusing to overwrite the input save; choose a new output path\n");
            return 2;
        }
    }

    image = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (image == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    memset(&save, 0, sizeof(save));
    memset(&rewritten, 0, sizeof(rewritten));

    if (!read_image(argv[1], image))
        goto cleanup;

    status = remaster_emerald_save_decode(
        image,
        REMASTER_EMERALD_SAVE_IMAGE_BYTES,
        &save);

    if (status != REMASTER_EMERALD_SAVE_OK
        && status != REMASTER_EMERALD_SAVE_DEGRADED) {
        fprintf(
            stderr,
            "save decode failed: %s\n",
            status_name(status));
        goto cleanup;
    }

    if (!print_summary(&save))
        goto cleanup;

    if (rewrite_path != NULL) {
        const uint32_t previous_counter = save.counter;

        if (!remaster_emerald_save_encode_next(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &save)) {
            fprintf(stderr, "rewrite encode failed\n");
            goto cleanup;
        }

        if (!write_image(rewrite_path, image))
            goto cleanup;

        /*
         * Verify the bytes actually persisted to disk, not merely the
         * in-memory buffer that was passed to fwrite.
         */
        memset(image, 0, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
        if (!read_image(rewrite_path, image))
            goto cleanup;

        status = remaster_emerald_save_decode(
            image,
            REMASTER_EMERALD_SAVE_IMAGE_BYTES,
            &rewritten);

        if (status != REMASTER_EMERALD_SAVE_OK
            && status != REMASTER_EMERALD_SAVE_DEGRADED) {
            fprintf(
                stderr,
                "rewritten save failed to decode: %s\n",
                status_name(status));
            goto cleanup;
        }

        /*
         * save now contains the same gameplay payload but its slot metadata
         * has advanced. Compare decoded gameplay/save blocks, not slot meta.
         */
        if (!equivalent_state(&save, &rewritten)) {
            fprintf(
                stderr,
                "rewritten save gameplay payload differs from source\n");
            goto cleanup;
        }

        printf(
            "rewrite_verified=1\n"
            "rewrite_previous_counter=%lu\n"
            "rewrite_counter=%lu\n"
            "rewrite_path=%s\n",
            (unsigned long)previous_counter,
            (unsigned long)rewritten.counter,
            rewrite_path);
    }

    result = 0;

cleanup:
    free(image);
    return result;
}
