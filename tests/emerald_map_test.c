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
    const uint16_t primary_attributes[4] = {
        0x0001,
        0x1002,
        0xF003,
        0x2004
    };
    const uint16_t secondary_attributes[3] = {
        0x0005,
        0x3006,
        0xF007
    };
    RemasterEmeraldMapView map;

    map.width = 3;
    map.height = 2;
    map.blocks = blocks;
    map.block_count = 6;
    map.border = border;
    map.border_count = 4;
    map.primary_attributes = primary_attributes;
    map.primary_attribute_count = 4;
    map.secondary_attributes = secondary_attributes;
    map.secondary_attribute_count = 3;

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
            remaster_emerald_metatile_attributes(&map, 1) == 0x1002,
            "primary metatile attribute mismatch"))
        return 1;

    if (!check(
            remaster_emerald_metatile_behavior(&map, 2) == 3,
            "primary metatile behavior mismatch"))
        return 1;

    if (!check(
            remaster_emerald_metatile_layer(&map, 2) == 15,
            "primary metatile layer mismatch"))
        return 1;

    if (!check(
            remaster_emerald_metatile_attributes(&map, 512) == 0x0005,
            "secondary metatile attribute mismatch"))
        return 1;

    if (!check(
            remaster_emerald_metatile_behavior(&map, 513) == 6,
            "secondary behavior mismatch"))
        return 1;

    if (!check(
            remaster_emerald_metatile_layer(&map, 514) == 15,
            "secondary layer mismatch"))
        return 1;

    if (!check(
            remaster_emerald_metatile_behavior(&map, 900) == 0xFF,
            "missing attribute must return MB_INVALID"))
        return 1;

    if (!check(
            remaster_emerald_behavior_blocks_north(0x32)
            && remaster_emerald_behavior_blocks_north(0x34)
            && remaster_emerald_behavior_blocks_north(0x35)
            && remaster_emerald_behavior_blocks_north(0xC0)
            && !remaster_emerald_behavior_blocks_north(0x33),
            "north blocked behavior mismatch"))
        return 1;

    if (!check(
            remaster_emerald_behavior_blocks_south(0x33)
            && remaster_emerald_behavior_blocks_south(0x36)
            && remaster_emerald_behavior_blocks_south(0x37)
            && remaster_emerald_behavior_blocks_south(0xC0)
            && !remaster_emerald_behavior_blocks_south(0x32),
            "south blocked behavior mismatch"))
        return 1;

    if (!check(
            remaster_emerald_behavior_blocks_east(0x30)
            && remaster_emerald_behavior_blocks_east(0x34)
            && remaster_emerald_behavior_blocks_east(0x36)
            && remaster_emerald_behavior_blocks_east(0xC1)
            && remaster_emerald_behavior_blocks_east(0xBE),
            "east blocked behavior mismatch"))
        return 1;

    if (!check(
            remaster_emerald_behavior_blocks_west(0x31)
            && remaster_emerald_behavior_blocks_west(0x35)
            && remaster_emerald_behavior_blocks_west(0x37)
            && remaster_emerald_behavior_blocks_west(0xC1)
            && remaster_emerald_behavior_blocks_west(0xBE),
            "west blocked behavior mismatch"))
        return 1;

    if (!check(
            remaster_emerald_directionally_blocked(
                0x33,
                0x00,
                REMASTER_EMERALD_DIR_SOUTH),
            "south edge on current tile should block"))
        return 1;

    if (!check(
            remaster_emerald_directionally_blocked(
                0x00,
                0x32,
                REMASTER_EMERALD_DIR_SOUTH),
            "north edge on target tile should block southward entry"))
        return 1;

    if (!check(
            !remaster_emerald_directionally_blocked(
                0x00,
                0x00,
                REMASTER_EMERALD_DIR_EAST),
            "unblocked eastward transition rejected"))
        return 1;

    if (!check(
            remaster_emerald_ledge_jump_direction(
                0x3B,
                REMASTER_EMERALD_DIR_SOUTH)
                == REMASTER_EMERALD_DIR_SOUTH
            && remaster_emerald_ledge_jump_direction(
                0x38,
                REMASTER_EMERALD_DIR_EAST)
                == REMASTER_EMERALD_DIR_EAST
            && remaster_emerald_ledge_jump_direction(
                0x3A,
                REMASTER_EMERALD_DIR_SOUTH)
                == REMASTER_EMERALD_DIR_NONE,
            "ledge direction mismatch"))
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
