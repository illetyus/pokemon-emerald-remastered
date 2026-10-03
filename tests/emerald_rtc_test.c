#include "remaster/emerald_rtc.h"
#include "remaster/platform.h"

#include <stdio.h>
#include <string.h>

typedef struct ClockFixture {
    RemasterWallClock now;
} ClockFixture;

static int read_clock(void *userdata, RemasterWallClock *out_clock)
{
    ClockFixture *fixture = (ClockFixture *)userdata;

    if (fixture == 0 || out_clock == 0)
        return 0;

    *out_clock = fixture->now;
    return 1;
}

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_rtc_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    ClockFixture fixture;
    RemasterPlatformVTable platform;
    RemasterEmeraldSave save;
    RemasterEmeraldTime desired;
    RemasterEmeraldTime local;
    RemasterEmeraldTime offset;

    if (!check(
            remaster_emerald_rtc_day_count(2000, 1, 1) == 1,
            "2000-01-01 day count mismatch"))
        return 1;

    if (!check(
            remaster_emerald_rtc_day_count(2000, 3, 1) == 61,
            "two-digit year leap calculation mismatch"))
        return 1;

    if (!check(
            remaster_emerald_rtc_day_count(2026, 1, 1)
                == remaster_emerald_rtc_day_count(26, 1, 1),
            "full-year conversion must preserve GBA two-digit year"))
        return 1;

    memset(&fixture, 0, sizeof(fixture));
    fixture.now.year = 2026;
    fixture.now.month = 10;
    fixture.now.day = 3;
    fixture.now.hour = 12;
    fixture.now.minute = 30;
    fixture.now.second = 15;

    memset(&platform, 0, sizeof(platform));
    platform.userdata = &fixture;
    platform.read_wall_clock = read_clock;
    remaster_platform_install(&platform);

    memset(&save, 0, sizeof(save));

    desired.days = 100;
    desired.hours = 5;
    desired.minutes = 20;
    desired.seconds = 10;

    if (!check(
            remaster_emerald_rtc_realign_now(&save, desired),
            "RTC realign failed"))
        return 1;

    if (!check(
            remaster_emerald_rtc_local_now(&save, &local),
            "RTC local-now failed"))
        return 1;

    if (!check(
            local.days == desired.days
            && local.hours == desired.hours
            && local.minutes == desired.minutes
            && local.seconds == desired.seconds,
            "realigned local time does not match requested time"))
        return 1;

    offset = remaster_emerald_save_get_local_time_offset(&save);
    if (!check(offset.days != 0, "RTC offset was not persisted into SaveBlock2"))
        return 1;

    fixture.now.hour = 13;
    fixture.now.minute = 31;
    fixture.now.second = 16;

    if (!check(
            remaster_emerald_rtc_local_now(&save, &local),
            "advanced RTC local-now failed"))
        return 1;

    if (!check(
            local.days == 100
            && local.hours == 6
            && local.minutes == 21
            && local.seconds == 11,
            "local time did not advance with platform wall clock"))
        return 1;

    local = remaster_emerald_save_get_last_berry_update(&save);
    if (!check(
            local.days == 100
            && local.hours == 5
            && local.minutes == 20
            && local.seconds == 10,
            "realign did not reset lastBerryTreeUpdate"))
        return 1;

    remaster_platform_install(0);
    puts("Emerald RTC compatibility test passed.");
    return 0;
}
