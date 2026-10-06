#ifndef REMASTER_PLATFORM_H
#define REMASTER_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterLogLevel {
    REMASTER_LOG_DEBUG = 0,
    REMASTER_LOG_INFO,
    REMASTER_LOG_WARNING,
    REMASTER_LOG_ERROR
} RemasterLogLevel;

typedef struct RemasterWallClock {
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
} RemasterWallClock;

typedef enum RemasterSaveReadResult {
    REMASTER_SAVE_READ_OK = 0,
    REMASTER_SAVE_READ_MISSING = 1,
    REMASTER_SAVE_READ_ERROR = 2
} RemasterSaveReadResult;

typedef struct RemasterPlatformVTable {
    void *userdata;

    uint64_t (*monotonic_time_ns)(void *userdata);
    int (*read_wall_clock)(void *userdata, RemasterWallClock *out_clock);

    int (*save_read)(
        void *userdata,
        const char *slot,
        uint8_t *buffer,
        size_t capacity,
        size_t *out_size);

    int (*save_write)(
        void *userdata,
        const char *slot,
        const uint8_t *buffer,
        size_t size);

    void (*log)(
        void *userdata,
        RemasterLogLevel level,
        const char *message);

    void (*emit_presentation_event)(
        void *userdata,
        uint32_t event_id,
        int64_t arg0,
        int64_t arg1);

    /* Preferred typed read. OK publishes the actual file size; bytes are
     * copied only when it fits capacity. MISSING means confirmed absence,
     * never an open/read/close error. Legacy save_read remains supported:
     * positive success -> OK, ambiguous failure -> ERROR, never MISSING. */
    RemasterSaveReadResult (*save_read_result)(
        void *userdata,
        const char *slot,
        uint8_t *buffer,
        size_t capacity,
        size_t *out_size);
} RemasterPlatformVTable;

void remaster_platform_install(const RemasterPlatformVTable *platform);
const RemasterPlatformVTable *remaster_platform_get(void);

#ifdef __cplusplus
}
#endif

#endif
