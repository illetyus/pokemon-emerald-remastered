#include "remaster/emerald_events.h"
#include "remaster/emerald_map.h"
#include "remaster/emerald_overworld.h"
#include "remaster/emerald_state.h"

static int elevation_matches(uint8_t event_elevation, uint8_t elevation)
{
    return event_elevation == 0u || event_elevation == elevation;
}

int remaster_emerald_object_event_visible(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldObjectEventDef *event)
{
    int hidden = 0;

    if (event == 0)
        return 0;

    /*
     * Vanilla ObjectEvent spawning uses:
     *   !FlagGet(template->flagId)
     * Flag 0 is the always-visible sentinel.
     */
    if (event->flag_id == 0u)
        return 1;

    if (save == 0)
        return 0;

    if (!remaster_emerald_flag_get(save, event->flag_id, &hidden))
        return 0;

    return !hidden;
}

int remaster_emerald_find_warp(
    const RemasterEmeraldWarpEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    size_t *out_index)
{
    size_t i;

    if (events == 0 || out_index == 0)
        return 0;

    for (i = 0; i < event_count; ++i) {
        if (events[i].x != x || events[i].y != y)
            continue;

        if (!elevation_matches(events[i].elevation, elevation))
            continue;

        *out_index = i;
        return 1;
    }

    return 0;
}

RemasterEmeraldCoordMatch remaster_emerald_find_coord_event(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldCoordEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation)
{
    RemasterEmeraldCoordMatch match;
    size_t i;

    match.kind = REMASTER_EMERALD_COORD_MATCH_NONE;
    match.event_index = 0;
    match.weather = 0;

    if (events == 0)
        return match;

    for (i = 0; i < event_count; ++i) {
        uint16_t value = 0;

        if (events[i].x != x || events[i].y != y)
            continue;

        if (!elevation_matches(events[i].elevation, elevation))
            continue;

        if (events[i].kind == REMASTER_EMERALD_COORD_WEATHER) {
            match.kind = REMASTER_EMERALD_COORD_MATCH_WEATHER;
            match.event_index = i;
            match.weather = events[i].weather;
            return match;
        }

        if (events[i].kind != REMASTER_EMERALD_COORD_TRIGGER)
            continue;

        if (events[i].trigger == 0u) {
            match.kind = REMASTER_EMERALD_COORD_MATCH_SCRIPT;
            match.event_index = i;
            return match;
        }

        if (save == 0)
            continue;

        if (!remaster_emerald_var_get(save, events[i].trigger, &value))
            continue;

        /*
         * Vanilla TryRunCoordEventScript compares VarGet(trigger) against
         * (u8)coordEvent->index. Preserve that truncation exactly instead of
         * treating the serialized u16 as a full-width comparison value.
         */
        if (value == (uint16_t)(uint8_t)events[i].index) {
            match.kind = REMASTER_EMERALD_COORD_MATCH_SCRIPT;
            match.event_index = i;
            return match;
        }
    }

    return match;
}


static int background_facing_matches(
    uint8_t kind,
    uint8_t facing_direction)
{
    switch (kind) {
    case REMASTER_EMERALD_BG_FACING_ANY:
        return 1;
    case REMASTER_EMERALD_BG_FACING_NORTH:
        return facing_direction == REMASTER_EMERALD_DIR_NORTH;
    case REMASTER_EMERALD_BG_FACING_SOUTH:
        return facing_direction == REMASTER_EMERALD_DIR_SOUTH;
    case REMASTER_EMERALD_BG_FACING_EAST:
        return facing_direction == REMASTER_EMERALD_DIR_EAST;
    case REMASTER_EMERALD_BG_FACING_WEST:
        return facing_direction == REMASTER_EMERALD_DIR_WEST;
    default:
        return 0;
    }
}

RemasterEmeraldBackgroundMatch remaster_emerald_find_background_event(
    const RemasterEmeraldSave *save,
    const RemasterEmeraldBackgroundEventDef *events,
    size_t event_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    uint8_t facing_direction)
{
    RemasterEmeraldBackgroundMatch match;
    size_t i;

    match.kind = REMASTER_EMERALD_BG_MATCH_NONE;
    match.event_index = 0;
    match.item_id = 0;
    match.hidden_flag_id = 0;
    match.secret_base_id = 0;

    if (events == 0)
        return match;

    for (i = 0; i < event_count; ++i) {
        int collected = 0;

        if (events[i].x != x || events[i].y != y)
            continue;

        if (!elevation_matches(events[i].elevation, elevation))
            continue;

        /*
         * Vanilla GetBackgroundEventAtPosition returns the first matching
         * coordinate/elevation event. A direction mismatch or a collected
         * hidden item therefore resolves to no interaction rather than
         * falling through to a later event at the same tile.
         */
        match.event_index = i;

        if (events[i].kind <= REMASTER_EMERALD_BG_FACING_WEST) {
            if (background_facing_matches(
                    events[i].kind,
                    facing_direction)) {
                match.kind = REMASTER_EMERALD_BG_MATCH_SCRIPT;
            }
            return match;
        }

        if (events[i].kind == REMASTER_EMERALD_BG_HIDDEN_ITEM) {
            if (save == 0 || events[i].hidden_flag_id == 0u)
                return match;

            if (!remaster_emerald_flag_get(
                    save,
                    events[i].hidden_flag_id,
                    &collected))
                return match;

            if (collected)
                return match;

            match.kind = REMASTER_EMERALD_BG_MATCH_HIDDEN_ITEM;
            match.item_id = events[i].item_id;
            match.hidden_flag_id = events[i].hidden_flag_id;
            return match;
        }

        if (events[i].kind == REMASTER_EMERALD_BG_SECRET_BASE) {
            if (facing_direction != REMASTER_EMERALD_DIR_NORTH)
                return match;

            match.kind = REMASTER_EMERALD_BG_MATCH_SECRET_BASE;
            match.secret_base_id = events[i].secret_base_id;
            return match;
        }

        return match;
    }

    return match;
}


int remaster_emerald_coord_weather_to_weather(
    uint16_t coord_weather,
    uint8_t *out_weather)
{
    uint8_t weather;

    if (out_weather == 0)
        return 0;

    switch (coord_weather) {
    case 1u:
        weather = REMASTER_EMERALD_WEATHER_SUNNY_CLOUDS;
        break;
    case 2u:
        weather = REMASTER_EMERALD_WEATHER_SUNNY;
        break;
    case 3u:
        weather = REMASTER_EMERALD_WEATHER_RAIN;
        break;
    case 4u:
        weather = REMASTER_EMERALD_WEATHER_SNOW;
        break;
    case 5u:
        weather = REMASTER_EMERALD_WEATHER_RAIN_THUNDERSTORM;
        break;
    case 6u:
        weather = REMASTER_EMERALD_WEATHER_FOG_HORIZONTAL;
        break;
    case 7u:
        weather = REMASTER_EMERALD_WEATHER_FOG_DIAGONAL;
        break;
    case 8u:
        weather = REMASTER_EMERALD_WEATHER_VOLCANIC_ASH;
        break;
    case 9u:
        weather = REMASTER_EMERALD_WEATHER_SANDSTORM;
        break;
    case 10u:
        weather = REMASTER_EMERALD_WEATHER_SHADE;
        break;
    case 11u:
        weather = REMASTER_EMERALD_WEATHER_DROUGHT;
        break;
    case 20u:
        weather = REMASTER_EMERALD_WEATHER_ROUTE119_CYCLE;
        break;
    case 21u:
        weather = REMASTER_EMERALD_WEATHER_ROUTE123_CYCLE;
        break;
    default:
        return 0;
    }

    *out_weather = weather;
    return 1;
}

static int step_event_is_warp_behavior(uint8_t behavior)
{
    switch (behavior) {
    case UINT8_C(0x0E): /* MB_MOSSDEEP_GYM_WARP */
    case UINT8_C(0x0F): /* MB_MT_PYRE_HOLE */
    case UINT8_C(0x29): /* MB_LAVARIDGE_GYM_B1F_WARP */
    case UINT8_C(0x60): /* MB_NON_ANIMATED_DOOR */
    case UINT8_C(0x61): /* MB_LADDER */
    case UINT8_C(0x67): /* MB_AQUA_HIDEOUT_WARP */
    case UINT8_C(0x68): /* MB_LAVARIDGE_GYM_1F_WARP */
    case UINT8_C(0x69): /* MB_ANIMATED_DOOR */
    case UINT8_C(0x6A): /* MB_UP_ESCALATOR */
    case UINT8_C(0x6B): /* MB_DOWN_ESCALATOR */
    case UINT8_C(0x70): /* MB_BRIDGE_OVER_OCEAN / Union Room exit */
        return 1;
    default:
        return 0;
    }
}

int remaster_emerald_process_step_events(
    RemasterEmeraldSave *save,
    const RemasterEmeraldCoordEventDef *coord_events,
    size_t coord_event_count,
    size_t coord_start_index,
    const RemasterEmeraldWarpEventDef *warps,
    size_t warp_count,
    int16_t x,
    int16_t y,
    uint8_t elevation,
    uint8_t metatile_behavior,
    void *userdata,
    RemasterEmeraldImmediateScriptFn run_immediate,
    RemasterEmeraldStepEventResult *out_result)
{
    size_t i;

    if (save == 0
        || out_result == 0
        || coord_start_index > coord_event_count
        || (coord_event_count != 0u && coord_events == 0)
        || (warp_count != 0u && warps == 0))
        return 0;

    out_result->kind = REMASTER_EMERALD_STEP_EVENT_NONE;
    out_result->coord_event_index = SIZE_MAX;
    out_result->next_coord_event_index = coord_event_count;
    out_result->warp_index = SIZE_MAX;
    out_result->script_id = 0;
    out_result->weather_changed = 0u;
    out_result->weather = 0u;

    for (i = coord_start_index; i < coord_event_count; ++i) {
        const RemasterEmeraldCoordEventDef *event = &coord_events[i];

        if (event->x != x || event->y != y)
            continue;

        if (!elevation_matches(event->elevation, elevation))
            continue;

        if (event->kind == REMASTER_EMERALD_COORD_WEATHER) {
            uint8_t weather;

            if (remaster_emerald_coord_weather_to_weather(
                    event->weather,
                    &weather)) {
                RemasterEmeraldOverworldState state;

                if (!remaster_emerald_overworld_get(save, &state))
                    return 0;

                state.weather = weather;
                if (!remaster_emerald_overworld_set(save, &state))
                    return 0;

                out_result->weather_changed = 1u;
                out_result->weather = weather;
            }

            continue;
        }

        if (event->kind != REMASTER_EMERALD_COORD_TRIGGER
            || event->script_id == 0)
            continue;

        if (event->trigger == 0u) {
            if (run_immediate != 0) {
                if (!run_immediate(userdata, event->script_id))
                    return 0;
                continue;
            }

            out_result->kind =
                REMASTER_EMERALD_STEP_EVENT_IMMEDIATE_SCRIPT;
            out_result->coord_event_index = i;
            out_result->next_coord_event_index = i + 1u;
            out_result->script_id = event->script_id;
            return 1;
        }

        {
            uint16_t value = 0;

            if (!remaster_emerald_var_get(
                    save,
                    event->trigger,
                    &value))
                return 0;

            if (value == (uint16_t)(uint8_t)event->index) {
                out_result->kind =
                    REMASTER_EMERALD_STEP_EVENT_COORD_SCRIPT;
                out_result->coord_event_index = i;
                out_result->next_coord_event_index = i + 1u;
                out_result->script_id = event->script_id;
                return 1;
            }
        }
    }

    if (step_event_is_warp_behavior(metatile_behavior)
        && warps != 0) {
        size_t warp_index = 0;

        if (remaster_emerald_find_warp(
                warps,
                warp_count,
                x,
                y,
                elevation,
                &warp_index)) {
            out_result->kind = REMASTER_EMERALD_STEP_EVENT_WARP;
            out_result->warp_index = warp_index;
            return 1;
        }
    }

    return 1;
}
