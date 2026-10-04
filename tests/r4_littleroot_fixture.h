#ifndef REMASTER_R4_LITTLEROOT_FIXTURE_H
#define REMASTER_R4_LITTLEROOT_FIXTURE_H

#include "remaster/emerald_events.h"
#include "remaster/emerald_map.h"
#include "remaster/emerald_transition.h"

#include <stddef.h>
#include <stdint.h>

typedef struct RemasterR4FixtureMap {
    const char *id;
    const char *name;
    int32_t group_num;
    int32_t map_num;
    uint16_t layout_num;
    uint8_t weather_id;
    uint8_t map_type_id;
    uint8_t requires_flash;
    RemasterEmeraldMapView view;
    const RemasterEmeraldWarpEventDef *warps;
    size_t warp_count;
    const RemasterEmeraldConnectionDef *connections;
    size_t connection_count;
} RemasterR4FixtureMap;

extern const RemasterR4FixtureMap gRemasterR4LittlerootMaps[];
extern const size_t gRemasterR4LittlerootMapCount;

const RemasterR4FixtureMap *remaster_r4_fixture_find_map(
    int32_t group_num,
    int32_t map_num);

#endif
