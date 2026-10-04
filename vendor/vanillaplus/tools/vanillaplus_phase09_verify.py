#!/usr/bin/env python3
"""Static/source-data checks for Vanilla+ Phase 9 ecological wild encounters."""
from pathlib import Path
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []

def read(path):
    p = ROOT / path
    if not p.exists():
        errors.append(f"missing required file: {path}")
        return ""
    return p.read_text(encoding="utf-8")

def require(source, token, label):
    if token not in source:
        errors.append(f"{label}: missing {token}")

wild = read("src/wild_encounter.c")
battle_commands = read("src/battle_script_commands.c")
engine = read("src/phase9_wild_ecosystem.c")
engine_h = read("include/phase9_wild_ecosystem.h")
ecology = read("src/data/phase9_wild_ecology.h")
makefile = read("Makefile")
strings = read("src/strings.c")
workflow = read(".github/workflows/build.yml")
global_h = read("include/global.h")
encounter_json_text = read("src/data/wild_encounters.json")
ld_script = read("ld_script.txt")
sym_ewram = read("sym_ewram.txt")
map_groups = read("include/constants/map_groups.h")

# Phase 9 is transient runtime state only: no SaveBlock persistence fields.
for forbidden in ["phase9SpeciesBag", "phase9Habitat", "phase9Wild"]:
    if forbidden.lower() in global_h.lower():
        errors.append(f"save-layout guard: unexpected persisted Phase 9 field {forbidden}")

# Habitat metadata: every National Dex species gets a physical habitat.
allowed_habitats = {
    "PHASE9_HABITAT_CAVE",
    "PHASE9_HABITAT_FOREST",
    "PHASE9_HABITAT_GRASSLAND",
    "PHASE9_HABITAT_MOUNTAIN",
    "PHASE9_HABITAT_ROUGH_TERRAIN",
    "PHASE9_HABITAT_SEA",
    "PHASE9_HABITAT_URBAN",
    "PHASE9_HABITAT_WATERS_EDGE",
}
entry_pattern = re.compile(r"^\s*\[(NATIONAL_DEX_[A-Z0-9_]+)\]\s*=\s*([^,]+),\s*$", re.MULTILINE)
entries = entry_pattern.findall(ecology)
if len(entries) != 386:
    errors.append(f"habitat coverage: expected 386 National Dex entries, found {len(entries)}")

habitats_by_name = {}
for dex_name, expression in entries:
    if dex_name in habitats_by_name:
        errors.append(f"habitat coverage: duplicate {dex_name}")
        continue
    tokens = {token.strip() for token in expression.split("|")}
    bad = tokens - allowed_habitats
    if bad:
        errors.append(f"habitat coverage: {dex_name} uses unknown habitat(s) {sorted(bad)}")
    if not tokens:
        errors.append(f"habitat coverage: {dex_name} has empty habitat")
    habitats_by_name[dex_name.removeprefix("NATIONAL_DEX_")] = tokens

if "PHASE9_HABITAT_RARE" in ecology:
    errors.append("rarity policy: canonical rare bucket must be resolved to physical habitats")

for token in [
    "[NATIONAL_DEX_ARTICUNO] = PHASE9_HABITAT_CAVE | PHASE9_HABITAT_MOUNTAIN",
    "[NATIONAL_DEX_ZAPDOS] = PHASE9_HABITAT_ROUGH_TERRAIN | PHASE9_HABITAT_URBAN",
    "[NATIONAL_DEX_MOLTRES] = PHASE9_HABITAT_MOUNTAIN | PHASE9_HABITAT_ROUGH_TERRAIN",
    "[NATIONAL_DEX_MEW] = PHASE9_HABITAT_FOREST",
    "[NATIONAL_DEX_LUGIA] = PHASE9_HABITAT_SEA",
    "[NATIONAL_DEX_RAYQUAZA] = PHASE9_HABITAT_MOUNTAIN",
    "[NATIONAL_DEX_DEOXYS] = PHASE9_HABITAT_MOUNTAIN | PHASE9_HABITAT_ROUGH_TERRAIN",
]:
    require(ecology, token, "rare-to-physical habitat mapping")

# The new module must be explicitly registered in pokeemerald's ordered linker layout.
require(ld_script, "src/phase9_wild_ecosystem.o(.text);", "Phase 9 linker text placement")
require(ld_script, "src/phase9_wild_ecosystem.o(.rodata);", "Phase 9 linker rodata placement")
require(sym_ewram, '.include "src/phase9_wild_ecosystem.o"', "Phase 9 EWRAM placement")

# Runtime selection: each eligible species appears once in a shuffled bag.
for token in [
    "#define PHASE9_MAX_LOCAL_POOL 64",
    "sPhase9SpeciesBag[PHASE9_MAX_LOCAL_POOL]",
    "IsPhase9LocalPoolSpecies",
    "GetPhase9LocalPoolDivisor",
    "GetPhase9LocalSpeciesHash",
    "if (sPhase9SpeciesBagCount < PHASE9_MAX_LOCAL_POOL)",
    "for (i = sPhase9SpeciesBagCount; i > 1; i--)",
    "Random() % i",
    "sPhase9SpeciesBag[0] == sPhase9LastSpecies",
    "sPhase9SpeciesBagCursor >= sPhase9SpeciesBagCount",
    "SpeciesToNationalPokedexNum(species)",
    "sPhase9HabitatByNationalDex[nationalDexNum] & habitatMask",
    "species == SPECIES_MR_MIME",
]:
    require(engine, token, "equal-probability shuffle bag")

# Field-test tuning: Ultra Ball is intentionally 4x (40 / 10 in catch formula).
require(
    battle_commands,
    "[ITEM_ULTRA_BALL - ITEM_ULTRA_BALL]  = 40,",
    "Ultra Ball 4x catch multiplier",
)

# Dynamic level = rounded average of living non-egg party members +/- 15, clamped 2..100.
for token in [
    "#define PHASE9_LEVEL_DELTA 15",
    "#define PHASE9_LEVEL_MIN   2",
    "#define PHASE9_LEVEL_MAX   100",
    "MON_DATA_SPECIES_OR_EGG",
    "MON_DATA_IS_EGG",
    "MON_DATA_HP",
    "MON_DATA_LEVEL",
    "averageLevel = (totalLevel + eligiblePartyCount / 2) / eligiblePartyCount;",
    "minLevel = averageLevel - PHASE9_LEVEL_DELTA;",
    "maxLevel = averageLevel + PHASE9_LEVEL_DELTA;",
    "Random() % (maxLevel - minLevel + 1)",
]:
    require(engine, token, "dynamic level scaling")

# Normal Hoenn encounters use Phase 9; Frontier special modes keep vanilla generation.
for token in [
    '#include "phase9_wild_ecosystem.h"',
    "TryGeneratePhase9WildMon(gWildMonHeaders[headerId].landMonsInfo, PHASE9_AREA_LAND",
    "TryGeneratePhase9WildMon(gWildMonHeaders[headerId].waterMonsInfo, PHASE9_AREA_WATER",
    "TryGeneratePhase9WildMon(wildPokemonInfo, PHASE9_AREA_ROCKS",
    "GeneratePhase9FishingWildMon(gWildMonHeaders[headerId].fishingMonsInfo, rod)",
    "Phase9ChooseWildSpecies(landMonsInfo, PHASE9_AREA_LAND, 0, FALSE)",
    "Phase9ChooseWildSpecies(waterMonsInfo, PHASE9_AREA_WATER, 0, FALSE)",
]:
    require(wild, token, "Phase 9 encounter routing")

if "TryGenerateWildMon(gBattlePikeWildMonHeaders" not in wild:
    errors.append("Frontier regression: Battle Pike must retain vanilla generator")
if "TryGenerateWildMon(gBattlePyramidWildMonHeaders" not in wild:
    errors.append("Frontier regression: Battle Pyramid must retain vanilla generator")

fish_start = wild.find("void FishingWildEncounter(u8 rod)")
fish_end = wild.find("u16 GetLocalWildMon", fish_start)
fish_block = wild[fish_start:fish_end] if fish_start >= 0 and fish_end >= 0 else ""
if not fish_block:
    errors.append("fishing routing: FishingWildEncounter block missing")
else:
    if "CheckFeebas()" in fish_block:
        errors.append("equal-probability policy: Feebas special tile override is still active")
    require(fish_block, "GeneratePhase9FishingWildMon", "equal-probability fishing")

# Static/ability species bias must not be applied inside the Phase 9 picker.
for forbidden in ["ABILITY_STATIC", "ABILITY_MAGNET_PULL", "TryGetAbilityInfluencedWildMonIndex"]:
    if forbidden in engine:
        errors.append(f"equal-probability policy: Phase 9 engine contains species bias {forbidden}")

# Reproduce the C route-local ecology algorithm exactly enough to catch
# accidental bias, oversized local pools, or any species becoming unreachable.
try:
    encounter_data = json.loads(encounter_json_text)
except Exception as exc:
    errors.append(f"wild encounter JSON parse failed: {exc}")
    encounter_data = {}

habitat_order = [
    "PHASE9_HABITAT_CAVE",
    "PHASE9_HABITAT_FOREST",
    "PHASE9_HABITAT_GRASSLAND",
    "PHASE9_HABITAT_MOUNTAIN",
    "PHASE9_HABITAT_ROUGH_TERRAIN",
    "PHASE9_HABITAT_SEA",
    "PHASE9_HABITAT_URBAN",
    "PHASE9_HABITAT_WATERS_EDGE",
]
habitat_bits = {name: 1 << i for i, name in enumerate(habitat_order)}
habitat_mask_by_name = {
    species: sum(habitat_bits[token] for token in tokens)
    for species, tokens in habitats_by_name.items()
}

map_ids = {}
map_pattern = re.compile(
    r"^#define\s+(MAP_[A-Z0-9_]+)\s+\((\d+)\s*\|\s*\((\d+)\s*<<\s*8\)\)",
    re.MULTILINE,
)
for map_name, map_num, map_group in map_pattern.findall(map_groups):
    map_ids[map_name] = (int(map_group), int(map_num))

normal_group = None
for group in encounter_data.get("wild_encounter_groups", []):
    if group.get("label") == "gWildMonHeaders":
        normal_group = group
        break

def profile_mask(mons):
    counts = [0] * len(habitat_order)
    seen = set()
    for mon in mons:
        species = mon["species"].removeprefix("SPECIES_")
        if species in seen:
            continue
        seen.add(species)
        mask = habitat_mask_by_name.get(species, 0)
        for habitat in range(len(habitat_order)):
            if mask & (1 << habitat):
                counts[habitat] += 1

    best_habitat = 0
    second_habitat = 0
    best_count = 0
    second_count = 0
    for habitat, count in enumerate(counts):
        if count > best_count:
            second_count = best_count
            second_habitat = best_habitat
            best_count = count
            best_habitat = habitat
        elif count > second_count:
            second_count = count
            second_habitat = habitat

    if best_count == 0:
        return 0

    mask = 1 << best_habitat
    if second_count != 0 and second_count * 2 >= best_count:
        mask |= 1 << second_habitat
    return mask

def apply_map_override(map_name, area, mask):
    if area not in ("land", "rocks"):
        return mask

    volcanic = {
        "MAP_MT_CHIMNEY", "MAP_JAGGED_PASS", "MAP_FIERY_PATH",
        "MAP_MAGMA_HIDEOUT_1F", "MAP_MAGMA_HIDEOUT_2F_1R",
        "MAP_MAGMA_HIDEOUT_2F_2R", "MAP_MAGMA_HIDEOUT_3F_1R",
        "MAP_MAGMA_HIDEOUT_3F_2R", "MAP_MAGMA_HIDEOUT_4F",
        "MAP_MAGMA_HIDEOUT_3F_3R", "MAP_MAGMA_HIDEOUT_2F_3R",
    }
    mt_pyre = {
        "MAP_MT_PYRE_1F", "MAP_MT_PYRE_2F", "MAP_MT_PYRE_3F",
        "MAP_MT_PYRE_4F", "MAP_MT_PYRE_5F", "MAP_MT_PYRE_6F",
        "MAP_MT_PYRE_EXTERIOR", "MAP_MT_PYRE_SUMMIT",
    }
    shoal = {
        "MAP_SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM",
        "MAP_SHOAL_CAVE_LOW_TIDE_INNER_ROOM",
        "MAP_SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM",
        "MAP_SHOAL_CAVE_LOW_TIDE_LOWER_ROOM",
        "MAP_SHOAL_CAVE_HIGH_TIDE_ENTRANCE_ROOM",
        "MAP_SHOAL_CAVE_HIGH_TIDE_INNER_ROOM",
        "MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM",
    }
    desert = {
        "MAP_ROUTE111", "MAP_DESERT_RUINS", "MAP_MIRAGE_TOWER_1F",
        "MAP_MIRAGE_TOWER_2F", "MAP_MIRAGE_TOWER_3F",
        "MAP_MIRAGE_TOWER_4F", "MAP_DESERT_UNDERPASS",
    }
    seafloor = {
        "MAP_SEAFLOOR_CAVERN_ENTRANCE", "MAP_SEAFLOOR_CAVERN_ROOM1",
        "MAP_SEAFLOOR_CAVERN_ROOM2", "MAP_SEAFLOOR_CAVERN_ROOM3",
        "MAP_SEAFLOOR_CAVERN_ROOM4", "MAP_SEAFLOOR_CAVERN_ROOM5",
        "MAP_SEAFLOOR_CAVERN_ROOM6", "MAP_SEAFLOOR_CAVERN_ROOM7",
        "MAP_SEAFLOOR_CAVERN_ROOM8", "MAP_SEAFLOOR_CAVERN_ROOM9",
    }

    if map_name in volcanic:
        return habitat_bits["PHASE9_HABITAT_MOUNTAIN"] | habitat_bits["PHASE9_HABITAT_ROUGH_TERRAIN"]
    if map_name in mt_pyre:
        return habitat_bits["PHASE9_HABITAT_CAVE"] | habitat_bits["PHASE9_HABITAT_FOREST"]
    if map_name in shoal:
        return habitat_bits["PHASE9_HABITAT_CAVE"] | habitat_bits["PHASE9_HABITAT_MOUNTAIN"]
    if map_name in {"MAP_NEW_MAUVILLE_ENTRANCE", "MAP_NEW_MAUVILLE_INSIDE"}:
        return habitat_bits["PHASE9_HABITAT_URBAN"] | habitat_bits["PHASE9_HABITAT_ROUGH_TERRAIN"]
    if map_name in desert:
        return habitat_bits["PHASE9_HABITAT_ROUGH_TERRAIN"] | habitat_bits["PHASE9_HABITAT_GRASSLAND"]
    if map_name in seafloor:
        return habitat_bits["PHASE9_HABITAT_CAVE"] | habitat_bits["PHASE9_HABITAT_ROUGH_TERRAIN"]
    return mask

def local_divisor(candidate_count):
    if candidate_count > 120:
        return 6
    if candidate_count > 80:
        return 4
    if candidate_count > 50:
        return 3
    if candidate_count > 16:
        return 2
    return 1

def u32(value):
    return value & 0xFFFFFFFF

def local_hash(national_dex, map_group, map_num, area, rod):
    value = u32(national_dex * 0x045D9F3B)
    value ^= u32((map_group + 1) * 0x27D4EB2D)
    value ^= u32((map_num + 1) * 0x165667B1)
    value ^= u32((area + 1) * 0x1B873593)
    value ^= u32((rod + 1) * 0x85EBCA6B)
    value = u32(value)
    value ^= value >> 16
    value = u32(value * 0x7FEB352D)
    value ^= value >> 15
    return u32(value)

if normal_group is None:
    errors.append("wild encounter JSON: gWildMonHeaders missing")
    reachable_habitats = set()
    local_pool_sizes = []
else:
    species_order = [dex_name.removeprefix("NATIONAL_DEX_") for dex_name, _ in entries]
    national_dex_by_name = {name: i + 1 for i, name in enumerate(species_order)}
    reach_count = {name: 0 for name in species_order}
    reachable_habitats = set()
    local_pool_sizes = []

    def check_context(encounter, area, rod, mons):
        map_name = encounter["map"]
        if map_name not in map_ids:
            errors.append(f"map id missing for {map_name}")
            return

        map_group, map_num = map_ids[map_name]
        area_id = {"land": 0, "water": 1, "rocks": 2, "fishing": 3}[area]
        rod_id = {"none": 0, "old": 0, "good": 1, "super": 2}[rod]
        mask = apply_map_override(map_name, area, profile_mask(mons))
        originals = {mon["species"].removeprefix("SPECIES_") for mon in mons}
        candidate_count = sum(1 for name in species_order if habitat_mask_by_name[name] & mask)
        divisor = local_divisor(candidate_count)
        pool = []

        for name in species_order:
            national_dex = national_dex_by_name[name]
            if name in originals:
                include = True
            elif not (habitat_mask_by_name[name] & mask):
                include = False
            elif (
                name == "MR_MIME"
                and area == "land"
                and map_name in {"MAP_NEW_MAUVILLE_ENTRANCE", "MAP_NEW_MAUVILLE_INSIDE"}
            ):
                include = True
            else:
                include = local_hash(national_dex, map_group, map_num, area_id, rod_id) % divisor == 0

            if include:
                pool.append(name)
                reach_count[name] += 1

        local_pool_sizes.append(len(pool))
        if not pool:
            errors.append(f"local pool empty: {map_name} {area}/{rod}")
        if len(pool) > 64:
            errors.append(f"local pool exceeds PHASE9_MAX_LOCAL_POOL: {map_name} {area}/{rod} = {len(pool)}")

        for habitat, bit in habitat_bits.items():
            if mask & bit:
                reachable_habitats.add(habitat)

    for encounter in normal_group.get("encounters", []):
        if "land_mons" in encounter:
            check_context(encounter, "land", "none", encounter["land_mons"]["mons"])
        if "water_mons" in encounter:
            check_context(encounter, "water", "none", encounter["water_mons"]["mons"])
        if "rock_smash_mons" in encounter:
            check_context(encounter, "rocks", "none", encounter["rock_smash_mons"]["mons"])
        if "fishing_mons" in encounter:
            mons = encounter["fishing_mons"]["mons"]
            check_context(encounter, "fishing", "old", mons[0:2])
            check_context(encounter, "fishing", "good", mons[2:5])
            check_context(encounter, "fishing", "super", mons[5:10])

    unreachable = [name for name, count in reach_count.items() if count == 0]
    if unreachable:
        errors.append(f"386-species local-pool coverage: {len(unreachable)} unreachable: {unreachable[:20]}")

    if len(set(reach_count)) != 386:
        errors.append("386-species coverage table is incomplete")

# Completed phases are version-agnostic but still require synchronized markers.
release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
continue_match = re.search(r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)', strings)
if not release_match or not test_match or not continue_match:
    errors.append("Phase 9 synchronized build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    errors.append("Phase 9 release/test/Continue build markers must stay synchronized")
require(workflow, "python3 tools/vanillaplus_phase09_verify.py", "permanent CI Phase 9 verifier")

if errors:
    print("Vanilla+ Phase 9 verification FAILED:")
    for error in errors:
        print(" - " + error)
    sys.exit(1)

print("Vanilla+ Phase 9 verification PASSED")
print(f" - habitat entries: {len(entries)}/386")
if normal_group is not None:
    print(f" - reachable habitat classes: {len(reachable_habitats)}/{len(allowed_habitats)}")
    print(f" - local pool size range: {min(local_pool_sizes)}..{max(local_pool_sizes)}")
    print(" - route-local species coverage: 386/386")
