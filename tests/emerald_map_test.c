#include "remaster/emerald_map.h"

#include <stdio.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_map_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    const uint16_t blocks[6] = {
        0x0001,
        0x0402,
        0x1303,
        REMASTER_EMERALD_MAPGRID_UNDEFINED,
        0xF004,
        0x2005
    };
    const uint16_t border[4] = {
        0x0010,
        0x0011,
        0x0012,
        0x0013
    };
    RemasterEmeraldMapView map;

    map.width = 3;
    map.height = 2;
    map.blocks = blocks;
    map.block_count = 6;
    map.border = border;
    map.border_count = 4;

    if (!check(remaster_emerald_map_is_valid(&map), "map should be valid"))
        return 1;

    if (!check(
            remaster_emerald_map_metatile_at(&map, 2, 0) == 3,
            "metatile mask mismatch"))
        return 1;

    if (!check(
            remaster_emerald_map_collision_at(&map, 1, 0) == 1,
            "collision bits mismatch"))
        return 1;

    if (!check(
            remaster_emerald_map_elevation_at(&map, 2, 0) == 1,
            "elevation bits mismatch"))
        return 1;

    if (!check(
            !remaster_emerald_map_base_can_enter(&map, 1, 0, 0),
            "nonzero collision must block"))
        return 1;

    if (!check(
            remaster_emerald_map_base_can_enter(&map, 2, 0, 1),
            "matching elevation should allow entry"))
        return 1;

    if (!check(
            !remaster_emerald_map_base_can_enter(&map, 2, 0, 2),
            "mismatched elevation should block"))
        return 1;

    if (!check(
            remaster_emerald_map_base_can_enter(&map, 1, 1, 7),
            "elevation 15 must behave as wildcard"))
        return 1;

    if (!check(
            remaster_emerald_elevations_compatible(0, 7)
            && remaster_emerald_elevations_compatible(4, 4)
            && !remaster_emerald_elevations_compatible(4, 5),
            "object elevation compatibility mismatch"))
        return 1;

    /*
     * In-map MAPGRID_UNDEFINED uses the repeating border metatile but remains
     * collision-blocked and elevation 0.
     * x=0,y=1 => border index ((1)&1) + ((2)&1)*2 = 1.
     */
    if (!check(
            remaster_emerald_map_metatile_at(&map, 0, 1) == 0x11,
            "undefined metatile did not borrow border"))
        return 1;
    if (!check(
            remaster_emerald_map_collision_at(&map, 0, 1) == 1,
            "undefined in-map block must collide"))
        return 1;

    /*
     * Outside map uses repeating 2x2 border and forcibly ORs collision mask.
     * x=-1,y=-1 => index 0.
     */
    if (!check(
            remaster_emerald_map_metatile_at(&map, -1, -1) == 0x10,
            "outside border metatile mismatch"))
        return 1;
    if (!check(
            remaster_emerald_map_collision_at(&map, -1, -1) == 3,
            "outside border collision mask mismatch"))
        return 1;

    puts("Emerald map-grid compatibility test passed.");
    return 0;
}
