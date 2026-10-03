#include "remaster/emerald_events.h"
#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"
#include "remaster/emerald_transition.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    FOOTER_ID_OFFSET = 4084,
    FOOTER_CHECKSUM_OFFSET = 4086,
    FOOTER_SIGNATURE_OFFSET = 4088,
    FOOTER_COUNTER_OFFSET = 4092
};

static const uint32_t kSignature = UINT32_C(0x08012025);

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

static size_t sector_size(uint16_t id)
{
    if (id == 0)
        return REMASTER_EMERALD_SAVE_BLOCK2_BYTES;
    if (id >= 1 && id <= 3)
        return REMASTER_EMERALD_SECTOR_DATA_BYTES;
    if (id == 4)
        return REMASTER_EMERALD_SAVE_BLOCK1_BYTES
            - 3u * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    if (id >= 5 && id <= 12)
        return REMASTER_EMERALD_SECTOR_DATA_BYTES;
    if (id == 13)
        return REMASTER_EMERALD_STORAGE_BYTES
            - 8u * REMASTER_EMERALD_SECTOR_DATA_BYTES;
    return 0;
}

static const uint8_t *payload_for(
    const RemasterEmeraldSave *save,
    uint16_t id)
{
    if (id == 0)
        return save->save_block2;

    if (id <= 4)
        return save->save_block1
            + (size_t)(id - 1u) * REMASTER_EMERALD_SECTOR_DATA_BYTES;

    return save->pokemon_storage
        + (size_t)(id - 5u) * REMASTER_EMERALD_SECTOR_DATA_BYTES;
}

static void build_slot(
    uint8_t *image,
    uint8_t slot,
    uint32_t counter,
    uint16_t rotation,
    const RemasterEmeraldSave *save)
{
    uint16_t id;

    for (id = 0; id < REMASTER_EMERALD_MAIN_SECTORS; ++id) {
        const uint16_t physical =
            (uint16_t)((id + rotation) % REMASTER_EMERALD_MAIN_SECTORS);
        uint8_t *sector =
            image
            + ((size_t)slot * REMASTER_EMERALD_MAIN_SECTORS + physical)
            * REMASTER_EMERALD_SECTOR_BYTES;
        const uint8_t *payload = payload_for(save, id);
        const size_t size = sector_size(id);

        memset(sector, 0, REMASTER_EMERALD_SECTOR_BYTES);
        memcpy(sector, payload, size);
        put16(sector + FOOTER_ID_OFFSET, id);
        put16(
            sector + FOOTER_CHECKSUM_OFFSET,
            remaster_emerald_checksum(payload, size));
        put32(sector + FOOTER_SIGNATURE_OFFSET, kSignature);
        put32(sector + FOOTER_COUNTER_OFFSET, counter);
    }
}

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "r1_vanillaplus_integration_test: %s\n", message);
        return 0;
    }

    return 1;
}

int main(void)
{
    uint8_t *image;
    RemasterEmeraldSave initial;
    RemasterEmeraldSave live;
    RemasterEmeraldSave reloaded;
    RemasterEmeraldOverworldState state;
    RemasterEmeraldWarpState dynamic_warp;
    RemasterEmeraldWarpState observed_dynamic;
    RemasterEmeraldWarpState destination;
    RemasterEmeraldWarpEventDef target_warps[2];
    RemasterEmeraldObjectEventDef object_event;
    RemasterEmeraldCoordEventDef coord_event;
    RemasterEmeraldCoordMatch coord_match;
    uint16_t var_value = 0;
    int flag_value = 0;

    image = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    if (image == 0)
        return 1;

    memset(image, 0xff, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    memset(&initial, 0, sizeof(initial));
    memset(&live, 0, sizeof(live));
    memset(&reloaded, 0, sizeof(reloaded));
    memset(&state, 0, sizeof(state));
    memset(&dynamic_warp, 0, sizeof(dynamic_warp));
    memset(&observed_dynamic, 0, sizeof(observed_dynamic));
    memset(&destination, 0, sizeof(destination));
    memset(target_warps, 0, sizeof(target_warps));
    memset(&object_event, 0, sizeof(object_event));
    memset(&coord_event, 0, sizeof(coord_event));

    /* Seed a non-zero encryption key like a real Emerald save. */
    initial.save_block2[0xAC] = 0x78;
    initial.save_block2[0xAD] = 0x56;
    initial.save_block2[0xAE] = 0x34;
    initial.save_block2[0xAF] = 0x12;

    state.player_x = 5;
    state.player_y = 8;
    state.map_group = 0;
    state.map_num = 9;
    state.warp_id = 0;
    state.warp_x = -1;
    state.warp_y = -1;
    state.map_layout_id = 12;
    state.weather = 1;
    state.flash_level = 0;
    state.party_count = 3;
    state.money = 54321;
    state.coins = 777;
    state.registered_item = 42;

    if (!check(
            remaster_emerald_overworld_set(&initial, &state),
            "failed to seed overworld state"))
        return 1;

    if (!check(
            remaster_emerald_flag_set(&initial, 0x123, 1),
            "failed to seed persistent flag"))
        return 1;

    if (!check(
            remaster_emerald_var_set(&initial, 0x405A, 2),
            "failed to seed persistent var"))
        return 1;

    dynamic_warp.map_group = 4;
    dynamic_warp.map_num = 7;
    dynamic_warp.warp_id = 1;
    dynamic_warp.x = 13;
    dynamic_warp.y = 14;

    if (!check(
            remaster_emerald_dynamic_warp_set(&initial, &dynamic_warp),
            "failed to seed dynamic warp"))
        return 1;

    build_slot(image, 0, 40, 4, &initial);
    build_slot(image, 1, 41, 9, &initial);

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &live) == REMASTER_EMERALD_SAVE_OK,
            "failed to decode valid two-slot save image"))
        return 1;

    if (!check(
            live.counter == 41
            && live.selected_slot == 1
            && live.last_written_sector == 9,
            "newest save slot metadata mismatch"))
        return 1;

    if (!check(
            remaster_emerald_overworld_get(&live, &state),
            "failed to read decoded overworld state"))
        return 1;

    if (!check(
            state.player_x == 5
            && state.player_y == 8
            && state.map_group == 0
            && state.map_num == 9
            && state.money == 54321
            && state.party_count == 3,
            "decoded gameplay state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_dynamic_warp_get(&live, &observed_dynamic)
            && observed_dynamic.map_group == 4
            && observed_dynamic.map_num == 7
            && observed_dynamic.warp_id == 1
            && observed_dynamic.x == 13
            && observed_dynamic.y == 14,
            "decoded dynamic warp mismatch"))
        return 1;

    /*
     * Use the same hide-flag rule as Vanilla ObjectEvent spawning.
     * Set flag -> hidden, clear flag -> visible.
     */
    object_event.local_id = 2;
    object_event.x = 12;
    object_event.y = 13;
    object_event.elevation = 3;
    object_event.flag_id = 0x123;

    if (!check(
            !remaster_emerald_object_event_visible(&live, &object_event),
            "seeded hide flag should hide object event"))
        return 1;

    if (!check(
            remaster_emerald_flag_set(&live, 0x123, 0)
            && remaster_emerald_object_event_visible(&live, &object_event),
            "clearing hide flag should reveal object event"))
        return 1;

    coord_event.kind = REMASTER_EMERALD_COORD_TRIGGER;
    coord_event.x = 10;
    coord_event.y = 1;
    coord_event.elevation = 3;
    coord_event.trigger = 0x405A;
    coord_event.index = 3;

    coord_match = remaster_emerald_find_coord_event(
        &live,
        &coord_event,
        1,
        10,
        1,
        3);

    if (!check(
            coord_match.kind == REMASTER_EMERALD_COORD_MATCH_NONE,
            "coord trigger should not match old var value"))
        return 1;

    if (!check(
            remaster_emerald_var_set(&live, 0x405A, 3),
            "failed to change coord trigger var"))
        return 1;

    coord_match = remaster_emerald_find_coord_event(
        &live,
        &coord_event,
        1,
        10,
        1,
        3);

    if (!check(
            coord_match.kind == REMASTER_EMERALD_COORD_MATCH_SCRIPT,
            "coord trigger should match updated var value"))
        return 1;

    /*
     * Enter a target map through warp id 1. Map entry clears only temporary
     * flags/vars; persistent values must survive.
     */
    if (!check(
            remaster_emerald_flag_set(&live, 0x001F, 1)
            && remaster_emerald_var_set(&live, 0x4000, 0xAAAA),
            "failed to seed temporary field-event state"))
        return 1;

    target_warps[0].x = 2;
    target_warps[0].y = 3;
    target_warps[1].x = 17;
    target_warps[1].y = 6;

    destination.map_group = 2;
    destination.map_num = 5;
    destination.warp_id = 1;
    destination.x = -1;
    destination.y = -1;

    if (!check(
            remaster_emerald_apply_warp(
                &live,
                destination,
                88,
                REMASTER_EMERALD_WEATHER_SUNNY,
                REMASTER_EMERALD_MAP_TYPE_TOWN,
                0,
                30,
                20,
                target_warps,
                2),
            "failed to apply gameplay warp"))
        return 1;

    if (!check(
            remaster_emerald_overworld_get(&live, &state),
            "failed to read post-warp state"))
        return 1;

    if (!check(
            state.map_group == 2
            && state.map_num == 5
            && state.warp_id == 1
            && state.player_x == 17
            && state.player_y == 6
            && state.map_layout_id == 88,
            "post-warp location mismatch"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&live, 0x001F, &flag_value)
            && flag_value == 0
            && remaster_emerald_var_get(&live, 0x4000, &var_value)
            && var_value == 0,
            "map entry did not clear temporary flag/var state"))
        return 1;

    if (!check(
            remaster_emerald_flag_get(&live, 0x123, &flag_value)
            && flag_value == 0
            && remaster_emerald_var_get(&live, 0x405A, &var_value)
            && var_value == 3,
            "persistent event state changed during map transition"))
        return 1;

    /*
     * Serialize the mutated live state back into a valid Emerald image and
     * decode again. This is the R1 compatibility boundary.
     */
    if (!check(
            remaster_emerald_save_encode_next(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &live),
            "failed to encode mutated gameplay state"))
        return 1;

    if (!check(
            remaster_emerald_save_decode(
                image,
                REMASTER_EMERALD_SAVE_IMAGE_BYTES,
                &reloaded) == REMASTER_EMERALD_SAVE_OK,
            "mutated save image did not decode cleanly"))
        return 1;

    if (!check(
            remaster_emerald_overworld_get(&reloaded, &state),
            "failed to read reloaded state"))
        return 1;

    if (!check(
            state.map_group == 2
            && state.map_num == 5
            && state.player_x == 17
            && state.player_y == 6
            && state.money == 54321
            && state.party_count == 3,
            "reloaded gameplay state mismatch"))
        return 1;

    if (!check(
            remaster_emerald_var_get(&reloaded, 0x405A, &var_value)
            && var_value == 3,
            "reloaded persistent var mismatch"))
        return 1;

    if (!check(
            remaster_emerald_dynamic_warp_get(&reloaded, &observed_dynamic)
            && observed_dynamic.map_group == 4
            && observed_dynamic.map_num == 7
            && observed_dynamic.warp_id == 1
            && observed_dynamic.x == 13
            && observed_dynamic.y == 14,
            "dynamic warp did not survive save rewrite"))
        return 1;

    free(image);
    puts("R1 Vanilla+ save/gameplay integration regression passed.");
    return 0;
}
