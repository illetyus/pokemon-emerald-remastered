#include "remaster/emerald_events.h"
#include "remaster/emerald_map.h"
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

        if (value == events[i].index) {
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
