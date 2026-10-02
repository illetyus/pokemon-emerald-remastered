#include "remaster/emerald_movement.h"

static int outside_movement_range(
    const RemasterEmeraldMover *mover,
    int32_t x,
    int32_t y)
{
    if (mover->range_x != 0u) {
        const int32_t left =
            mover->initial_x - (int32_t)mover->range_x;
        const int32_t right =
            mover->initial_x + (int32_t)mover->range_x;

        if (x < left || x > right)
            return 1;
    }

    if (mover->range_y != 0u) {
        const int32_t top =
            mover->initial_y - (int32_t)mover->range_y;
        const int32_t bottom =
            mover->initial_y + (int32_t)mover->range_y;

        if (y < top || y > bottom)
            return 1;
    }

    return 0;
}

static int object_collision(
    const RemasterEmeraldMovementContext *context,
    const RemasterEmeraldMover *mover,
    int32_t x,
    int32_t y)
{
    size_t i;

    if (context->objects == 0)
        return 0;

    for (i = 0; i < context->object_count; ++i) {
        const RemasterEmeraldObjectCollider *object =
            &context->objects[i];

        if (!object->active || i == mover->self_object_index)
            continue;

        if (context->object_collision_exempt != 0
            && context->object_collision_exempt(
                context->userdata,
                i,
                mover->self_object_index))
            continue;

        if ((object->current_x == x && object->current_y == y)
            || (object->previous_x == x && object->previous_y == y)) {
            if (remaster_emerald_elevations_compatible(
                    mover->current_elevation,
                    object->elevation))
                return 1;
        }
    }

    return 0;
}

void remaster_emerald_move_coords(
    uint8_t direction,
    int32_t *x,
    int32_t *y)
{
    if (x == 0 || y == 0)
        return;

    switch (direction) {
    case REMASTER_EMERALD_DIR_SOUTH:
        ++*y;
        break;
    case REMASTER_EMERALD_DIR_NORTH:
        --*y;
        break;
    case REMASTER_EMERALD_DIR_WEST:
        --*x;
        break;
    case REMASTER_EMERALD_DIR_EAST:
        ++*x;
        break;
    default:
        break;
    }
}

RemasterEmeraldCollision remaster_emerald_get_collision_at(
    const RemasterEmeraldMovementContext *context,
    const RemasterEmeraldMover *mover,
    int32_t target_x,
    int32_t target_y,
    uint8_t direction)
{
    uint8_t current_behavior;
    uint8_t target_behavior;
    uint8_t target_elevation;

    if (context == 0 || context->map == 0 || mover == 0)
        return REMASTER_EMERALD_COLLISION_IMPASSABLE;

    if (outside_movement_range(mover, target_x, target_y))
        return REMASTER_EMERALD_COLLISION_OUTSIDE_RANGE;

    if (remaster_emerald_map_collision_at(
            context->map,
            target_x,
            target_y) != 0u)
        return REMASTER_EMERALD_COLLISION_IMPASSABLE;

    if (context->connection_valid != 0
        && !context->connection_valid(
            context->userdata,
            target_x,
            target_y))
        return REMASTER_EMERALD_COLLISION_IMPASSABLE;

    current_behavior = remaster_emerald_map_behavior_at(
        context->map,
        mover->current_x,
        mover->current_y);
    target_behavior = remaster_emerald_map_behavior_at(
        context->map,
        target_x,
        target_y);

    if (remaster_emerald_directionally_blocked(
            current_behavior,
            target_behavior,
            direction))
        return REMASTER_EMERALD_COLLISION_IMPASSABLE;

    if (mover->tracked_by_camera
        && context->camera_can_move != 0
        && !context->camera_can_move(
            context->userdata,
            direction))
        return REMASTER_EMERALD_COLLISION_IMPASSABLE;

    target_elevation = remaster_emerald_map_elevation_at(
        context->map,
        target_x,
        target_y);

    if (remaster_emerald_map_elevation_mismatch(
            mover->current_elevation,
            target_elevation))
        return REMASTER_EMERALD_COLLISION_ELEVATION_MISMATCH;

    if (object_collision(context, mover, target_x, target_y))
        return REMASTER_EMERALD_COLLISION_OBJECT_EVENT;

    return REMASTER_EMERALD_COLLISION_NONE;
}

RemasterEmeraldCollision remaster_emerald_player_basic_collision(
    const RemasterEmeraldMovementContext *context,
    const RemasterEmeraldMover *player,
    uint8_t direction)
{
    int32_t x;
    int32_t y;
    RemasterEmeraldCollision collision;
    uint8_t behavior;

    if (context == 0 || player == 0)
        return REMASTER_EMERALD_COLLISION_IMPASSABLE;

    x = player->current_x;
    y = player->current_y;
    remaster_emerald_move_coords(direction, &x, &y);

    collision = remaster_emerald_get_collision_at(
        context,
        player,
        x,
        y,
        direction);

    behavior = remaster_emerald_map_behavior_at(
        context->map,
        x,
        y);

    if (remaster_emerald_ledge_jump_direction(
            behavior,
            direction) != REMASTER_EMERALD_DIR_NONE)
        return REMASTER_EMERALD_COLLISION_LEDGE_JUMP;

    return collision;
}
