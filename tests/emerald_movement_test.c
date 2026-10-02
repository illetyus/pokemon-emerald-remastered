#include "remaster/emerald_movement.h"

#include <stdio.h>
#include <string.h>

static int connection_valid(void *userdata, int32_t x, int32_t y)
{
    (void)userdata;
    return !(x == 2 && y == 1);
}

static int camera_can_move(void *userdata, uint8_t direction)
{
    (void)userdata;
    return direction != REMASTER_EMERALD_DIR_NORTH;
}

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "emerald_movement_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    /*
     * 3x3 map. All raw map collision bits are clear.
     * Center=metatile 0 (normal).
     * East=metatile 1 (normal).
     * South=metatile 2 (jump south).
     * North=metatile 3 (elevation 2).
     * West=metatile 4 (target east-edge blocked).
     */
    const uint16_t blocks[9] = {
        0x0000, 0x2003, 0x0000,
        0x0004, 0x0000, 0x0001,
        0x0000, 0x0002, 0x0000
    };
    const uint16_t border[4] = {0, 0, 0, 0};
    const uint16_t attrs[5] = {
        0x0000,
        0x0000,
        0x003B,
        0x0000,
        0x0030
    };
    RemasterEmeraldMapView map;
    RemasterEmeraldMovementContext context;
    RemasterEmeraldMover player;
    RemasterEmeraldObjectCollider objects[2];
    RemasterEmeraldCollision collision;

    memset(&map, 0, sizeof(map));
    map.width = 3;
    map.height = 3;
    map.blocks = blocks;
    map.block_count = 9;
    map.border = border;
    map.border_count = 4;
    map.primary_attributes = attrs;
    map.primary_attribute_count = 5;

    memset(&context, 0, sizeof(context));
    context.map = &map;

    memset(&player, 0, sizeof(player));
    player.current_x = 1;
    player.current_y = 1;
    player.initial_x = 1;
    player.initial_y = 1;
    player.current_elevation = 0;
    player.self_object_index = 0;

    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_NONE,
            "normal east step should be open"))
        return 1;

    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_SOUTH);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_LEDGE_JUMP,
            "south ledge should produce LEDGE_JUMP"))
        return 1;

    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_WEST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_IMPASSABLE,
            "target directional edge should block"))
        return 1;

    player.current_elevation = 1;
    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_NORTH);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_ELEVATION_MISMATCH,
            "elevation mismatch should be reported after impassable checks"))
        return 1;
    player.current_elevation = 0;

    memset(objects, 0, sizeof(objects));
    objects[0].active = 1;
    objects[0].current_x = 1;
    objects[0].current_y = 1;
    objects[0].previous_x = 1;
    objects[0].previous_y = 1;
    objects[0].elevation = 0;

    objects[1].active = 1;
    objects[1].current_x = 2;
    objects[1].current_y = 1;
    objects[1].previous_x = 2;
    objects[1].previous_y = 0;
    objects[1].elevation = 0;

    context.objects = objects;
    context.object_count = 2;

    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_OBJECT_EVENT,
            "active object on target tile should collide"))
        return 1;

    objects[1].elevation = 2;
    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_NONE,
            "different nonzero elevations should not object-collide"))
        return 1;

    context.connection_valid = connection_valid;
    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_EAST);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_IMPASSABLE,
            "invalid connection must block before objects"))
        return 1;
    context.connection_valid = 0;

    player.tracked_by_camera = 1;
    context.camera_can_move = camera_can_move;
    collision = remaster_emerald_player_basic_collision(
        &context,
        &player,
        REMASTER_EMERALD_DIR_NORTH);
    if (!check(
            collision == REMASTER_EMERALD_COLLISION_IMPASSABLE,
            "camera boundary must block before elevation"))
        return 1;

    player.tracked_by_camera = 0;
    player.range_x = 0;
    player.range_y = 0;

    {
        int32_t x = 4;
        int32_t y = 4;
        remaster_emerald_move_coords(
            REMASTER_EMERALD_DIR_WEST,
            &x,
            &y);
        if (!check(x == 3 && y == 4, "MoveCoords west mismatch"))
            return 1;
    }

    puts("Emerald movement compatibility test passed.");
    return 0;
}
