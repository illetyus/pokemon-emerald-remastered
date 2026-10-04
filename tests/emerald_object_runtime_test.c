#include "remaster/emerald_movement.h"
#include "remaster/emerald_object_state.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_object_runtime_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldSave save;
    RemasterEmeraldObjectEventDef events[3];
    RemasterEmeraldObjectRuntime runtime;
    RemasterEmeraldRuntimeObject observed;
    RemasterEmeraldObjectCollider colliders[
        REMASTER_EMERALD_RUNTIME_OBJECT_COUNT];
    size_t collider_count = 0;
    size_t index = 0;

    const uint16_t blocks[9] = {
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000
    };
    const uint16_t border[4] = {0, 0, 0, 0};
    const uint16_t attrs[1] = {0};
    RemasterEmeraldMapView map;
    RemasterEmeraldMovementContext movement;
    RemasterEmeraldMover player;
    RemasterEmeraldCollision collision;

    memset(&save, 0, sizeof(save));
    memset(events, 0, sizeof(events));
    memset(&runtime, 0, sizeof(runtime));
    memset(colliders, 0, sizeof(colliders));
    memset(&map, 0, sizeof(map));
    memset(&movement, 0, sizeof(movement));
    memset(&player, 0, sizeof(player));

    events[0].local_id = 1;
    events[0].x = 1;
    events[0].y = 1;
    events[0].elevation = 1;
    events[0].flag_id = 0;

    events[1].local_id = 2;
    events[1].x = 2;
    events[1].y = 1;
    events[1].elevation = 1;
    events[1].flag_id = 0x0020;

    events[2].local_id = 3;
    events[2].x = 0;
    events[2].y = 1;
    events[2].elevation = 2;
    events[2].flag_id = 0;

    if (!check(
            remaster_emerald_flag_set(&save, 0x0020, 1),
            "failed to hide object 2"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_load(
                &runtime,
                &save,
                events,
                3,
                1,
                1),
            "runtime object load failed"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_find(
                &runtime,
                1,
                &index)
            && remaster_emerald_object_runtime_get(
                &runtime,
                index,
                &observed)
            && observed.active == 1
            && observed.current_x == 1
            && observed.current_y == 1
            && observed.previous_x == 1
            && observed.previous_y == 1
            && observed.initial_x == 1
            && observed.initial_y == 1
            && observed.elevation == 1,
            "visible runtime object seed mismatch"))
        return 1;

    if (!check(
            !remaster_emerald_object_runtime_find(
                &runtime,
                2,
                &index),
            "hidden object must not consume an active runtime slot"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_set_position(
                &runtime,
                1,
                2,
                1,
                1)
            && remaster_emerald_object_runtime_find(
                &runtime,
                1,
                &index)
            && remaster_emerald_object_runtime_get(
                &runtime,
                index,
                &observed)
            && observed.current_x == 2
            && observed.current_y == 1
            && observed.previous_x == 1
            && observed.previous_y == 1,
            "runtime movement did not preserve previous tile"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_build_colliders(
                &runtime,
                colliders,
                REMASTER_EMERALD_RUNTIME_OBJECT_COUNT,
                &collider_count)
            && collider_count == 2,
            "runtime collider build failed"))
        return 1;

    map.width = 3;
    map.height = 3;
    map.blocks = blocks;
    map.block_count = 9;
    map.border = border;
    map.border_count = 4;
    map.primary_attributes = attrs;
    map.primary_attribute_count = 1;

    movement.map = &map;
    movement.objects = colliders;
    movement.object_count = collider_count;

    player.current_x = 1;
    player.current_y = 1;
    player.initial_x = 1;
    player.initial_y = 1;
    player.current_elevation = 1;
    player.self_object_index = SIZE_MAX;

    collision = remaster_emerald_player_basic_collision(
        &movement,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_OBJECT_EVENT,
            "runtime object's current tile must block player"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_set_position(
                &runtime,
                1,
                2,
                0,
                1)
            && remaster_emerald_object_runtime_build_colliders(
                &runtime,
                colliders,
                REMASTER_EMERALD_RUNTIME_OBJECT_COUNT,
                &collider_count),
            "second runtime move failed"))
        return 1;

    collision = remaster_emerald_player_basic_collision(
        &movement,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_OBJECT_EVENT,
            "runtime object's previous tile must remain blocking"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_set_position(
                &runtime,
                1,
                2,
                1,
                2)
            && remaster_emerald_object_runtime_build_colliders(
                &runtime,
                colliders,
                REMASTER_EMERALD_RUNTIME_OBJECT_COUNT,
                &collider_count),
            "elevation-separated runtime move failed"))
        return 1;

    collision = remaster_emerald_player_basic_collision(
        &movement,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_NONE,
            "different nonzero elevations must not object-collide"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_set_position(
                &runtime,
                1,
                2,
                1,
                1)
            && remaster_emerald_object_runtime_set_player_collision_exempt(
                &runtime,
                1,
                1)
            && remaster_emerald_object_runtime_build_colliders(
                &runtime,
                colliders,
                REMASTER_EMERALD_RUNTIME_OBJECT_COUNT,
                &collider_count),
            "player collision exemption setup failed"))
        return 1;

    collision = remaster_emerald_player_basic_collision(
        &movement,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_NONE,
            "player/follower collision exemption was not honored"))
        return 1;

    if (!check(
            remaster_emerald_object_runtime_set_player_collision_exempt(
                &runtime,
                1,
                0)
            && remaster_emerald_object_runtime_set_active(
                &runtime,
                1,
                0)
            && remaster_emerald_object_runtime_build_colliders(
                &runtime,
                colliders,
                REMASTER_EMERALD_RUNTIME_OBJECT_COUNT,
                &collider_count),
            "runtime object deactivation failed"))
        return 1;

    collision = remaster_emerald_player_basic_collision(
        &movement,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_NONE,
            "inactive runtime object must not collide"))
        return 1;

    {
        RemasterEmeraldObjectEventDef many_events[20];
        size_t active_count = 0;

        memset(many_events, 0, sizeof(many_events));
        remaster_emerald_object_runtime_reset(&runtime);

        for (index = 0; index < 20; ++index) {
            many_events[index].local_id = (uint16_t)(index + 1u);
            many_events[index].x = 1;
            many_events[index].y = 1;
            many_events[index].elevation = 1;
            many_events[index].flag_id = 0;
        }

        if (!check(
                remaster_emerald_object_runtime_load(
                    &runtime,
                    &save,
                    many_events,
                    20,
                    1,
                    1)
                && runtime.count == REMASTER_EMERALD_RUNTIME_OBJECT_COUNT,
                "visible template overflow must cap at 16 runtime slots"))
            return 1;

        for (index = 0; index < runtime.count; ++index) {
            if (runtime.objects[index].active)
                ++active_count;
        }

        if (!check(
                active_count == REMASTER_EMERALD_RUNTIME_OBJECT_COUNT
                && runtime.objects[0].local_id == 1
                && runtime.objects[
                    REMASTER_EMERALD_RUNTIME_OBJECT_COUNT - 1u
                ].local_id == REMASTER_EMERALD_RUNTIME_OBJECT_COUNT,
                "runtime slot ordering must follow Vanilla template scan order"))
            return 1;
    }

    puts("Emerald runtime object collision test passed.");
    return 0;
}
