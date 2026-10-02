#ifndef REMASTER_EMERALD_MOVEMENT_H
#define REMASTER_EMERALD_MOVEMENT_H

#include "remaster/emerald_map.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterEmeraldCollision {
    REMASTER_EMERALD_COLLISION_NONE = 0,
    REMASTER_EMERALD_COLLISION_OUTSIDE_RANGE = 1,
    REMASTER_EMERALD_COLLISION_IMPASSABLE = 2,
    REMASTER_EMERALD_COLLISION_ELEVATION_MISMATCH = 3,
    REMASTER_EMERALD_COLLISION_OBJECT_EVENT = 4,
    REMASTER_EMERALD_COLLISION_STOP_SURFING = 5,
    REMASTER_EMERALD_COLLISION_LEDGE_JUMP = 6,
    REMASTER_EMERALD_COLLISION_PUSHED_BOULDER = 7,
    REMASTER_EMERALD_COLLISION_ROTATING_GATE = 8,
    REMASTER_EMERALD_COLLISION_WHEELIE_HOP = 9,
    REMASTER_EMERALD_COLLISION_ISOLATED_VERTICAL_RAIL = 10,
    REMASTER_EMERALD_COLLISION_ISOLATED_HORIZONTAL_RAIL = 11,
    REMASTER_EMERALD_COLLISION_VERTICAL_RAIL = 12,
    REMASTER_EMERALD_COLLISION_HORIZONTAL_RAIL = 13
} RemasterEmeraldCollision;

typedef struct RemasterEmeraldObjectCollider {
    uint8_t active;
    int32_t current_x;
    int32_t current_y;
    int32_t previous_x;
    int32_t previous_y;
    uint8_t elevation;
} RemasterEmeraldObjectCollider;

typedef struct RemasterEmeraldMover {
    int32_t current_x;
    int32_t current_y;
    int32_t initial_x;
    int32_t initial_y;
    uint8_t range_x;
    uint8_t range_y;
    uint8_t current_elevation;
    uint8_t tracked_by_camera;
    size_t self_object_index;
} RemasterEmeraldMover;

typedef int (*RemasterEmeraldConnectionValidFn)(
    void *userdata,
    int32_t x,
    int32_t y);

typedef int (*RemasterEmeraldCameraMoveFn)(
    void *userdata,
    uint8_t direction);

typedef int (*RemasterEmeraldObjectCollisionExemptFn)(
    void *userdata,
    size_t obstacle_index,
    size_t collider_index);

typedef struct RemasterEmeraldMovementContext {
    const RemasterEmeraldMapView *map;

    const RemasterEmeraldObjectCollider *objects;
    size_t object_count;

    void *userdata;
    RemasterEmeraldConnectionValidFn connection_valid;
    RemasterEmeraldCameraMoveFn camera_can_move;
    RemasterEmeraldObjectCollisionExemptFn object_collision_exempt;
} RemasterEmeraldMovementContext;

void remaster_emerald_move_coords(
    uint8_t direction,
    int32_t *x,
    int32_t *y);

RemasterEmeraldCollision remaster_emerald_get_collision_at(
    const RemasterEmeraldMovementContext *context,
    const RemasterEmeraldMover *mover,
    int32_t target_x,
    int32_t target_y,
    uint8_t direction);

RemasterEmeraldCollision remaster_emerald_player_basic_collision(
    const RemasterEmeraldMovementContext *context,
    const RemasterEmeraldMover *player,
    uint8_t direction);

#ifdef __cplusplus
}
#endif

#endif
