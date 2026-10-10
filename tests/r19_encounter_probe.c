#include "remaster/emerald_encounter.h"
#include "remaster/emerald_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hex(const uint8_t *bytes, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) printf("%02x", (unsigned)bytes[i]);
}

static int seed_number(const char *s, uint32_t *value)
{
    const char *p;
    unsigned long long parsed;
    if (*s == '\0' || strlen(s) > 10) return 0;
    for (p = s; *p; ++p) if (*p < '0' || *p > '9') return 0;
    parsed = strtoull(s, NULL, 10);
    if (parsed > UINT32_MAX) return 0;
    *value = (uint32_t)parsed;
    return 1;
}

static int seed_party(RemasterEmeraldSave *save)
{
    RemasterEmeraldPartyPokemon mon;
    RemasterEmeraldCalculatedStats stats;
    const RemasterEmeraldSpeciesInfo *info = remaster_emerald_species_info(277);
    uint8_t ivs[6] = {0}, evs[6] = {0};
    memset(&mon, 0, sizeof(mon));
    mon.box.personality = 1;
    mon.box.ot_id = UINT32_C(0x11223344);
    mon.box.header_flags = 2;
    if (info == NULL || !remaster_emerald_box_pokemon_set_species(&mon.box, 277)
        || !remaster_emerald_box_pokemon_set_experience(&mon.box,
            remaster_emerald_experience_for_level(info->growth_rate, 20))
        || !remaster_emerald_calculate_stats(277, 20,
            remaster_emerald_box_pokemon_nature(&mon.box), ivs, evs, &stats)) return 0;
    mon.level = 20;
    mon.hp = mon.max_hp = stats.hp;
    mon.attack = stats.attack;
    mon.defense = stats.defense;
    mon.speed = stats.speed;
    mon.sp_attack = stats.sp_attack;
    mon.sp_defense = stats.sp_defense;
    mon.box.checksum = remaster_emerald_box_pokemon_checksum(&mon.box);
    return remaster_emerald_party_set_count(save, 1) && remaster_emerald_party_set(save, 0, &mon);
}

static int snapshot(const RemasterEmeraldSave *save, const RemasterEmeraldEncounterRuntime *r,
    const RemasterEmeraldEncounterStepContext *context, const RemasterEmeraldEncounterResult *result)
{
    size_t i;
    uint16_t repel = 0;
    if (!remaster_emerald_var_get(save, 0x4021, &repel)) return 0;
    fputs("{\"version\":1,\"format\":\"vanillaplus\",\"domains\":{\"save_block2\":\"", stdout);
    hex(save->save_block2, sizeof(save->save_block2));
    fputs("\",\"save_block1\":\"", stdout);
    hex(save->save_block1, sizeof(save->save_block1));
    fputs("\",\"storage\":\"", stdout);
    hex(save->pokemon_storage, sizeof(save->pokemon_storage));
    fputs("\",\"script\":null,\"objects\":null,\"battle\":null,\"encounter\":{\"version\":1,", stdout);
    printf("\"rng\":{\"state\":%lu,\"calls\":%llu},\"species_bag\":[",
        (unsigned long)r->rng.state, (unsigned long long)r->rng.calls);
    for (i = 0; i < REMASTER_EMERALD_ENCOUNTER_MAX_LOCAL_POOL; ++i)
        printf("%s%u", i ? "," : "", (unsigned)r->species_bag[i]);
    fputs("],", stdout);
#define FIELD(name) printf("\"" #name "\":%u,", (unsigned)r->name)
    FIELD(species_bag_count); FIELD(species_bag_cursor); FIELD(species_bag_habitat_mask);
    FIELD(last_species); FIELD(species_bag_map_group); FIELD(species_bag_map_num);
    FIELD(species_bag_area); FIELD(species_bag_rod); FIELD(species_bag_valid);
    FIELD(roamer_map_group); FIELD(roamer_map_num); FIELD(roamer_location_valid);
    FIELD(wild_immunity_steps); FIELD(previous_behavior); FIELD(previous_behavior_valid);
#undef FIELD
    fputs("\"roamer_location_history\":[", stdout);
    for (i = 0; i < 3; ++i)
        printf("%s[%u,%u]", i ? "," : "", (unsigned)r->roamer_location_history[i][0],
            (unsigned)r->roamer_location_history[i][1]);
    fputs("]}},\"observations\":{", stdout);
#define FIELD(name) printf("\"" #name "\":%u,", (unsigned)result->name)
    FIELD(occurred); FIELD(repel_wore_off); FIELD(kind); FIELD(area); FIELD(rod);
    FIELD(level); FIELD(nature); FIELD(gender); FIELD(ability_num); FIELD(species); FIELD(modified_rate);
#undef FIELD
    printf("\"rng_calls_before\":%llu,\"rng_calls_after\":%llu,\"repel\":%u,",
        (unsigned long long)result->rng_calls_before, (unsigned long long)result->rng_calls_after, (unsigned)repel);
    printf("\"map_group\":%u,\"map_num\":%u,\"immunity\":%u,\"rng_state\":%lu,\"rng_calls\":%llu,\"pokemon\":",
        (unsigned)context->map_group, (unsigned)context->map_num, (unsigned)r->wild_immunity_steps,
        (unsigned long)r->rng.state, (unsigned long long)r->rng.calls);
    if (result->occurred) {
        RemasterEmeraldSave codec;
        memset(&codec, 0, sizeof(codec));
        if (!remaster_emerald_party_set(&codec, 0, &result->pokemon)) return 0;
        putchar('"');
        hex(codec.save_block1 + 0x238, REMASTER_EMERALD_PARTY_POKEMON_BYTES);
        putchar('"');
    } else fputs("null", stdout);
    fputs("}}\n", stdout);
    return !ferror(stdout);
}

int main(int argc, char **argv)
{
    RemasterEmeraldSave save;
    RemasterEmeraldEncounterRuntime runtime;
    RemasterEmeraldEncounterStepContext context;
    RemasterEmeraldEncounterResult result;
    const RemasterEmeraldEncounterMapInfo *map;
    uint32_t seed;
    unsigned mode = 0, recipe_rod = 0, index = 0;
    char line[128];
    if ((argc != 3 && argc != 4) || !seed_number(argv[2], &seed)) return 1;
    memset(&save, 0, sizeof(save));
    memset(&context, 0, sizeof(context));
    memset(&result, 0, sizeof(result));
    context.map_num = 16;
    context.current_behavior = context.previous_behavior = 2;
    remaster_emerald_encounter_runtime_init(&runtime, seed);
    if (!seed_party(&save)) return 1;
    if (strcmp(argv[1], "land-route101-v1") == 0) {
        /* Explicit post-movement trigger context; no R4 movement is claimed. */
    } else if (strcmp(argv[1], "repel-expiry-v1") == 0) {
        runtime.wild_immunity_steps = 4;
        runtime.previous_behavior = 2;
        runtime.previous_behavior_valid = 1;
        if (!remaster_emerald_var_set(&save, 0x4021, 1)) return 1;
    } else if (strcmp(argv[1], "disabled-route101-v1") == 0) {
        context.encounters_disabled = 1;
    } else if (strcmp(argv[1], "water-route102-v1") == 0) {
        context.map_num = 17;
        context.current_behavior = context.previous_behavior = 0x10;
    } else if (strcmp(argv[1], "rocks-route111-v1") == 0) {
        mode = 2;
        context.map_num = 26;
    } else {
        if (strcmp(argv[1], "old-rod-route102-v1") == 0) recipe_rod = 1;
        else if (strcmp(argv[1], "good-rod-route102-v1") == 0) recipe_rod = 2;
        else if (strcmp(argv[1], "super-rod-route102-v1") == 0) recipe_rod = 3;
        else return 1;
        mode = 1;
        context.map_num = 17;
        if (!remaster_emerald_var_set(&save, 0x4021, 100)) return 1;
    }
    map = remaster_emerald_encounter_map_find(context.map_group, context.map_num);
    if (map == NULL || (mode == 1 && !map->fishing.anchor_count)
        || (mode == 2 && !map->rocks.anchor_count)
        || (mode == 0 && !(context.map_num == 17 ? map->water.anchor_count : map->land.anchor_count))) return 1;
    if (argc == 4) {
        if (strcmp(argv[3], "--transport-noise") == 0) {
            save.counter = 777; save.selected_slot = 1; save.last_written_sector = 13;
        } else if (strcmp(argv[3], "--runtime-noise") == 0) runtime.species_bag[63] ^= 1;
        else return 1;
    }
    if (!snapshot(&save, &runtime, &context, &result)) return 1;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        char op[32], arg[16], extra[2];
        int count = sscanf(line, "%31s %15s %1s", op, arg, extra);
        uint32_t rod;
        int occurred;
        if (++index > 4096 || count < 1 || strchr(line, '\n') == NULL) goto rejected;
        memset(&result, 0, sizeof(result));
        if (mode == 0 && count == 1 && strcmp(op, "encounter_step") == 0)
            occurred = remaster_emerald_encounter_step(&runtime, &save, &context, &result);
        else if (mode == 1 && count == 2 && strcmp(op, "encounter_fishing") == 0
            && seed_number(arg, &rod) && rod == recipe_rod)
            occurred = remaster_emerald_encounter_fishing(&runtime, &save, context.map_group,
                context.map_num, (uint8_t)rod, &result);
        else if (mode == 2 && count == 1 && strcmp(op, "encounter_rock_smash") == 0)
            occurred = remaster_emerald_encounter_rock_smash(&runtime, &save,
                context.map_group, context.map_num, &result);
        else goto rejected;
        if (!!occurred != !!result.occurred || !snapshot(&save, &runtime, &context, &result)) goto rejected;
    }
    return ferror(stdin) || ferror(stdout) || index == 0 ? 1 : 0;
rejected:
    fprintf(stderr, "rejected encounter command at index %u\n", index);
    return 1;
}
