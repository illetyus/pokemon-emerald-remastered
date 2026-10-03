#ifndef REMASTER_EMERALD_RTC_H
#define REMASTER_EMERALD_RTC_H

#include "remaster/emerald_save.h"
#include "remaster/platform.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t remaster_emerald_rtc_day_count(
    int32_t full_year,
    int32_t month,
    int32_t day);

RemasterEmeraldTime remaster_emerald_time_difference(
    RemasterEmeraldTime later,
    RemasterEmeraldTime earlier);

int remaster_emerald_rtc_raw_now(
    RemasterEmeraldTime *out_time);

int remaster_emerald_rtc_local_now(
    const RemasterEmeraldSave *save,
    RemasterEmeraldTime *out_time);

int remaster_emerald_rtc_realign_now(
    RemasterEmeraldSave *save,
    RemasterEmeraldTime desired_local_time);

#ifdef __cplusplus
}
#endif

#endif
