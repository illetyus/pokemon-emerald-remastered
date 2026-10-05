#!/usr/bin/env python3
"""Generate R12 encounter data from the pinned Vanilla+ source tree."""

from __future__ import annotations

import ast
import json
from pathlib import Path
import re
import sys
from typing import Any


class CatalogError(RuntimeError):
    pass


class Resolver:
    def __init__(self) -> None:
        self.expr: dict[str, str] = {
            "TRUE": "1",
            "FALSE": "0",
        }
        self.cache: dict[str, int] = {}

    def add_text(self, text: str) -> None:
        text = re.sub(r"\\\r?\n", " ", text)
        for raw in text.splitlines():
            raw = re.sub(r"//.*$", "", raw).strip()
            m = re.match(r"#define\s+([A-Za-z_]\w*)\s+(.+)$", raw)
            if not m:
                continue
            name, expression = m.groups()
            if "(" in name:
                continue
            self.expr[name] = expression.strip()

    def resolve(self, name: str) -> int:
        return self.eval(name)

    def eval(self, expression: str, stack: tuple[str, ...] = ()) -> int:
        expression = expression.strip().rstrip(",")
        expression = re.sub(r"/\*.*?\*/", "", expression)
        expression = re.sub(
            r"\b(0x[0-9A-Fa-f]+|\d+)[uUlL]+\b",
            r"\1",
            expression,
        )
        expression = re.sub(r"\((?:u|s)?(?:8|16|32|64)\)", "", expression)

        if re.fullmatch(r"[A-Za-z_]\w*", expression):
            if expression in self.cache:
                return self.cache[expression]
            if expression in stack:
                raise CatalogError(f"recursive define: {expression}")
            if expression not in self.expr:
                raise CatalogError(f"unresolved constant: {expression}")
            value = self.eval(
                self.expr[expression],
                stack + (expression,),
            )
            self.cache[expression] = value
            return value

        identifiers = sorted(set(re.findall(r"\b[A-Za-z_]\w*\b", expression)))
        for identifier in identifiers:
            value = self.eval(identifier, stack)
            expression = re.sub(
                rf"\b{re.escape(identifier)}\b",
                str(value),
                expression,
            )

        node = ast.parse(expression, mode="eval").body
        return self._eval_ast(node)

    def _eval_ast(self, node: ast.AST) -> int:
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return int(node.value)
        if isinstance(node, ast.UnaryOp):
            value = self._eval_ast(node.operand)
            if isinstance(node.op, ast.UAdd):
                return value
            if isinstance(node.op, ast.USub):
                return -value
            if isinstance(node.op, ast.Invert):
                return ~value
        if isinstance(node, ast.BinOp):
            left = self._eval_ast(node.left)
            right = self._eval_ast(node.right)
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            if isinstance(node.op, ast.Mult):
                return left * right
            if isinstance(node.op, (ast.Div, ast.FloorDiv)):
                return left // right
            if isinstance(node.op, ast.Mod):
                return left % right
            if isinstance(node.op, ast.BitOr):
                return left | right
            if isinstance(node.op, ast.BitAnd):
                return left & right
            if isinstance(node.op, ast.BitXor):
                return left ^ right
            if isinstance(node.op, ast.LShift):
                return left << right
            if isinstance(node.op, ast.RShift):
                return left >> right
        raise CatalogError(f"unsupported expression: {ast.dump(node)}")


def map_parts(value: int) -> tuple[int, int]:
    return ((value >> 8) & 0xFF, value & 0xFF)


def map_directory_index(vendor: Path) -> dict[str, str]:
    payload = json.loads(
        (vendor / "data/maps/map_groups.json").read_text(encoding="utf-8")
    )
    result: dict[str, str] = {}
    for group_name in payload["group_order"]:
        for directory in payload[group_name]:
            map_payload = json.loads(
                (vendor / "data/maps" / directory / "map.json").read_text(
                    encoding="utf-8"
                )
            )
            result[map_payload["id"]] = directory
    return result


def padded(values: list[int], size: int) -> str:
    values = list(values)
    if len(values) > size:
        raise CatalogError(f"too many values: {len(values)} > {size}")
    values.extend([0] * (size - len(values)))
    return "{" + ",".join(str(v) for v in values) + "}"


def area_initializer(
    area: dict[str, Any] | None,
    resolver: Resolver,
) -> str:
    if not area:
        return "{0,0,{0}}"

    anchors = [
        resolver.resolve(mon["species"])
        for mon in area.get("mons", [])
    ]
    if len(anchors) > 12:
        raise CatalogError("area anchor count exceeds R12 max")
    return (
        "{"
        f"{int(area['encounter_rate'])},"
        f"{len(anchors)},"
        f"{padded(anchors, 12)}"
        "}"
    )


def build_maps(
    vendor: Path,
    resolver: Resolver,
) -> tuple[list[str], dict[str, Any]]:
    payload = json.loads(
        (vendor / "src/data/wild_encounters.json").read_text(encoding="utf-8")
    )
    groups = payload["wild_encounter_groups"]
    normal = next(group for group in groups if group["label"] == "gWildMonHeaders")
    pike = next(group for group in groups if group["label"] == "gBattlePikeWildMonHeaders")
    pyramid = next(
        group for group in groups
        if group["label"] == "gBattlePyramidWildMonHeaders"
    )

    directories = map_directory_index(vendor)
    missing_maps: list[str] = []
    missing_sections: list[str] = []
    missing_species: list[str] = []
    rows: list[str] = []

    for encounter in normal["encounters"]:
        map_symbol = encounter["map"]
        try:
            map_value = resolver.resolve(map_symbol)
        except CatalogError:
            missing_maps.append(map_symbol)
            continue

        directory = directories.get(map_symbol)
        if not directory:
            missing_maps.append(map_symbol)
            continue

        map_payload = json.loads(
            (vendor / "data/maps" / directory / "map.json").read_text(
                encoding="utf-8"
            )
        )
        section_symbol = map_payload["region_map_section"]
        try:
            section_id = resolver.resolve(section_symbol)
        except CatalogError:
            missing_sections.append(section_symbol)
            section_id = 0

        for field in (
            "land_mons",
            "water_mons",
            "rock_smash_mons",
            "fishing_mons",
        ):
            for mon in (encounter.get(field) or {}).get("mons", []):
                try:
                    resolver.resolve(mon["species"])
                except CatalogError:
                    missing_species.append(mon["species"])

        group, number = map_parts(map_value)
        rows.append(
            "{"
            f"{group},{number},{section_id},"
            f"{area_initializer(encounter.get('land_mons'), resolver)},"
            f"{area_initializer(encounter.get('water_mons'), resolver)},"
            f"{area_initializer(encounter.get('rock_smash_mons'), resolver)},"
            f"{area_initializer(encounter.get('fishing_mons'), resolver)}"
            "}"
        )

    report = {
        "map_encounter_count": len(normal["encounters"]),
        "land_map_count": sum(
            1 for x in normal["encounters"] if x.get("land_mons")
        ),
        "water_map_count": sum(
            1 for x in normal["encounters"] if x.get("water_mons")
        ),
        "rock_smash_map_count": sum(
            1 for x in normal["encounters"] if x.get("rock_smash_mons")
        ),
        "fishing_map_count": sum(
            1 for x in normal["encounters"] if x.get("fishing_mons")
        ),
        "battle_pyramid_table_count": len(pyramid.get("encounters", [])),
        "battle_pike_table_count": len(pike.get("encounters", [])),
        "missing_map_constants": sorted(set(missing_maps)),
        "missing_region_sections": sorted(set(missing_sections)),
        "missing_anchor_species": sorted(set(missing_species)),
    }
    return rows, report


def build_habitats(
    vendor: Path,
    resolver: Resolver,
) -> tuple[list[int], int]:
    ecology = (
        vendor / "src/data/phase9_wild_ecology.h"
    ).read_text(encoding="utf-8")
    resolver.add_text(ecology)

    habitat_by_suffix: dict[str, int] = {}
    for m in re.finditer(
        r"\[(NATIONAL_DEX_[A-Z0-9_]+)\]\s*=\s*([^,]+),",
        ecology,
    ):
        suffix = m.group(1)[len("NATIONAL_DEX_"):]
        habitat_by_suffix[suffix] = resolver.eval(m.group(2))

    species_text = (
        vendor / "include/constants/species.h"
    ).read_text(encoding="utf-8")
    species_by_id: dict[int, str] = {}
    for m in re.finditer(
        r"#define\s+(SPECIES_[A-Z0-9_]+)\s+(\d+)\b",
        species_text,
    ):
        species_id = int(m.group(2))
        if species_id < resolver.resolve("NUM_SPECIES"):
            species_by_id[species_id] = m.group(1)[len("SPECIES_"):]

    masks = [0] * resolver.resolve("NUM_SPECIES")
    for species_id, suffix in species_by_id.items():
        masks[species_id] = habitat_by_suffix.get(suffix, 0)

    return masks, len(habitat_by_suffix)


def build_overrides_and_guarantees(
    vendor: Path,
    resolver: Resolver,
) -> tuple[list[tuple[int, int, int]], list[tuple[int, int, int, int]]]:
    source = (
        vendor / "src/phase9_wild_ecosystem.c"
    ).read_text(encoding="utf-8")

    start = source.index("static u16 ApplyPhase9MapHabitatOverride")
    end = source.index("static u16 GetPhase9HabitatMask", start)
    block = source[start:end]

    overrides: list[tuple[int, int, int]] = []
    for match in re.finditer(
        r"((?:\s*case\s+MAP_[A-Z0-9_]+\s*:\s*)+)\s*"
        r"return\s+([^;]+);",
        block,
    ):
        mask = resolver.eval(match.group(2))
        for symbol in re.findall(r"case\s+(MAP_[A-Z0-9_]+)", match.group(1)):
            group, number = map_parts(resolver.resolve(symbol))
            overrides.append((group, number, mask))

    g_start = source.index("static bool8 IsPhase9GuaranteedLocalSpecies")
    g_end = source.index("static bool8 IsPhase9LocalPoolSpecies", g_start)
    g_block = source[g_start:g_end]
    species_match = re.search(r"species\s*==\s*(SPECIES_[A-Z0-9_]+)", g_block)
    map_symbols = re.findall(r"MAP_[A-Z0-9_]+", g_block)
    guarantees: list[tuple[int, int, int, int]] = []
    if species_match:
        species_id = resolver.resolve(species_match.group(1))
        for symbol in sorted(set(map_symbols)):
            group, number = map_parts(resolver.resolve(symbol))
            guarantees.append((group, number, 1, species_id))

    return overrides, guarantees


def build_behavior_tables(
    vendor: Path,
    resolver: Resolver,
) -> tuple[list[int], list[int]]:
    source = (
        vendor / "src/metatile_behavior.c"
    ).read_text(encoding="utf-8")
    resolver.add_text(source)
    count = resolver.resolve("NUM_METATILE_BEHAVIORS")
    flags = [0] * count
    bridges = [0] * count

    array_start = source.index("static const u8 sTileBitAttributes")
    array_end = source.index("};", array_start)
    array_block = source[array_start:array_end]
    for match in re.finditer(
        r"\[(MB_[A-Z0-9_]+)\]\s*=\s*([^,]+),",
        array_block,
    ):
        index = resolver.resolve(match.group(1))
        flags[index] = resolver.eval(match.group(2))

    function_start = source.index("bool8 MetatileBehavior_IsBridgeOverWater")
    function_end = source.index("}", function_start)
    function_block = source[function_start:function_end]
    for symbol in set(re.findall(r"MB_[A-Z0-9_]+", function_block)):
        bridges[resolver.resolve(symbol)] = 1

    return flags, bridges


def build_learnsets(
    vendor: Path,
    resolver: Resolver,
) -> tuple[list[tuple[int, int]], list[tuple[int, int]], int]:
    learnset_text = (
        vendor / "src/data/pokemon/level_up_learnsets.h"
    ).read_text(encoding="utf-8")
    pointer_text = (
        vendor / "src/data/pokemon/level_up_learnset_pointers.h"
    ).read_text(encoding="utf-8")

    arrays: dict[str, list[tuple[int, int]]] = {}
    for match in re.finditer(
        r"static const u16\s+(s[A-Za-z0-9_]+LevelUpLearnset)"
        r"\[\]\s*=\s*\{(.*?)\};",
        learnset_text,
        flags=re.S,
    ):
        entries: list[tuple[int, int]] = []
        for item in re.finditer(
            r"LEVEL_UP_MOVE\(\s*(\d+)\s*,\s*(MOVE_[A-Z0-9_]+)\s*\)",
            match.group(2),
        ):
            entries.append(
                (int(item.group(1)), resolver.resolve(item.group(2)))
            )
        arrays[match.group(1)] = entries

    pointer_by_species: dict[int, str] = {}
    for match in re.finditer(
        r"\[(SPECIES_[A-Z0-9_]+)\]\s*=\s*"
        r"(s[A-Za-z0-9_]+LevelUpLearnset)",
        pointer_text,
    ):
        species_id = resolver.resolve(match.group(1))
        if species_id < resolver.resolve("NUM_SPECIES"):
            pointer_by_species[species_id] = match.group(2)

    flat: list[tuple[int, int]] = []
    slices: list[tuple[int, int]] = []
    populated = 0
    for species_id in range(resolver.resolve("NUM_SPECIES")):
        name = pointer_by_species.get(species_id)
        entries = arrays.get(name or "", [])
        start = len(flat)
        flat.extend(entries)
        slices.append((start, len(entries)))
        if entries:
            populated += 1

    return flat, slices, populated


def build_roamer_locations(vendor: Path, resolver: Resolver) -> list[list[int]]:
    source = (vendor / "src/roamer.c").read_text(encoding="utf-8")
    start = source.index("static const u8 sRoamerLocations")
    end = source.index("};", start)
    block = source[start:end]

    rows: list[list[int]] = []
    for match in re.finditer(r"\{([^{}]+)\}", block):
        row: list[int] = []
        tokens = [token.strip() for token in match.group(1).split(",")]
        if len(tokens) != 6:
            continue
        for token in tokens:
            if token == "___":
                row.append(0xFF)
                continue
            route = re.fullmatch(r"MAP_NUM\(([^)]+)\)", token)
            if not route:
                raise CatalogError(f"unhandled roamer token: {token}")
            map_value = resolver.resolve("MAP_" + route.group(1))
            row.append(map_value & 0xFF)
        if all(value == 0xFF for value in row):
            continue
        rows.append(row)
    return rows


def constants_block(resolver: Resolver) -> list[str]:
    pairs = [
        ("R12_VAR_REPEL_STEP_COUNT", "VAR_REPEL_STEP_COUNT"),
        ("R12_FLAG_ENC_UP", "FLAG_SYS_ENC_UP_ITEM"),
        ("R12_FLAG_ENC_DOWN", "FLAG_SYS_ENC_DOWN_ITEM"),
        ("R12_FLAG_SOOTOPOLIS_LEGENDARIES", "FLAG_LEGENDARIES_IN_SOOTOPOLIS"),
        ("R12_ABILITY_STENCH", "ABILITY_STENCH"),
        ("R12_ABILITY_SAND_VEIL", "ABILITY_SAND_VEIL"),
        ("R12_ABILITY_INTIMIDATE", "ABILITY_INTIMIDATE"),
        ("R12_ABILITY_SYNCHRONIZE", "ABILITY_SYNCHRONIZE"),
        ("R12_ABILITY_ILLUMINATE", "ABILITY_ILLUMINATE"),
        ("R12_ABILITY_KEEN_EYE", "ABILITY_KEEN_EYE"),
        ("R12_ABILITY_CUTE_CHARM", "ABILITY_CUTE_CHARM"),
        ("R12_ABILITY_ARENA_TRAP", "ABILITY_ARENA_TRAP"),
        ("R12_ABILITY_WHITE_SMOKE", "ABILITY_WHITE_SMOKE"),
        ("R12_ITEM_CLEANSE_TAG", "ITEM_CLEANSE_TAG"),
        ("R12_ITEM_POKE_BALL", "ITEM_POKE_BALL"),
        ("R12_WEATHER_SANDSTORM", "WEATHER_SANDSTORM"),
        ("R12_SPECIES_MR_MIME", "SPECIES_MR_MIME"),
    ]

    soot_group, soot_num = map_parts(resolver.resolve("MAP_SOOTOPOLIS_CITY"))
    lines = ["enum {"]
    for public, source in pairs:
        lines.append(f"    {public} = {resolver.resolve(source)},")
    lines.extend(
        [
            f"    R12_MAP_SOOTOPOLIS_GROUP = {soot_group},",
            f"    R12_MAP_SOOTOPOLIS_NUM = {soot_num}",
            "};",
        ]
    )
    return lines


def render(
    maps: list[str],
    habitats: list[int],
    overrides: list[tuple[int, int, int]],
    guarantees: list[tuple[int, int, int, int]],
    behavior_flags: list[int],
    bridge_flags: list[int],
    learnsets: list[tuple[int, int]],
    learnset_slices: list[tuple[int, int]],
    roamer_locations: list[list[int]],
    resolver: Resolver,
) -> str:
    lines = [
        "/* AUTO-GENERATED by tools/build_r12_encounter_catalog.py. */",
        "/* Source: pinned vendor/vanillaplus encounter + Phase9 data. */",
        "",
        *constants_block(resolver),
        "",
        "static const RemasterEmeraldEncounterMapInfo kRemasterEncounterMaps[] = {",
    ]
    lines.extend("    " + row + "," for row in maps)
    lines.extend(
        [
            "};",
            "",
            "static const uint16_t kRemasterEncounterSpeciesHabitat[] = {",
        ]
    )
    for i in range(0, len(habitats), 16):
        lines.append("    " + ",".join(str(v) for v in habitats[i:i + 16]) + ",")
    lines.extend(
        [
            "};",
            "",
            "static const RemasterEmeraldEncounterHabitatOverride "
            "kRemasterEncounterHabitatOverrides[] = {",
        ]
    )
    lines.extend(
        f"    {{{g},{n},{m}}}," for g, n, m in overrides
    )
    lines.extend(
        [
            "};",
            "",
            "static const RemasterEmeraldEncounterGuarantee "
            "kRemasterEncounterGuarantees[] = {",
        ]
    )
    lines.extend(
        f"    {{{g},{n},{a},{s}}}," for g, n, a, s in guarantees
    )
    lines.extend(
        [
            "};",
            "",
            "static const uint8_t kRemasterEncounterBehaviorFlags[] = {",
        ]
    )
    for i in range(0, len(behavior_flags), 24):
        lines.append(
            "    " + ",".join(str(v) for v in behavior_flags[i:i + 24]) + ","
        )
    lines.extend(
        [
            "};",
            "",
            "static const uint8_t kRemasterEncounterBridgeFlags[] = {",
        ]
    )
    for i in range(0, len(bridge_flags), 24):
        lines.append(
            "    " + ",".join(str(v) for v in bridge_flags[i:i + 24]) + ","
        )
    lines.extend(
        [
            "};",
            "",
            "static const RemasterEmeraldEncounterLearnsetEntry "
            "kRemasterEncounterLearnsetEntries[] = {",
        ]
    )
    lines.extend(
        f"    {{{level},{move}}}," for level, move in learnsets
    )
    lines.extend(
        [
            "};",
            "",
            "static const RemasterEmeraldEncounterLearnsetSlice "
            "kRemasterEncounterLearnsetSlices[] = {",
        ]
    )
    lines.extend(
        f"    {{{start},{count}}}," for start, count in learnset_slices
    )
    lines.extend(
        [
            "};",
            "",
            "static const uint8_t kRemasterRoamerLocations[][6] = {",
        ]
    )
    lines.extend(
        "    {" + ",".join(str(v) for v in row) + "},"
        for row in roamer_locations
    )
    lines.extend(["};", ""])
    return "\n".join(lines)


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: build_r12_encounter_catalog.py "
            "<vendor-root> <output.inc> <report.json>",
            file=sys.stderr,
        )
        return 2

    vendor = Path(sys.argv[1]).resolve()
    output = Path(sys.argv[2]).resolve()
    report_path = Path(sys.argv[3]).resolve()

    resolver = Resolver()
    for path in sorted((vendor / "include").rglob("*.h")):
        resolver.add_text(path.read_text(encoding="utf-8", errors="ignore"))

    try:
        maps, report = build_maps(vendor, resolver)
        habitats, habitat_count = build_habitats(vendor, resolver)
        overrides, guarantees = build_overrides_and_guarantees(vendor, resolver)
        behavior_flags, bridge_flags = build_behavior_tables(vendor, resolver)
        learnsets, slices, learnset_species_count = build_learnsets(vendor, resolver)
        roamer_locations = build_roamer_locations(vendor, resolver)

        phase9 = (
            vendor / "src/phase9_wild_ecosystem.c"
        ).read_text(encoding="utf-8")

        def phase9_define(name: str) -> int:
            match = re.search(
                rf"#define\s+{re.escape(name)}\s+(\d+)",
                phase9,
            )
            if not match:
                raise CatalogError(f"missing Phase9 define: {name}")
            return int(match.group(1))

        report.update(
            {
                "national_dex_habitat_count": habitat_count,
                "phase9_level_delta": phase9_define("PHASE9_LEVEL_DELTA"),
                "phase9_level_min": phase9_define("PHASE9_LEVEL_MIN"),
                "phase9_level_max": phase9_define("PHASE9_LEVEL_MAX"),
                "phase9_max_local_pool": phase9_define("PHASE9_MAX_LOCAL_POOL"),
                "new_mauville_mr_mime_guarantee": any(
                    entry[3] == resolver.resolve("SPECIES_MR_MIME")
                    for entry in guarantees
                ),
                "metatile_behavior_count": len(behavior_flags),
                "learnset_species_count": learnset_species_count,
                "learnset_entry_count": len(learnsets),
                "habitat_override_map_count": len(overrides),
                "roamer_location_set_count": len(roamer_locations),
            }
        )

        rendered = render(
            maps,
            habitats,
            overrides,
            guarantees,
            behavior_flags,
            bridge_flags,
            learnsets,
            slices,
            roamer_locations,
            resolver,
        )

        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(rendered, encoding="utf-8", newline="\n")
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
            newline="\n",
        )
    except (CatalogError, KeyError, ValueError, SyntaxError, OSError) as exc:
        print(f"R12 encounter catalog generation FAILED: {exc}", file=sys.stderr)
        return 1

    print("R12 encounter catalog generation PASSED")
    print(f" - maps: {report['map_encounter_count']}")
    print(f" - habitats: {report['national_dex_habitat_count']}")
    print(f" - learnset species: {report['learnset_species_count']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
