#include "remaster/emerald_rtc.h"

static int emerald_is_leap_year(uint32_t year)
{
    return ((year % 4u) == 0u && (year % 100u) != 0u)
        || (year % 400u) == 0u;
}

uint16_t remaster_emerald_rtc_day_count(
    int32_t full_year,
    int32_t month,
    int32_t day)
{
    static const uint8_t kDaysInMonth[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    int32_t year;
    int32_t i;
    uint32_t day_count = 0;

    if (month < 1 || month > 12 || day < 1)
        return 0;

    /*
     * The GBA RTC stores a two-digit year. Vanilla Emerald therefore feeds
     * 0..99 into ConvertDateToDayCount. Preserve that representation when
     * the modern platform supplies a full calendar year.
     */
    year = full_year % 100;
    if (year < 0)
        year += 100;

    for (i = year - 1; i >= 0; --i) {
        day_count += 365u;
        if (emerald_is_leap_year((uint32_t)i))
            ++day_count;
    }

    for (i = 0; i < month - 1; ++i)
        day_count += kDaysInMonth[i];

    if (month > 2 && emerald_is_leap_year((uint32_t)year))
        ++day_count;

    day_count += (uint32_t)day;

    return (uint16_t)day_count;
}

RemasterEmeraldTime remaster_emerald_time_difference(
    RemasterEmeraldTime later,
    RemasterEmeraldTime earlier)
{
    RemasterEmeraldTime result;
    int32_t seconds = (int32_t)later.seconds - (int32_t)earlier.seconds;
    int32_t minutes = (int32_t)later.minutes - (int32_t)earlier.minutes;
    int32_t hours = (int32_t)later.hours - (int32_t)earlier.hours;
    int32_t days = (int32_t)later.days - (int32_t)earlier.days;

    if (seconds < 0) {
        seconds += 60;
        --minutes;
    }

    if (minutes < 0) {
        minutes += 60;
        --hours;
    }

    if (hours < 0) {
        hours += 24;
        --days;
    }

    result.days = (int16_t)days;
    result.hours = (int8_t)hours;
    result.minutes = (int8_t)minutes;
    result.seconds = (int8_t)seconds;
    return result;
}

int remaster_emerald_rtc_raw_now(
    RemasterEmeraldTime *out_time)
{
    const RemasterPlatformVTable *platform = remaster_platform_get();
    RemasterWallClock wall_clock;

    if (out_time == 0
        || platform == 0
        || platform->read_wall_clock == 0)
        return 0;

    if (!platform->read_wall_clock(platform->userdata, &wall_clock))
        return 0;

    out_time->days = (int16_t)remaster_emerald_rtc_day_count(
        wall_clock.year,
        wall_clock.month,
        wall_clock.day);
    out_time->hours = (int8_t)wall_clock.hour;
    out_time->minutes = (int8_t)wall_clock.minute;
    out_time->seconds = (int8_t)wall_clock.second;

    return 1;
}

int remaster_emerald_rtc_local_now(
    const RemasterEmeraldSave *save,
    RemasterEmeraldTime *out_time)
{
    RemasterEmeraldTime raw;
    RemasterEmeraldTime offset;

    if (save == 0 || out_time == 0)
        return 0;

    if (!remaster_emerald_rtc_raw_now(&raw))
        return 0;

    offset = remaster_emerald_save_get_local_time_offset(save);
    *out_time = remaster_emerald_time_difference(raw, offset);
    return 1;
}

int remaster_emerald_rtc_realign_now(
    RemasterEmeraldSave *save,
    RemasterEmeraldTime desired_local_time)
{
    RemasterEmeraldTime raw;
    RemasterEmeraldTime offset;

    if (save == 0)
        return 0;

    if (!remaster_emerald_rtc_raw_now(&raw))
        return 0;

    offset = remaster_emerald_time_difference(raw, desired_local_time);
    remaster_emerald_save_set_local_time_offset(save, offset);
    remaster_emerald_save_set_last_berry_update(save, desired_local_time);
    return 1;
}
