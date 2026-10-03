#include "remaster/emerald_events.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_events_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldObjectEventDef object_event;
    RemasterEmeraldWarpEventDef warps[2];
    RemasterEmeraldCoordEventDef coords[5];
    RemasterEmeraldCoordMatch match;
    RemasterEmeraldBackgroundEventDef backgrounds[3];
    RemasterEmeraldBackgroundMatch bg_match;
    size_t index = 99u;

    memset(&save, 0, sizeof(save));
    memset(&object_event, 0, sizeof(object_event));
    memset(warps, 0, sizeof(warps));
    memset(coords, 0, sizeof(coords));
    memset(backgrounds, 0, sizeof(backgrounds));

    object_event.local_id = 1;
    object_event.flag_id = 0;

    if (!check(
            remaster_emerald_object_event_visible(&save, &object_event),
            "flag 0 object should always be visible"))
        return 1;

    object_event.flag_id = 0x123;
    if (!check(
            remaster_emerald_object_event_visible(&save, &object_event),
            "clear hide flag should leave object visible"))
        return 1;

    if (!check(
            remaster_emerald_flag_set(&save, 0x123, 1),
            "failed to set hide flag"))
        return 1;

    if (!check(
            !remaster_emerald_object_event_visible(&save, &object_event),
            "set hide flag should hide object"))
        return 1;

    warps[0].x = 4;
    warps[0].y = 5;
    warps[0].elevation = 3;
    warps[0].dest_map_group = 1;
    warps[0].dest_map_num = 2;
    warps[0].dest_warp_id = 1;

    warps[1].x = 4;
    warps[1].y = 5;
    warps[1].elevation = 0;
    warps[1].dest_map_group = 7;
    warps[1].dest_map_num = 8;
    warps[1].dest_warp_id = 2;

    if (!check(
            remaster_emerald_find_warp(
                warps,
                2,
                4,
                5,
                3,
                &index)
            && index == 0,
            "exact elevation warp should win by array order"))
        return 1;

    if (!check(
            remaster_emerald_find_warp(
                warps,
                2,
                4,
                5,
                9,
                &index)
            && index == 1,
            "elevation 0 warp should act as wildcard"))
        return 1;

    if (!check(
            !remaster_emerald_find_warp(
                warps,
                2,
                9,
                9,
                3,
                &index),
            "unmatched warp should not resolve"))
        return 1;

    coords[0].kind = REMASTER_EMERALD_COORD_TRIGGER;
    coords[0].x = 1;
    coords[0].y = 2;
    coords[0].elevation = 3;
    coords[0].trigger = 0x4007;
    coords[0].index = 3;

    coords[1].kind = REMASTER_EMERALD_COORD_TRIGGER;
    coords[1].x = 1;
    coords[1].y = 2;
    coords[1].elevation = 3;
    coords[1].trigger = 0x4007;
    coords[1].index = 4;

    coords[2].kind = REMASTER_EMERALD_COORD_TRIGGER;
    coords[2].x = 8;
    coords[2].y = 8;
    coords[2].elevation = 0;
    coords[2].trigger = 0;

    coords[3].kind = REMASTER_EMERALD_COORD_WEATHER;
    coords[3].x = 6;
    coords[3].y = 7;
    coords[3].elevation = 0;
    coords[3].weather = 12;

    /*
     * Vanilla stores index as u16 in the map structure but compares it as
     * (u8)index. A high byte in generated/native data must not change the
     * trigger result.
     */
    coords[4].kind = REMASTER_EMERALD_COORD_TRIGGER;
    coords[4].x = 9;
    coords[4].y = 10;
    coords[4].elevation = 0;
    coords[4].trigger = 0x4007;
    coords[4].index = 0x0104;

    if (!check(
            remaster_emerald_var_set(&save, 0x4007, 4),
            "failed to set coord var"))
        return 1;

    match = remaster_emerald_find_coord_event(
        &save,
        coords,
        4,
        1,
        2,
        3);

    if (!check(
            match.kind == REMASTER_EMERALD_COORD_MATCH_SCRIPT
            && match.event_index == 1,
            "coord event should match persisted var/index"))
        return 1;

    match = remaster_emerald_find_coord_event(
        &save,
        coords,
        4,
        8,
        8,
        5);

    if (!check(
            match.kind == REMASTER_EMERALD_COORD_MATCH_SCRIPT
            && match.event_index == 2,
            "trigger 0 should run immediately with elevation wildcard"))
        return 1;

    match = remaster_emerald_find_coord_event(
        &save,
        coords,
        4,
        6,
        7,
        4);

    if (!check(
            match.kind == REMASTER_EMERALD_COORD_MATCH_WEATHER
            && match.event_index == 3
            && match.weather == 12,
            "weather coord event mismatch"))
        return 1;

    match = remaster_emerald_find_coord_event(
        &save,
        coords,
        5,
        9,
        10,
        6);

    if (!check(
            match.kind == REMASTER_EMERALD_COORD_MATCH_SCRIPT
            && match.event_index == 4,
            "coord index must compare with Vanilla u8 truncation"))
        return 1;

    backgrounds[0].x = 2;
    backgrounds[0].y = 3;
    backgrounds[0].elevation = 0;
    backgrounds[0].kind = REMASTER_EMERALD_BG_FACING_NORTH;

    bg_match = remaster_emerald_find_background_event(
        &save,
        backgrounds,
        1,
        2,
        3,
        5,
        REMASTER_EMERALD_DIR_NORTH);

    if (!check(
            bg_match.kind == REMASTER_EMERALD_BG_MATCH_SCRIPT
            && bg_match.event_index == 0,
            "north-facing sign should resolve"))
        return 1;

    bg_match = remaster_emerald_find_background_event(
        &save,
        backgrounds,
        1,
        2,
        3,
        5,
        REMASTER_EMERALD_DIR_SOUTH);

    if (!check(
            bg_match.kind == REMASTER_EMERALD_BG_MATCH_NONE,
            "wrong-facing sign should not resolve"))
        return 1;

    backgrounds[1].x = 4;
    backgrounds[1].y = 5;
    backgrounds[1].elevation = 3;
    backgrounds[1].kind = REMASTER_EMERALD_BG_HIDDEN_ITEM;
    backgrounds[1].item_id = 16;
    backgrounds[1].hidden_flag_id = 0x1F4;

    bg_match = remaster_emerald_find_background_event(
        &save,
        &backgrounds[1],
        1,
        4,
        5,
        3,
        REMASTER_EMERALD_DIR_NORTH);

    if (!check(
            bg_match.kind == REMASTER_EMERALD_BG_MATCH_HIDDEN_ITEM
            && bg_match.item_id == 16
            && bg_match.hidden_flag_id == 0x1F4,
            "uncollected hidden item should resolve"))
        return 1;

    if (!check(
            remaster_emerald_flag_set(&save, 0x1F4, 1),
            "hidden-item flag set failed"))
        return 1;

    bg_match = remaster_emerald_find_background_event(
        &save,
        &backgrounds[1],
        1,
        4,
        5,
        3,
        REMASTER_EMERALD_DIR_NORTH);

    if (!check(
            bg_match.kind == REMASTER_EMERALD_BG_MATCH_NONE,
            "collected hidden item should not resolve"))
        return 1;

    backgrounds[2].x = 7;
    backgrounds[2].y = 8;
    backgrounds[2].elevation = 0;
    backgrounds[2].kind = REMASTER_EMERALD_BG_SECRET_BASE;
    backgrounds[2].secret_base_id = 33;

    bg_match = remaster_emerald_find_background_event(
        &save,
        &backgrounds[2],
        1,
        7,
        8,
        2,
        REMASTER_EMERALD_DIR_NORTH);

    if (!check(
            bg_match.kind == REMASTER_EMERALD_BG_MATCH_SECRET_BASE
            && bg_match.secret_base_id == 33,
            "north-facing secret base should resolve"))
        return 1;

    puts("Emerald map-event compatibility test passed.");
    return 0;
}
