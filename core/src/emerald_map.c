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
