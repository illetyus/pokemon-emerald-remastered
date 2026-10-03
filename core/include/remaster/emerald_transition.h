#ifndef REMASTER_EMERALD_TRANSITION_H
#define REMASTER_EMERALD_TRANSITION_H

#include "remaster/emerald_events.h"
#include "remaster/emerald_map.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REMASTER_EMERALD_MAP_TYPE_NONE = 0,
    REMASTER_EMERALD_MAP_TYPE_TOWN = 1,
    REMASTER_EMERALD_MAP_TYPE_CITY = 2,
    REMASTER_EMERALD_MAP_TYPE_ROUTE = 3,
    REMASTER_EMERALD_MAP_TYPE_UNDERGROUND = 4,
    REMASTER_EMERALD_MAP_TYPE_UNDERWATER = 5,
    REMASTER_EMERALD_MAP_TYPE_OCEAN_ROUTE = 6,
    REMASTER_EMERALD_MAP_TYPE_UNKNOWN = 7,
    REMASTER_EMERALD_MAP_TYPE_INDOOR = 8,
    REMASTER_EMERALD_MAP_TYPE_SECRET_BASE = 9,

    REMASTER_EMERALD_WEATHER_NONE = 0,
    REMASTER_EMERALD_WEATHER_SUNNY_CLOUDS = 1,
    REMASTER_EMERALD_WEATHER_SUNNY = 2,
    REMASTER_EMERALD_WEATHER_RAIN = 3,
    REMASTER_EMERALD_WEATHER_SNOW = 4,
    REMASTER_EMERALD_WEATHER_RAIN_THUNDERSTORM = 5,
    REMASTER_EMERALD_WEATHER_FOG_HORIZONTAL = 6,
    REMASTER_EMERALD_WEATHER_VOLCANIC_ASH = 7,
    REMASTER_EMERALD_WEATHER_SANDSTORM = 8,
    REMASTER_EMERALD_WEATHER_FOG_DIAGONAL = 9,
    REMASTER_EMERALD_WEATHER_UNDERWATER = 10,
    REMASTER_EMERALD_WEATHER_SHADE = 11,
    REMASTER_EMERALD_WEATHER_DROUGHT = 12,
    REMASTER_EMERALD_WEATHER_DOWNPOUR = 13,
    REMASTER_EMERALD_WEATHER_UNDERWATER_BUBBLES = 14,
    REMASTER_EMERALD_WEATHER_ABNORMAL = 15,
    REMASTER_EMERALD_WEATHER_ROUTE119_CYCLE = 20,
    REMASTER_EMERALD_WEATHER_ROUTE123_CYCLE = 21,

    REMASTER_EMERALD_FLAG_SYS_USE_FLASH = 0x888,
    REMASTER_EMERALD_MAX_FLASH_LEVEL = 8
};

uint8_t remaster_emerald_translate_map_weather(
    uint8_t map_weather,
    uint8_t weather_cycle_stage);

int remaster_emerald_map_type_is_outdoors(uint8_t map_type);

typedef struct RemasterEmeraldConnectionDef {
    uint8_t direction;
    int32_t offset;
    uint8_t dest_map_group;
    uint8_t dest_map_num;
    int16_t dest_width;
    int16_t dest_height;
} RemasterEmeraldConnectionDef;

/*
 * Mirrors Vanilla GetIncomingConnection/IsCoordInIncomingConnectingMap.
 * The source player position and dimensions are map-local, matching
 * SaveBlock1.pos rather than the GBA MAP_OFFSET-expanded grid.
 */
int remaster_emerald_find_incoming_connection(
    const RemasterEmeraldConnectionDef *connections,
    size_t connection_count,
    uint8_t direction,
    int16_t player_x,
    int16_t player_y,
    int16_t source_width,
    int16_t source_height,
    size_t *out_index);

/*
 * Mirrors the stateful result of CameraMove + LoadMapFromCameraTransition.
 * Unlike a normal warp, a camera/connection transition does not clear
 * FLAG_SYS_USE_FLASH merely because the destination is outdoors.
 */
int remaster_emerald_apply_connection_transition(
    RemasterEmeraldSave *save,
    const RemasterEmeraldConnectionDef *connection,
    uint16_t target_layout_id,
    uint8_t target_weather,
    uint8_t target_map_type,
    int target_requires_flash);

/*
 * Mirrors the stateful part of Vanilla WarpIntoMap:
 * - apply destination WarpData to SaveBlock1.location
 * - load target map layout identity
 * - choose player coords from target warp id, explicit coords, or map center
 * - clear temporary flags/vars for the newly entered map
 *
 * Map data itself is supplied by the platform-neutral world catalog.
 */
int remaster_emerald_apply_warp(
    RemasterEmeraldSave *save,
    RemasterEmeraldWarpState destination,
    uint16_t target_layout_id,
    uint8_t target_weather,
    uint8_t target_map_type,
    int target_requires_flash,
    int16_t target_width,
    int16_t target_height,
    const RemasterEmeraldWarpEventDef *target_warps,
    size_t target_warp_count);

#ifdef __cplusplus
}
#endif

#endif
