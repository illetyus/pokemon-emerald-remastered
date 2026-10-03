#include "remaster/emerald_map.h"

static int map_in_bounds(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    return map != 0
        && x >= 0
        && y >= 0
        && x < map->width
        && y < map->height;
}

int remaster_emerald_map_is_valid(
    const RemasterEmeraldMapView *map)
{
    if (map == 0
        || map->width <= 0
        || map->height <= 0
        || map->blocks == 0
        || map->border == 0
        || map->border_count < 4u)
        return 0;

    return map->block_count
        >= (size_t)map->width * (size_t)map->height;
}

static uint16_t map_border_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    uint32_t ix;
    uint32_t iy;
    size_t index;

    if (!remaster_emerald_map_is_valid(map))
        return REMASTER_EMERALD_MAPGRID_UNDEFINED;

    /*
     * Vanilla Emerald:
     *   i = (x + 1) & 1;
     *   i += ((y + 1) & 1) * 2;
     */
    ix = ((uint32_t)(x + 1)) & 1u;
    iy = ((uint32_t)(y + 1)) & 1u;
    index = (size_t)ix + (size_t)iy * 2u;

    return (uint16_t)(
        map->border[index]
        | REMASTER_EMERALD_MAPGRID_COLLISION_MASK);
}

uint16_t remaster_emerald_map_raw_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    size_t index;

    if (!remaster_emerald_map_is_valid(map))
        return REMASTER_EMERALD_MAPGRID_UNDEFINED;

    if (!map_in_bounds(map, x, y))
        return map_border_at(map, x, y);

    index = (size_t)y * (size_t)map->width + (size_t)x;
    return map->blocks[index];
}

uint16_t remaster_emerald_map_metatile_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    const uint16_t block = remaster_emerald_map_raw_at(map, x, y);

    /*
     * An undefined in-map word borrows the repeating border's metatile id,
     * matching MapGridGetMetatileIdAt.
     */
    if (block == REMASTER_EMERALD_MAPGRID_UNDEFINED)
        return (uint16_t)(
            map_border_at(map, x, y)
            & REMASTER_EMERALD_MAPGRID_METATILE_MASK);

    return (uint16_t)(
        block & REMASTER_EMERALD_MAPGRID_METATILE_MASK);
}

uint8_t remaster_emerald_map_collision_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    const uint16_t block = remaster_emerald_map_raw_at(map, x, y);

    if (block == REMASTER_EMERALD_MAPGRID_UNDEFINED)
        return 1u;

    return (uint8_t)(
        (block & REMASTER_EMERALD_MAPGRID_COLLISION_MASK)
        >> REMASTER_EMERALD_MAPGRID_COLLISION_SHIFT);
}

uint8_t remaster_emerald_map_elevation_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    const uint16_t block = remaster_emerald_map_raw_at(map, x, y);

    if (block == REMASTER_EMERALD_MAPGRID_UNDEFINED)
        return 0u;

    return (uint8_t)(
        (block & REMASTER_EMERALD_MAPGRID_ELEVATION_MASK)
        >> REMASTER_EMERALD_MAPGRID_ELEVATION_SHIFT);
}

uint16_t remaster_emerald_metatile_attributes(
    const RemasterEmeraldMapView *map,
    uint16_t metatile_id)
{
    if (map == 0 || metatile_id >= 1024u)
        return UINT16_C(0x00FF);

    if (metatile_id < 512u) {
        if (map->primary_attributes == 0
            || metatile_id >= map->primary_attribute_count)
            return UINT16_C(0x00FF);

        return map->primary_attributes[metatile_id];
    }

    metatile_id = (uint16_t)(metatile_id - 512u);

    if (map->secondary_attributes == 0
        || metatile_id >= map->secondary_attribute_count)
        return UINT16_C(0x00FF);

    return map->secondary_attributes[metatile_id];
}

uint8_t remaster_emerald_metatile_behavior(
    const RemasterEmeraldMapView *map,
    uint16_t metatile_id)
{
    return (uint8_t)(
        remaster_emerald_metatile_attributes(map, metatile_id)
        & UINT16_C(0x00FF));
}

uint8_t remaster_emerald_metatile_layer(
    const RemasterEmeraldMapView *map,
    uint16_t metatile_id)
{
    return (uint8_t)(
        (remaster_emerald_metatile_attributes(map, metatile_id)
            & UINT16_C(0xF000))
        >> 12u);
}

uint8_t remaster_emerald_map_behavior_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    return remaster_emerald_metatile_behavior(
        map,
        remaster_emerald_map_metatile_at(map, x, y));
}

uint8_t remaster_emerald_map_layer_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y)
{
    return remaster_emerald_metatile_layer(
        map,
        remaster_emerald_map_metatile_at(map, x, y));
}

int remaster_emerald_elevations_compatible(
    uint8_t a,
    uint8_t b)
{
    if (a == 0u || b == 0u)
        return 1;

    return a == b;
}

int remaster_emerald_map_elevation_mismatch(
    uint8_t current_elevation,
    uint8_t target_elevation)
{
    if (current_elevation == 0u)
        return 0;

    if (target_elevation == 0u || target_elevation == 15u)
        return 0;

    return target_elevation != current_elevation;
}

int remaster_emerald_behavior_blocks_north(uint8_t behavior)
{
    return behavior == UINT8_C(0x32)
        || behavior == UINT8_C(0x34)
        || behavior == UINT8_C(0x35)
        || behavior == UINT8_C(0xC0);
}

int remaster_emerald_behavior_blocks_south(uint8_t behavior)
{
    return behavior == UINT8_C(0x33)
        || behavior == UINT8_C(0x36)
        || behavior == UINT8_C(0x37)
        || behavior == UINT8_C(0xC0);
}

int remaster_emerald_behavior_blocks_east(uint8_t behavior)
{
    return behavior == UINT8_C(0x30)
        || behavior == UINT8_C(0x34)
        || behavior == UINT8_C(0x36)
        || behavior == UINT8_C(0xC1)
        || behavior == UINT8_C(0xBE);
}

int remaster_emerald_behavior_blocks_west(uint8_t behavior)
{
    return behavior == UINT8_C(0x31)
        || behavior == UINT8_C(0x35)
        || behavior == UINT8_C(0x37)
        || behavior == UINT8_C(0xC1)
        || behavior == UINT8_C(0xBE);
}

int remaster_emerald_directionally_blocked(
    uint8_t current_behavior,
    uint8_t target_behavior,
    uint8_t direction)
{
    switch (direction) {
    case REMASTER_EMERALD_DIR_SOUTH:
        return remaster_emerald_behavior_blocks_south(current_behavior)
            || remaster_emerald_behavior_blocks_north(target_behavior);

    case REMASTER_EMERALD_DIR_NORTH:
        return remaster_emerald_behavior_blocks_north(current_behavior)
            || remaster_emerald_behavior_blocks_south(target_behavior);

    case REMASTER_EMERALD_DIR_WEST:
        return remaster_emerald_behavior_blocks_west(current_behavior)
            || remaster_emerald_behavior_blocks_east(target_behavior);

    case REMASTER_EMERALD_DIR_EAST:
        return remaster_emerald_behavior_blocks_east(current_behavior)
            || remaster_emerald_behavior_blocks_west(target_behavior);

    default:
        return 1;
    }
}

uint8_t remaster_emerald_ledge_jump_direction(
    uint8_t target_behavior,
    uint8_t direction)
{
    switch (direction) {
    case REMASTER_EMERALD_DIR_SOUTH:
        return target_behavior == UINT8_C(0x3B)
            ? REMASTER_EMERALD_DIR_SOUTH
            : REMASTER_EMERALD_DIR_NONE;

    case REMASTER_EMERALD_DIR_NORTH:
        return target_behavior == UINT8_C(0x3A)
            ? REMASTER_EMERALD_DIR_NORTH
            : REMASTER_EMERALD_DIR_NONE;

    case REMASTER_EMERALD_DIR_WEST:
        return target_behavior == UINT8_C(0x39)
            ? REMASTER_EMERALD_DIR_WEST
            : REMASTER_EMERALD_DIR_NONE;

    case REMASTER_EMERALD_DIR_EAST:
        return target_behavior == UINT8_C(0x38)
            ? REMASTER_EMERALD_DIR_EAST
            : REMASTER_EMERALD_DIR_NONE;

    default:
        return REMASTER_EMERALD_DIR_NONE;
    }
}

int remaster_emerald_map_base_can_enter(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y,
    uint8_t current_elevation)
{
    const uint8_t collision =
        remaster_emerald_map_collision_at(map, x, y);
    const uint8_t elevation =
        remaster_emerald_map_elevation_at(map, x, y);

    if (collision != 0u)
        return 0;

    return !remaster_emerald_map_elevation_mismatch(
        current_elevation,
        elevation);
}


int remaster_emerald_map_can_enter_direction(
    const RemasterEmeraldMapView *map,
    int32_t from_x,
    int32_t from_y,
    int32_t target_x,
    int32_t target_y,
    uint8_t direction,
    uint8_t current_elevation)
{
    const uint8_t current_behavior =
        remaster_emerald_map_behavior_at(map, from_x, from_y);
    const uint8_t target_behavior =
        remaster_emerald_map_behavior_at(map, target_x, target_y);

    if (!remaster_emerald_map_base_can_enter(
            map,
            target_x,
            target_y,
            current_elevation))
        return 0;

    return !remaster_emerald_directionally_blocked(
        current_behavior,
        target_behavior,
        direction);
}
