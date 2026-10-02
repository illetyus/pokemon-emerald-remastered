#ifndef REMASTER_EMERALD_MAP_H
#define REMASTER_EMERALD_MAP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_MAPGRID_METATILE_MASK = 0x03FF,
    REMASTER_EMERALD_MAPGRID_COLLISION_MASK = 0x0C00,
    REMASTER_EMERALD_MAPGRID_ELEVATION_MASK = 0xF000,
    REMASTER_EMERALD_MAPGRID_COLLISION_SHIFT = 10,
    REMASTER_EMERALD_MAPGRID_ELEVATION_SHIFT = 12,
    REMASTER_EMERALD_MAPGRID_UNDEFINED = 0x03FF
};

typedef struct RemasterEmeraldMapView {
    int32_t width;
    int32_t height;

    const uint16_t *blocks;
    size_t block_count;

    const uint16_t *border;
    size_t border_count;
} RemasterEmeraldMapView;

int remaster_emerald_map_is_valid(
    const RemasterEmeraldMapView *map);

uint16_t remaster_emerald_map_raw_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y);

uint16_t remaster_emerald_map_metatile_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y);

uint8_t remaster_emerald_map_collision_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y);

uint8_t remaster_emerald_map_elevation_at(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y);

int remaster_emerald_elevations_compatible(
    uint8_t a,
    uint8_t b);

int remaster_emerald_map_elevation_mismatch(
    uint8_t current_elevation,
    uint8_t target_elevation);

int remaster_emerald_map_base_can_enter(
    const RemasterEmeraldMapView *map,
    int32_t x,
    int32_t y,
    uint8_t current_elevation);

#ifdef __cplusplus
}
#endif

#endif
