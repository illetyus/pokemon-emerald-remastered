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


static int connection_coord_matches(
    int32_t coord,
    int32_t source_max,
    int32_t destination_max,
    int32_t offset)
{
    int32_t start = offset;
    int32_t end = source_max;

    if (start < 0)
        start = 0;

    if (destination_max + offset < end)
        end = destination_max + offset;

    return start <= coord && coord <= end;
}

int remaster_emerald_find_incoming_connection(
    const RemasterEmeraldConnectionDef *connections,
    size_t connection_count,
    uint8_t direction,
    int16_t player_x,
    int16_t player_y,
    int16_t source_width,
    int16_t source_height,
    size_t *out_index)
{
    size_t i;

    if (connections == 0 || out_index == 0
        || source_width <= 0 || source_height <= 0)
        return 0;

    for (i = 0; i < connection_count; ++i) {
        const RemasterEmeraldConnectionDef *connection = &connections[i];
        int matches = 0;

        if (connection->direction != direction
            || connection->dest_width <= 0
            || connection->dest_height <= 0)
            continue;

        switch (direction) {
        case REMASTER_EMERALD_DIR_SOUTH:
        case REMASTER_EMERALD_DIR_NORTH:
            matches = connection_coord_matches(
                player_x,
                source_width,
                connection->dest_width,
                connection->offset);
            break;

        case REMASTER_EMERALD_DIR_WEST:
        case REMASTER_EMERALD_DIR_EAST:
            matches = connection_coord_matches(
                player_y,
                source_height,
                connection->dest_height,
                connection->offset);
            break;

        default:
            break;
        }

        if (matches) {
            *out_index = i;
            return 1;
        }
    }

    return 0;
}

int remaster_emerald_apply_connection_transition(
    RemasterEmeraldSave *save,
    const RemasterEmeraldConnectionDef *connection,
    uint16_t target_layout_id,
    uint8_t target_weather,
    uint8_t target_map_type,
    int target_requires_flash)
{
    RemasterEmeraldOverworldState state;

    (void)target_map_type;

    if (save == 0 || connection == 0
        || connection->dest_width <= 0
        || connection->dest_height <= 0)
        return 0;

    if (!remaster_emerald_overworld_get(save, &state))
        return 0;

    switch (connection->direction) {
    case REMASTER_EMERALD_DIR_EAST:
        state.player_x = 0;
        state.player_y = (int16_t)(state.player_y - connection->offset);
        break;

    case REMASTER_EMERALD_DIR_WEST:
        state.player_x = (int16_t)(connection->dest_width - 1);
        state.player_y = (int16_t)(state.player_y - connection->offset);
        break;

    case REMASTER_EMERALD_DIR_SOUTH:
        state.player_x = (int16_t)(state.player_x - connection->offset);
        state.player_y = 0;
        break;

    case REMASTER_EMERALD_DIR_NORTH:
        state.player_x = (int16_t)(state.player_x - connection->offset);
        state.player_y = (int16_t)(connection->dest_height - 1);
        break;

    default:
        return 0;
    }

    if (state.player_x < 0
        || state.player_y < 0
        || state.player_x >= connection->dest_width
        || state.player_y >= connection->dest_height)
        return 0;

    state.map_group = (int8_t)connection->dest_map_group;
    state.map_num = (int8_t)connection->dest_map_num;
    state.warp_id = -1;
    state.warp_x = -1;
    state.warp_y = -1;
    state.map_layout_id = target_layout_id;

    remaster_emerald_clear_temp_field_event_data(save);

    state.weather = remaster_emerald_translate_map_weather(
        target_weather,
        state.weather_cycle_stage);
    state.saved_music = 0u;

    /*
     * LoadMapFromCameraTransition calls SetDefaultFlashLevel but does not
     * perform LoadMapFromWarp's outdoor FlagClear(FLAG_SYS_USE_FLASH).
     */
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
    state.saved_music = 0u; /* MUS_DUMMY */

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
