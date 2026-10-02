#include "remaster/emerald_events.h"
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
