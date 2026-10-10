#include "remaster/emerald_overworld.h"
#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_quest.h"
#include "remaster/emerald_script_runtime.h"
#include "r4_littleroot_fixture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const RemasterEmeraldScriptRegistry gR2LittlerootRegistry;

static const RemasterEmeraldSpecialBinding kBindings[] = {
    {"HealPlayerParty", 0, REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN, REMASTER_EMERALD_SCRIPT_DOMAIN_ACTION_HEAL_PARTY, 0},
    {"TurnOffTVScreen", 65, REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL, 0, 0},
    {"StartWallClock", 157, REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL, 0, 0},
    {"Special_ViewWallClock", 158, REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL, 0, 0},
    {"ChooseStarter", 159, REMASTER_EMERALD_SCRIPT_REQUEST_STARTER_SELECTION, 0, 0x800D},
    {"ChangePokemonNickname", 161, REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL, 0, 0}
};
static const RemasterEmeraldSpecialRegistry kSpecials = {kBindings, sizeof(kBindings) / sizeof(kBindings[0])};

static void hex(const uint8_t *bytes, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        printf("%02x", (unsigned)bytes[i]);
}

static void json_string(const char *s)
{
    const unsigned char *p = (const unsigned char *)(s == NULL ? "" : s);
    putchar('"');
    for (; *p; ++p) {
        if (*p == '"' || *p == '\\')
            printf("\\%c", (int)*p);
        else if (*p < 32)
            printf("\\u%04x", (unsigned)*p);
        else
            putchar((int)*p);
    }
    putchar('"');
}

static unsigned var(const RemasterEmeraldSave *save, uint16_t id)
{
    uint16_t value = 0;
    (void)remaster_emerald_var_get(save, id, &value);
    return value;
}

static unsigned flag(const RemasterEmeraldSave *save, uint16_t id)
{
    int value = 0;
    (void)remaster_emerald_flag_get(save, id, &value);
    return value != 0;
}

static int snapshot(const RemasterEmeraldSave *save,
    const RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldOverworldActionResult *action, int host_lock)
{
    RemasterEmeraldOverworldState world;
    const RemasterEmeraldQuestObjective *objective = remaster_emerald_quest_active(save);
    if (!remaster_emerald_overworld_get(save, &world))
        return 0;
    fputs("{\"version\":1,\"format\":\"vanillaplus\",\"domains\":{\"save_block2\":\"", stdout);
    hex(save->save_block2, REMASTER_EMERALD_SAVE_BLOCK2_BYTES);
    fputs("\",\"save_block1\":\"", stdout);
    hex(save->save_block1, REMASTER_EMERALD_SAVE_BLOCK1_BYTES);
    fputs("\",\"storage\":\"", stdout);
    hex(save->pokemon_storage, REMASTER_EMERALD_STORAGE_BYTES);
    fputs("\",\"script\":", stdout);
    if (runtime != NULL) {
        RemasterEmeraldScriptRuntime normalized = *runtime;
        uint8_t checkpoint[298];
        size_t i;
        if (remaster_emerald_script_runtime_checkpoint_size() != sizeof(checkpoint)
            || normalized.vm.stack_depth > REMASTER_EMERALD_SCRIPT_STACK_DEPTH)
            return 0;
        for (i = normalized.vm.stack_depth; i < REMASTER_EMERALD_SCRIPT_STACK_DEPTH; ++i)
            memset(&normalized.vm.stack[i], 0, sizeof(normalized.vm.stack[i]));
        if (!normalized.has_pending_request)
            memset(&normalized.pending_request, 0, sizeof(normalized.pending_request));
        if (!remaster_emerald_script_runtime_checkpoint_write(&normalized, checkpoint, sizeof(checkpoint))) {
            fprintf(stderr, "checkpoint rejected active request type=%u action=%u status=%u\n",
                (unsigned)normalized.pending_request.type, (unsigned)normalized.pending_request.action,
                (unsigned)normalized.vm.status);
            return 0;
        }
        putchar('"');
        hex(checkpoint, sizeof(checkpoint));
        putchar('"');
    } else
        fputs("null", stdout);
    fputs(",\"objects\":null,\"encounter\":null,\"battle\":null},\"observations\":{", stdout);
    printf("\"x\":%d,\"y\":%d,\"map_group\":%d,\"map_num\":%d,\"kind\":%u,\"collision\":%u,",
        (int)world.player_x, (int)world.player_y, (int)world.map_group, (int)world.map_num,
        (unsigned)action->kind, (unsigned)action->collision);
    printf("\"intro\":%u,\"rival\":%u,\"town\":%u,\"route101\":%u,\"lab\":%u,",
        var(save, 0x4092), var(save, 0x408D), var(save, 0x4050), var(save, 0x4060), var(save, 0x4084));
    printf("\"clock\":%u,\"rescued\":%u,\"pokemon_get\":%u,\"party_count\":%u,\"objective\":%u,",
        flag(save, 0x51), flag(save, 0x52), flag(save, 0x860),
        (unsigned)remaster_emerald_party_count(save), objective == NULL ? 0u : (unsigned)objective->id);
    printf("\"script_status\":%u,\"pending_type\":%u,\"pending_action\":%u,\"pending_sequence\":%llu,\"pending_resource\":",
        runtime == NULL ? 0u : (unsigned)runtime->vm.status,
        runtime == NULL || !runtime->has_pending_request ? 0u : (unsigned)runtime->pending_request.type,
        runtime == NULL || !runtime->has_pending_request ? 0u : (unsigned)runtime->pending_request.action,
        runtime == NULL || !runtime->has_pending_request ? 0ull : (unsigned long long)runtime->pending_request.sequence);
    json_string(runtime == NULL || !runtime->has_pending_request ? NULL : runtime->pending_request.resource_id);
    printf(",\"host_lock\":%s", host_lock ? "true" : "false");
    fputs("}}\n", stdout);
    return !ferror(stdout);
}

static int number(const char *text, unsigned max, unsigned *out)
{
    char *end;
    unsigned long value;
    const char *p;
    if (*text == '\0')
        return 0;
    for (p = text; *p; ++p)
        if (*p < '0' || *p > '9')
            return 0;
    value = strtoul(text, &end, 10);
    if (*end || value > max)
        return 0;
    *out = (unsigned)value;
    return 1;
}

static int seed_world(RemasterEmeraldSave *save)
{
    RemasterEmeraldOverworldState world;
    RemasterEmeraldPartyPokemon mon;
    const RemasterEmeraldSpeciesInfo *info = remaster_emerald_species_info(277);
    RemasterEmeraldCalculatedStats stats;
    uint8_t ivs[6] = {0}, evs[6] = {0};
    const RemasterR4FixtureMap *house = &gRemasterR4LittlerootMaps[0];
    memset(&world, 0, sizeof(world));
    memset(&mon, 0, sizeof(mon));
    world.map_group = (int8_t)house->group_num;
    world.map_num = (int8_t)house->map_num;
    world.map_layout_id = house->layout_num;
    world.player_x = 8;
    world.player_y = 7;
    world.warp_id = -1;
    world.warp_x = world.warp_y = -1;
    mon.box.personality = 1;
    mon.box.ot_id = UINT32_C(0x11223344);
    mon.box.header_flags = 2;
    if (info == NULL || !remaster_emerald_box_pokemon_set_species(&mon.box, 277)
        || !remaster_emerald_box_pokemon_set_experience(&mon.box,
            remaster_emerald_experience_for_level(info->growth_rate, 5))
        || !remaster_emerald_calculate_stats(277, 5,
            remaster_emerald_box_pokemon_nature(&mon.box), ivs, evs, &stats))
        return 0;
    mon.level = 5;
    mon.hp = mon.max_hp = stats.hp;
    mon.attack = stats.attack;
    mon.defense = stats.defense;
    mon.speed = stats.speed;
    mon.sp_attack = stats.sp_attack;
    mon.sp_defense = stats.sp_defense;
    mon.box.checksum = remaster_emerald_box_pokemon_checksum(&mon.box);
    return remaster_emerald_overworld_set(save, &world)
        && remaster_emerald_flag_set(save, 0x860, 1)
        && remaster_emerald_flag_set(save, 0x52, 1)
        && remaster_emerald_var_set(save, 0x4050, 2)
        && remaster_emerald_var_set(save, 0x4084, 3)
        && remaster_emerald_party_set_count(save, 1)
        && remaster_emerald_party_set(save, 0, &mon);
}

int main(int argc, char **argv)
{
    RemasterEmeraldSave save;
    RemasterEmeraldScriptRuntime runtime;
    RemasterEmeraldOverworldActionResult action;
    int scripts;
    char line[512];
    unsigned index = 0;
    int host_lock = 0;
    if (argc != 2 || (strcmp(argv[1], "opening-male-v1") != 0
        && strcmp(argv[1], "opening-female-v1") != 0 && strcmp(argv[1], "house-exit-v1") != 0))
        return 1;
    scripts = strcmp(argv[1], "house-exit-v1") != 0;
    memset(&save, 0, sizeof(save));
    memset(&runtime, 0, sizeof(runtime));
    memset(&action, 0, sizeof(action));
    if (scripts) {
        save.save_block2[8] = (uint8_t)(strcmp(argv[1], "opening-female-v1") == 0);
        remaster_emerald_script_runtime_init(&runtime, &save, &gR2LittlerootRegistry);
        remaster_emerald_script_runtime_set_special_registry(&runtime, &kSpecials);
    } else if (!seed_world(&save))
        return 1;
    if (!snapshot(&save, scripts ? &runtime : NULL, &action, host_lock))
        return 1;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        char op[32], a[128], b[128], c[128], extra[2];
        int count = sscanf(line, "%31s %127s %127s %127s %1s", op, a, b, c, extra);
        unsigned x, y, z;
        if (++index > 4096 || count < 1 || strchr(line, '\n') == NULL)
            goto rejected;
        if (strcmp(op, "script_start") == 0 && count == 2 && scripts) {
            if (remaster_emerald_script_runtime_dispatch_script_id(&runtime, a)
                != REMASTER_EMERALD_SCRIPT_DISPATCH_STARTED)
                goto rejected;
        } else if (strcmp(op, "script_run") == 0 && count == 2 && scripts && number(a, 4096, &x) && x > 0) {
            if (runtime.vm.status != REMASTER_EMERALD_SCRIPT_RUNNING
                && runtime.vm.status != REMASTER_EMERALD_SCRIPT_STEP_LIMIT)
                goto rejected;
            if (remaster_emerald_script_runtime_run(&runtime, x) == REMASTER_EMERALD_SCRIPT_ERROR)
                goto rejected;
        } else if (strcmp(op, "script_complete") == 0 && count == 4 && scripts
            && number(a, 65535, &x) && number(b, 65535, &y) && number(c, 65535, &z)) {
            RemasterEmeraldScriptRequest request;
            RemasterEmeraldScriptCompletion completion;
            if (!remaster_emerald_script_runtime_pending_request(&runtime, &request)
                || (unsigned)request.type != x || request.action != y
                || request.type == REMASTER_EMERALD_SCRIPT_REQUEST_WARP
                || request.type == REMASTER_EMERALD_SCRIPT_REQUEST_DOMAIN
                || request.type == REMASTER_EMERALD_SCRIPT_REQUEST_STARTER_SELECTION)
                goto rejected;
            if (request.type == REMASTER_EMERALD_SCRIPT_REQUEST_WORLD) {
                /* Explicit headless scope owner; mutating world/follower actions remain handoffs. */
                if (request.action < REMASTER_EMERALD_SCRIPT_WORLD_LOCK
                    || request.action > REMASTER_EMERALD_SCRIPT_WORLD_RELEASE_ALL)
                    goto rejected;
                host_lock = request.action == REMASTER_EMERALD_SCRIPT_WORLD_LOCK
                    || request.action == REMASTER_EMERALD_SCRIPT_WORLD_LOCK_ALL;
            }
            memset(&completion, 0, sizeof(completion));
            completion.type = request.type;
            completion.sequence = request.sequence;
            completion.local_id = request.local_id;
            completion.map_id = request.map_id;
            completion.result_u16 = (uint16_t)z;
            completion.accepted = 1;
            if (!remaster_emerald_script_runtime_complete(&runtime, &completion))
                goto rejected;
        } else if (strcmp(op, "step") == 0 && count == 2 && !scripts) {
            RemasterEmeraldOverworldState world;
            RemasterEmeraldMovementContext movement;
            const RemasterR4FixtureMap *map;
            uint8_t direction;
            if (strcmp(a, "south") == 0) direction = REMASTER_EMERALD_DIR_SOUTH;
            else if (strcmp(a, "north") == 0) direction = REMASTER_EMERALD_DIR_NORTH;
            else if (strcmp(a, "west") == 0) direction = REMASTER_EMERALD_DIR_WEST;
            else if (strcmp(a, "east") == 0) direction = REMASTER_EMERALD_DIR_EAST;
            else goto rejected;
            if (!remaster_emerald_overworld_get(&save, &world)) goto rejected;
            map = remaster_r4_fixture_find_map(world.map_group, world.map_num);
            if (map == NULL) goto rejected;
            memset(&movement, 0, sizeof(movement));
            movement.map = &map->view;
            if (!remaster_emerald_overworld_step_action(&save, &movement,
                map->connections, map->connection_count, map->coord_events, map->coord_event_count,
                map->warps, map->warp_count, direction, &action)) goto rejected;
        } else if (strcmp(op, "warp") == 0 && count == 1 && !scripts) {
            RemasterEmeraldOverworldState world;
            RemasterEmeraldWarpState destination;
            const RemasterR4FixtureMap *map, *target;
            const RemasterEmeraldWarpEventDef *warp;
            if (action.kind != REMASTER_EMERALD_OVERWORLD_ACTION_WARP
                || !remaster_emerald_overworld_get(&save, &world)) goto rejected;
            map = remaster_r4_fixture_find_map(world.map_group, world.map_num);
            if (map == NULL || action.warp_index >= map->warp_count) goto rejected;
            warp = &map->warps[action.warp_index];
            target = remaster_r4_fixture_find_map(warp->dest_map_group, warp->dest_map_num);
            if (target == NULL) goto rejected;
            memset(&destination, 0, sizeof(destination));
            destination.map_group = (int8_t)warp->dest_map_group;
            destination.map_num = (int8_t)warp->dest_map_num;
            destination.warp_id = (int8_t)warp->dest_warp_id;
            destination.x = destination.y = -1;
            if (!remaster_emerald_apply_warp(&save, destination, target->layout_num,
                target->weather_id, target->map_type_id, target->requires_flash,
                (int16_t)target->view.width, (int16_t)target->view.height,
                target->warps, target->warp_count)) goto rejected;
            memset(&action, 0, sizeof(action));
        } else if (strcmp(op, "connection") == 0 && count == 1 && !scripts) {
            RemasterEmeraldOverworldState world;
            const RemasterR4FixtureMap *map, *target;
            const RemasterEmeraldConnectionDef *connection;
            if (action.kind != REMASTER_EMERALD_OVERWORLD_ACTION_CONNECTION
                || !remaster_emerald_overworld_get(&save, &world)) goto rejected;
            map = remaster_r4_fixture_find_map(world.map_group, world.map_num);
            if (map == NULL || action.connection_index >= map->connection_count) goto rejected;
            connection = &map->connections[action.connection_index];
            target = remaster_r4_fixture_find_map(connection->dest_map_group, connection->dest_map_num);
            if (target == NULL || !remaster_emerald_apply_connection_transition(&save, connection,
                target->layout_num, target->weather_id, target->map_type_id, target->requires_flash)) goto rejected;
            memset(&action, 0, sizeof(action));
        } else if (strcmp(op, "save_roundtrip") == 0 && count == 1) {
            RemasterEmeraldSave decoded;
            uint8_t *image = (uint8_t *)malloc(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
            int ok;
            if (image == NULL) goto rejected;
            memset(image, 0xff, REMASTER_EMERALD_SAVE_IMAGE_BYTES);
            ok = remaster_emerald_save_encode_next(image, REMASTER_EMERALD_SAVE_IMAGE_BYTES, &save)
                && remaster_emerald_save_decode(image, REMASTER_EMERALD_SAVE_IMAGE_BYTES, &decoded) == REMASTER_EMERALD_SAVE_OK;
            free(image);
            if (!ok || memcmp(save.save_block2, decoded.save_block2, sizeof(save.save_block2)) != 0
                || memcmp(save.save_block1, decoded.save_block1, sizeof(save.save_block1)) != 0
                || memcmp(save.pokemon_storage, decoded.pokemon_storage, sizeof(save.pokemon_storage)) != 0)
                goto rejected;
            save = decoded;
        } else goto rejected;
        if (!snapshot(&save, scripts ? &runtime : NULL, &action, host_lock)) goto rejected;
    }
    return ferror(stdin) || ferror(stdout) || index == 0 ? 1 : 0;
rejected:
    fprintf(stderr, "rejected command at index %u\n", index);
    return 1;
}
