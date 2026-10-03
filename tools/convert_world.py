#!/usr/bin/env python3
"""Convert pokeemerald/Vanilla+ world files into remaster intermediate JSON.

This tool intentionally preserves raw metatile words rather than guessing at
rendering semantics. Gameplay and presentation decoding are separate phases.
"""

from __future__ import annotations

import argparse
import ast
import json
import re
import struct
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def list_field(document: dict[str, Any], key: str) -> list[dict[str, Any]]:
    value = document.get(key)
    if value is None:
        return []
    if not isinstance(value, list):
        raise ValueError(f"{key} must be an array or null")
    return value


def read_u16_le(path: Path) -> list[int]:
    data = path.read_bytes()
    if len(data) % 2:
        raise ValueError(f"{path}: block data length is not 16-bit aligned")

    return list(struct.unpack(f"<{len(data) // 2}H", data))


def build_tileset_attribute_index(source_root: Path) -> dict[str, Path]:
    headers_path = source_root / "src/data/tilesets/headers.h"
    metatiles_path = source_root / "src/data/tilesets/metatiles.h"

    headers_text = headers_path.read_text(encoding="utf-8")
    metatiles_text = metatiles_path.read_text(encoding="utf-8")

    symbol_paths: dict[str, Path] = {}
    for match in re.finditer(
        r'const\s+u16\s+(gMetatileAttributes_[A-Za-z0-9_]+)\[\]\s*=\s*'
        r'INCBIN_U16\("([^"]+)"\);',
        metatiles_text,
    ):
        symbol_paths[match.group(1)] = Path(match.group(2))

    result: dict[str, Path] = {}
    tileset_pattern = re.compile(
        r'const\s+struct\s+Tileset\s+(gTileset_[A-Za-z0-9_]+)\s*=\s*'
        r'\{(.*?)\};',
        re.DOTALL,
    )
    attribute_pattern = re.compile(
        r'\.metatileAttributes\s*=\s*'
        r'(gMetatileAttributes_[A-Za-z0-9_]+)'
    )

    for match in tileset_pattern.finditer(headers_text):
        tileset_name = match.group(1)
        attr_match = attribute_pattern.search(match.group(2))
        if not attr_match:
            raise ValueError(
                f"{headers_path}: {tileset_name} has no metatileAttributes"
            )

        symbol = attr_match.group(1)
        if symbol not in symbol_paths:
            raise ValueError(
                f"{metatiles_path}: no INCBIN path for {symbol} "
                f"used by {tileset_name}"
            )

        result[tileset_name] = source_root / symbol_paths[symbol]

    return result


def _eval_int_expr(expr: str, symbols: dict[str, int]) -> int:
    node = ast.parse(expr, mode="eval")

    def visit(value: ast.AST) -> int:
        if isinstance(value, ast.Expression):
            return visit(value.body)
        if isinstance(value, ast.Constant) and isinstance(value.value, int):
            return int(value.value)
        if isinstance(value, ast.Name):
            if value.id not in symbols:
                raise KeyError(value.id)
            return symbols[value.id]
        if isinstance(value, ast.UnaryOp):
            operand = visit(value.operand)
            if isinstance(value.op, ast.UAdd):
                return operand
            if isinstance(value.op, ast.USub):
                return -operand
            if isinstance(value.op, ast.Invert):
                return ~operand
        if isinstance(value, ast.BinOp):
            left = visit(value.left)
            right = visit(value.right)
            if isinstance(value.op, ast.Add):
                return left + right
            if isinstance(value.op, ast.Sub):
                return left - right
            if isinstance(value.op, ast.Mult):
                return left * right
            if isinstance(value.op, ast.FloorDiv):
                return left // right
            if isinstance(value.op, ast.Div):
                return left // right
            if isinstance(value.op, ast.Mod):
                return left % right
            if isinstance(value.op, ast.LShift):
                return left << right
            if isinstance(value.op, ast.RShift):
                return left >> right
            if isinstance(value.op, ast.BitOr):
                return left | right
            if isinstance(value.op, ast.BitAnd):
                return left & right
            if isinstance(value.op, ast.BitXor):
                return left ^ right
        raise ValueError(f"unsupported integer expression: {expr}")

    return visit(node)


def build_numeric_constant_index(source_root: Path) -> dict[str, int]:
    paths = [
        source_root / "include/constants/flags.h",
        source_root / "include/constants/vars.h",
        source_root / "include/constants/opponents.h",
        source_root / "include/constants/weather.h",
        source_root / "include/constants/maps.h",
        source_root / "include/constants/layouts.h",
        source_root / "include/constants/map_types.h",
        source_root / "include/constants/items.h",
        source_root / "include/constants/event_bg.h",
        source_root / "include/constants/secret_bases.h",
        source_root / "include/constants/berry.h",
        source_root / "include/constants/event_objects.h",
        source_root / "include/constants/event_object_movement.h",
        source_root / "include/constants/trainer_types.h",
    ]

    expressions: dict[str, str] = {}
    pattern = re.compile(
        r"^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.+?)\s*$"
    )

    for path in paths:
        if not path.is_file():
            continue

        for raw_line in path.read_text(encoding="utf-8").splitlines():
            line = raw_line.split("//", 1)[0].strip()
            if not line:
                continue

            match = pattern.match(line)
            if not match:
                continue

            name = match.group(1)
            expr = match.group(2).strip()

            # Ignore function-like or string macros; only integer constants
            # participate in event/save identity conversion.
            if '"' in expr or "'" in expr:
                continue

            expressions[name] = expr

    symbols: dict[str, int] = {}
    pending = dict(expressions)

    while pending:
        progressed = False

        for name, expr in list(pending.items()):
            cleaned = expr
            cleaned = re.sub(
                r"\b(?:U|UL|ULL|L|LL)\b",
                "",
                cleaned,
            )
            cleaned = re.sub(
                r"(?<=\d)[uUlL]+\b",
                "",
                cleaned,
            )

            try:
                symbols[name] = _eval_int_expr(cleaned, symbols)
            except (KeyError, SyntaxError, ValueError, ZeroDivisionError):
                continue

            del pending[name]
            progressed = True

        if not progressed:
            break

    return symbols


def resolve_numeric(value: Any, constants: dict[str, int]) -> int | None:
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, int):
        return value
    if not isinstance(value, str):
        return None

    token = value.strip()
    if token in constants:
        return constants[token]

    try:
        return int(token, 0)
    except ValueError:
        return None


def build_map_location_index(source_root: Path) -> dict[str, tuple[int, int, str]]:
    groups_path = source_root / "data/maps/map_groups.json"
    groups = load_json(groups_path)

    result: dict[str, tuple[int, int, str]] = {}
    group_order = groups.get("group_order", [])

    for group_num, group_name in enumerate(group_order):
        if group_name not in groups:
            raise ValueError(
                f"{groups_path}: group_order references missing {group_name}"
            )

        for map_num, map_name in enumerate(groups[group_name]):
            if map_name in result:
                raise ValueError(
                    f"{groups_path}: duplicate map name {map_name}"
                )
            result[map_name] = (group_num, map_num, group_name)

    return result


def normalize_event(
    event: dict[str, Any],
    constants: dict[str, int],
) -> dict[str, Any]:
    # Preserve symbolic names and add numeric identities next to them.
    result = dict(event)

    for source_key, numeric_key in (
        ("flag", "flag_id"),
        ("var", "var_id"),
        ("var_value", "var_value_u16"),
        ("trainer_sight_or_berry_tree_id", "trainer_sight_or_berry_tree_id_u16"),
        ("dest_warp_id", "dest_warp_id_u16"),
        ("weather", "weather_id"),
        ("item", "item_id"),
        ("player_facing_dir", "player_facing_dir_id"),
        ("secret_base_id", "secret_base_id_u16"),
        ("graphics_id", "graphics_id_u16"),
        ("movement_type", "movement_type_u8"),
        ("trainer_type", "trainer_type_u16"),
    ):
        if source_key in result:
            numeric = resolve_numeric(result[source_key], constants)
            if numeric is not None:
                result[numeric_key] = numeric

    return result




def normalize_background_event(
    event: dict[str, Any],
    constants: dict[str, int],
) -> dict[str, Any]:
    item = normalize_event(event, constants)
    event_type = item.get("type")

    if event_type == "sign":
        facing = item.get("player_facing_dir_id")
        if isinstance(facing, int):
            item["kind_id"] = facing
    elif event_type == "hidden_item":
        item["kind_id"] = 7
    elif event_type == "secret_base":
        item["kind_id"] = 8

    return item


def normalize_object_events(
    events: list[dict[str, Any]],
    constants: dict[str, int],
) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []

    for index, event in enumerate(events):
        item = normalize_event(event, constants)
        # Vanilla mapjson generates object_event/clone_event local IDs from
        # the array index, starting at 1. Preserve that exact identity.
        item["local_id"] = index + 1
        result.append(item)

    return result


def add_numeric_map_targets(
    events: list[dict[str, Any]],
    map_id_locations: dict[str, tuple[int, int]],
) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []

    for event in events:
        item = dict(event)
        target = item.get("dest_map", item.get("map"))

        if target == "MAP_DYNAMIC":
            item["dynamic_target"] = True
        elif isinstance(target, str) and target in map_id_locations:
            group_num, map_num = map_id_locations[target]
            item["dest_group_num"] = group_num
            item["dest_map_num"] = map_num
            item["dynamic_target"] = False

        result.append(item)

    return result


def convert_map(
    source_root: Path,
    map_path: Path,
    layouts: dict[str, dict[str, Any]],
    tileset_attributes: dict[str, Path],
    map_locations: dict[str, tuple[int, int, str]],
    constants: dict[str, int],
    map_id_locations: dict[str, tuple[int, int]],
) -> dict[str, Any]:
    source = load_json(map_path)
    layout_id = source["layout"]
    map_name = source["name"]

    event_source = source
    shared_events_map = source.get("shared_events_map")
    shared_events_json: Path | None = None
    if shared_events_map is not None:
        if not isinstance(shared_events_map, str) or not shared_events_map:
            raise ValueError(
                f"{map_path}: shared_events_map must be a non-empty map name"
            )
        shared_events_json = (
            source_root / "data/maps" / shared_events_map / "map.json"
        )
        if not shared_events_json.is_file():
            raise FileNotFoundError(
                f"{map_path}: shared events map not found: {shared_events_json}"
            )
        event_source = load_json(shared_events_json)
        if event_source.get("name") != shared_events_map:
            raise ValueError(
                f"{shared_events_json}: shared map name mismatch"
            )

    if map_name not in map_locations:
        raise KeyError(
            f"{map_path}: map {map_name} is missing from map_groups.json"
        )

    map_group, map_num, map_group_name = map_locations[map_name]

    if layout_id not in layouts:
        raise KeyError(f"{map_path}: unknown layout {layout_id}")

    layout = layouts[layout_id]
    width = int(layout["width"])
    height = int(layout["height"])

    primary_tileset = layout["primary_tileset"]
    secondary_tileset = layout["secondary_tileset"]

    if primary_tileset not in tileset_attributes:
        raise KeyError(
            f"{map_path}: unknown primary tileset {primary_tileset}"
        )
    if secondary_tileset not in tileset_attributes:
        raise KeyError(
            f"{map_path}: unknown secondary tileset {secondary_tileset}"
        )

    primary_attributes_path = tileset_attributes[primary_tileset]
    secondary_attributes_path = tileset_attributes[secondary_tileset]
    primary_attributes = read_u16_le(primary_attributes_path)
    secondary_attributes = read_u16_le(secondary_attributes_path)

    if len(primary_attributes) > 512:
        raise ValueError(
            f"{primary_attributes_path}: primary attributes exceed 512 entries"
        )
    if len(secondary_attributes) > 512:
        raise ValueError(
            f"{secondary_attributes_path}: secondary attributes exceed 512 entries"
        )

    block_path = source_root / layout["blockdata_filepath"]
    source_words = read_u16_le(block_path)
    active_word_count = width * height

    border_path = source_root / layout["border_filepath"]
    border_source_words = read_u16_le(border_path)
    if len(border_source_words) < 4:
        raise ValueError(
            f"{border_path}: expected at least 4 Emerald border words, "
            f"got {len(border_source_words)}"
        )
    border_words = border_source_words[:4]
    border_trailing_words = border_source_words[4:]

    if len(source_words) < active_word_count:
        raise ValueError(
            f"{block_path}: expected at least {active_word_count} blocks, "
            f"got {len(source_words)}"
        )

    blocks = source_words[:active_word_count]
    trailing_words = source_words[active_word_count:]

    metatile_ids = [word & 0x03FF for word in blocks]
    collision = [(word & 0x0C00) >> 10 for word in blocks]
    elevation = [(word & 0xF000) >> 12 for word in blocks]

    map_weather_id = resolve_numeric(source.get("weather"), constants)
    map_type_id = resolve_numeric(source.get("map_type"), constants)

    if map_weather_id is None:
        raise ValueError(
            f"{map_path}: unresolved map weather {source.get('weather')!r}"
        )
    if map_type_id is None:
        raise ValueError(
            f"{map_path}: unresolved map type {source.get('map_type')!r}"
        )

    return {
        "schema_version": SCHEMA_VERSION,
        "source": {
            "map_json": str(map_path.relative_to(source_root)).replace("\\", "/"),
            "blockdata": str(block_path.relative_to(source_root)).replace("\\", "/"),
            "border": str(border_path.relative_to(source_root)).replace("\\", "/"),
            "primary_metatile_attributes": str(
                primary_attributes_path.relative_to(source_root)
            ).replace("\\", "/"),
            "secondary_metatile_attributes": str(
                secondary_attributes_path.relative_to(source_root)
            ).replace("\\", "/"),
            "shared_events_json": (
                str(shared_events_json.relative_to(source_root)).replace("\\", "/")
                if shared_events_json is not None
                else None
            ),
        },
        "map": {
            "id": source["id"],
            "name": map_name,
            "group_name": map_group_name,
            "group_num": map_group,
            "map_num": map_num,
            "layout": layout_id,
            "layout_num": int(layout["_numeric_id"]),
            "music": source.get("music"),
            "region_map_section": source.get("region_map_section"),
            "requires_flash": bool(source.get("requires_flash", False)),
            "weather": source.get("weather"),
            "weather_id": map_weather_id,
            "map_type": source.get("map_type"),
            "map_type_id": map_type_id,
            "allow_cycling": bool(source.get("allow_cycling", False)),
            "allow_escaping": bool(source.get("allow_escaping", False)),
            "allow_running": bool(source.get("allow_running", False)),
            "show_map_name": bool(source.get("show_map_name", False)),
            "battle_scene": source.get("battle_scene"),
            "shared_events_map": shared_events_map,
            "shared_scripts_map": source.get("shared_scripts_map"),
            "connections": add_numeric_map_targets(
                [
                    normalize_event(x, constants)
                    for x in list_field(source, "connections")
                ],
                map_id_locations,
            ),
            "object_events": normalize_object_events(
                list_field(event_source, "object_events"),
                constants,
            ),
            "warp_events": add_numeric_map_targets(
                [
                    normalize_event(x, constants)
                    for x in list_field(event_source, "warp_events")
                ],
                map_id_locations,
            ),
            "coord_events": [
                normalize_event(x, constants)
                for x in list_field(event_source, "coord_events")
            ],
            "bg_events": [
                normalize_background_event(x, constants)
                for x in list_field(event_source, "bg_events")
            ],
        },
        "layout": {
            "id": layout_id,
            "name": layout.get("name"),
            "width": width,
            "height": height,
            "primary_tileset": primary_tileset,
            "secondary_tileset": secondary_tileset,
            "primary_metatile_attributes_u16": primary_attributes,
            "secondary_metatile_attributes_u16": secondary_attributes,
            "primary_metatile_behavior_u8": [
                value & 0x00FF for value in primary_attributes
            ],
            "secondary_metatile_behavior_u8": [
                value & 0x00FF for value in secondary_attributes
            ],
            "primary_metatile_layer_u8": [
                (value & 0xF000) >> 12 for value in primary_attributes
            ],
            "secondary_metatile_layer_u8": [
                (value & 0xF000) >> 12 for value in secondary_attributes
            ],
            "source_word_count": len(source_words),
            "active_word_count": active_word_count,
            "border_source_word_count": len(border_source_words),
            "border_active_words_u16": border_words,
            "border_trailing_words_u16": border_trailing_words,
            "raw_blocks_u16": blocks,
            "trailing_words_u16": trailing_words,
            "metatile_ids_u16": metatile_ids,
            "collision_u8": collision,
            "elevation_u8": elevation,
        },
    }


def convert_world(source_root: Path, output_root: Path) -> dict[str, Any]:
    layouts_doc = load_json(source_root / "data/layouts/layouts.json")
    constants = build_numeric_constant_index(source_root)

    layouts: dict[str, dict[str, Any]] = {}
    for index, entry in enumerate(layouts_doc["layouts"]):
        layout_id = entry["id"]
        numeric_id = constants.get(layout_id)

        if numeric_id is None:
            raise KeyError(
                f"include/constants/layouts.h has no numeric id for {layout_id}"
            )

        expected_from_order = index + 1
        if numeric_id != expected_from_order:
            raise ValueError(
                f"{layout_id}: layouts.json index implies {expected_from_order}, "
                f"but layouts.h defines {numeric_id}"
            )

        layouts[layout_id] = {
            **entry,
            "_numeric_id": numeric_id,
        }

    tileset_attributes = build_tileset_attribute_index(source_root)
    map_locations = build_map_location_index(source_root)

    map_files = sorted((source_root / "data/maps").glob("*/map.json"))

    map_id_locations: dict[str, tuple[int, int]] = {}
    for map_path in map_files:
        source = load_json(map_path)
        map_name = source["name"]
        if map_name not in map_locations:
            raise KeyError(
                f"{map_path}: map {map_name} is missing from map_groups.json"
            )
        group_num, map_num, _ = map_locations[map_name]
        map_id_locations[source["id"]] = (group_num, map_num)

    output_maps = output_root / "maps"
    output_maps.mkdir(parents=True, exist_ok=True)

    manifest_maps: list[dict[str, Any]] = []

    for map_path in map_files:
        converted = convert_map(
            source_root,
            map_path,
            layouts,
            tileset_attributes,
            map_locations,
            constants,
            map_id_locations,
        )
        map_name = converted["map"]["name"]
        out_path = output_maps / f"{map_name}.json"
        out_path.write_text(
            json.dumps(converted, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )
        manifest_maps.append(
            {
                "id": converted["map"]["id"],
                "name": map_name,
                "file": f"maps/{map_name}.json",
                "width": converted["layout"]["width"],
                "height": converted["layout"]["height"],
                "layout_num": converted["map"]["layout_num"],
                "group_num": converted["map"]["group_num"],
                "map_num": converted["map"]["map_num"],
            }
        )

    manifest = {
        "schema_version": SCHEMA_VERSION,
        "map_count": len(manifest_maps),
        "maps": manifest_maps,
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_root", type=Path)
    args = parser.parse_args()

    manifest = convert_world(args.source_root.resolve(), args.output_root.resolve())
    print(f"Converted {manifest['map_count']} maps.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
