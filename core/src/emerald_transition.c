#include "remaster/emerald_transition.h"

uint8_t remaster_emerald_translate_map_weather(
    uint8_t map_weather,
    uint8_t weather_cycle_stage)
{
    static const uint8_t route119_cycle[4] = {
        REMASTER_EMERALD_WEATHER_SUNNY,
        REMASTER_EMERALD_WEATHER_RAIN,
        REMASTER_EMERALD_WEATHER_RAIN_THUNDERSTORM,
        REMASTER_EMERALD_WEATHER_RAIN
    };
    static const uint8_t route123_cycle[4] = {
        REMASTER_EMERALD_WEATHER_SUNNY,
        REMASTER_EMERALD_WEATHER_SUNNY,
        REMASTER_EMERALD_WEATHER_RAIN,
        REMASTER_EMERALD_WEATHER_SUNNY
    };
    const uint8_t stage = (uint8_t)(weather_cycle_stage % 4u);

    if (map_weather <= REMASTER_EMERALD_WEATHER_ABNORMAL)
        return map_weather;

    if (map_weather == REMASTER_EMERALD_WEATHER_ROUTE119_CYCLE)
        return route119_cycle[stage];

    if (map_weather == REMASTER_EMERALD_WEATHER_ROUTE123_CYCLE)
        return route123_cycle[stage];

    return REMASTER_EMERALD_WEATHER_NONE;
}

int remaster_emerald_map_type_is_outdoors(uint8_t map_type)
{
    return map_type == REMASTER_EMERALD_MAP_TYPE_ROUTE
        || map_type == REMASTER_EMERALD_MAP_TYPE_TOWN
        || map_type == REMASTER_EMERALD_MAP_TYPE_UNDERWATER
        || map_type == REMASTER_EMERALD_MAP_TYPE_CITY
        || map_type == REMASTER_EMERALD_MAP_TYPE_OCEAN_ROUTE;
}

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
    size_t target_warp_count)
{
    RemasterEmeraldOverworldState state;
    int16_t player_x;
    int16_t player_y;

    if (save == 0 || target_width <= 0 || target_height <= 0)
        return 0;

    if (!remaster_emerald_overworld_get(save, &state))
        return 0;

    /*
     * Vanilla SetPlayerCoordsFromWarp precedence:
     * 1. valid destination warp id -> coordinates of that warp
     * 2. explicit non-negative x/y
     * 3. center of map
     */
    if (destination.warp_id >= 0
        && (size_t)destination.warp_id < target_warp_count
        && target_warps != 0) {
        player_x = target_warps[(size_t)destination.warp_id].x;
        player_y = target_warps[(size_t)destination.warp_id].y;
    } else if (destination.x >= 0 && destination.y >= 0) {
        player_x = destination.x;
        player_y = destination.y;
    } else {
        player_x = (int16_t)(target_width / 2);
        player_y = (int16_t)(target_height / 2);
    }

    state.map_group = destination.map_group;
    state.map_num = destination.map_num;
    state.warp_id = destination.warp_id;
    state.warp_x = destination.x;
    state.warp_y = destination.y;
    state.player_x = player_x;
    state.player_y = player_y;
    state.map_layout_id = target_layout_id;

    /*
     * Vanilla LoadMapFromWarp order:
     * - clear temporary field event state
     * - translate and save map-header weather
     * - clear Flash usage when entering an outdoor map
     * - derive flashLevel from target cave state and FLAG_SYS_USE_FLASH
     */
    remaster_emerald_clear_temp_field_event_data(save);

    state.weather = remaster_emerald_translate_map_weather(
        target_weather,
        state.weather_cycle_stage);

    if (remaster_emerald_map_type_is_outdoors(target_map_type)) {
        if (!remaster_emerald_flag_set(
                save,
                REMASTER_EMERALD_FLAG_SYS_USE_FLASH,
                0))
            return 0;
    }

    if (!target_requires_flash) {
        state.flash_level = 0;
    } else {
        int flash_in_use = 0;

        if (!remaster_emerald_flag_get(
                save,
                REMASTER_EMERALD_FLAG_SYS_USE_FLASH,
                &flash_in_use))
            return 0;

        state.flash_level = flash_in_use
            ? 1u
            : (uint8_t)(REMASTER_EMERALD_MAX_FLASH_LEVEL - 1u);
    }

    return remaster_emerald_overworld_set(save, &state);
}
