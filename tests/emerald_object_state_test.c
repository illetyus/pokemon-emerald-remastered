#include "remaster/emerald_object_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_object_state_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldObjectTemplate input[2];
    RemasterEmeraldObjectTemplate output;
    uint8_t raw_in[REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES];
    uint8_t raw_out[REMASTER_EMERALD_SAVED_OBJECT_EVENT_BYTES];
    size_t index = 0;
    size_t i;

    memset(&save, 0, sizeof(save));
    memset(input, 0, sizeof(input));

    input[0].local_id = 1;
    input[0].kind = 0;
    input[0].graphics_id = 6;
    input[0].x = 16;
    input[0].y = 10;
    input[0].elevation = 3;
    input[0].movement_type = 2;
    input[0].movement_range_x = 1;
    input[0].movement_range_y = 2;
    input[0].trainer_type = 0;
    input[0].trainer_range_or_berry_tree_id = 0;
    input[0].legacy_script_address = UINT32_C(0x08123456);
    input[0].flag_id = 0x123;

    input[1] = input[0];
    input[1].local_id = 2;
    input[1].graphics_id = 9;
    input[1].x = -4;
    input[1].y = 27;
    input[1].movement_type = 5;
    input[1].movement_range_x = 3;
    input[1].movement_range_y = 4;
    input[1].flag_id = 0;

    if (!check(
            remaster_emerald_object_templates_replace(
                &save,
                input,
                2),
            "template replace failed"))
        return 1;

    if (!check(
            remaster_emerald_object_template_get(
                &save,
                0,
                &output),
            "template get failed"))
        return 1;

    if (!check(
            output.local_id == 1
            && output.kind == 0
            && output.graphics_id == 6
            && output.x == 16
            && output.y == 10
            && output.elevation == 3
            && output.movement_type == 2
            && output.movement_range_x == 1
            && output.movement_range_y == 2
            && output.legacy_script_address == UINT32_C(0x08123456)
            && output.flag_id == 0x123,
            "template field round-trip mismatch"))
        return 1;

    if (!check(
            remaster_emerald_object_template_find_local_id(
                &save,
                2,
                &index)
            && index == 1,
            "local-id lookup mismatch"))
        return 1;

    if (!check(
            remaster_emerald_object_template_set_coords(
                &save,
                2,
                31,
                -8)
            && remaster_emerald_object_template_set_movement_type(
                &save,
                2,
                12)
            && remaster_emerald_object_template_get(
                &save,
                1,
                &output),
            "template mutation failed"))
        return 1;

    if (!check(
            output.x == 31
            && output.y == -8
            && output.movement_type == 12
            && output.graphics_id == 9
            && output.flag_id == 0,
            "template mutation damaged neighboring fields"))
        return 1;

    if (!check(
            remaster_emerald_object_template_get(
                &save,
                2,
                &output)
            && output.local_id == 0
            && output.graphics_id == 0,
            "unused template slot was not cleared"))
        return 1;

    for (i = 0; i < sizeof(raw_in); ++i)
        raw_in[i] = (uint8_t)(i ^ 0xA5u);

    if (!check(
            remaster_emerald_saved_object_event_write(
                &save,
                15,
                raw_in)
            && remaster_emerald_saved_object_event_read(
                &save,
                15,
                raw_out),
            "opaque saved object event round-trip failed"))
        return 1;

    if (!check(
            memcmp(raw_in, raw_out, sizeof(raw_in)) == 0,
            "opaque object event bytes changed"))
        return 1;

    puts("Emerald object save-state compatibility test passed.");
    return 0;
}
