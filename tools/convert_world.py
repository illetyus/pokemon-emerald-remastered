#!/usr/bin/env python3
"""Convert pokeemerald/Vanilla+ world files into remaster intermediate JSON.

This tool intentionally preserves raw metatile words rather than guessing at
rendering semantics. Gameplay and presentation decoding are separate phases.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


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


def normalize_event(event: dict[str, Any]) -> dict[str, Any]:
    # Keep source names/flags/scripts as symbolic identifiers. They are part of
    # the gameplay contract and should not be baked into renderer logic.
    return dict(event)


def convert_map(
    source_root: Path,
    map_path: Path,
    layouts: dict[str, dict[str, Any]],
    tileset_attributes: dict[str, Path],
    map_locations: dict[str, tuple[int, int, str]],
) -> dict[str, Any]:
    source = load_json(map_path)
    layout_id = source["layout"]
    map_name = source["name"]

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
        },
        "map": {
            "id": source["id"],
            "name": map_name,
            "group_name": map_group_name,
            "group_num": map_group,
            "map_num": map_num,
            "layout": layout_id,
            "music": source.get("music"),
            "region_map_section": source.get("region_map_section"),
            "requires_flash": bool(source.get("requires_flash", False)),
            "weather": source.get("weather"),
            "map_type": source.get("map_type"),
            "allow_cycling": bool(source.get("allow_cycling", False)),
            "allow_escaping": bool(source.get("allow_escaping", False)),
            "allow_running": bool(source.get("allow_running", False)),
            "show_map_name": bool(source.get("show_map_name", False)),
            "battle_scene": source.get("battle_scene"),
            "connections": [
                normalize_event(x) for x in source.get("connections", [])
            ],
            "object_events": [
                normalize_event(x) for x in source.get("object_events", [])
            ],
            "warp_events": [
                normalize_event(x) for x in source.get("warp_events", [])
            ],
            "coord_events": [
                normalize_event(x) for x in source.get("coord_events", [])
            ],
            "bg_events": [
                normalize_event(x) for x in source.get("bg_events", [])
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
    layouts = {entry["id"]: entry for entry in layouts_doc["layouts"]}
    tileset_attributes = build_tileset_attribute_index(source_root)
    map_locations = build_map_location_index(source_root)

    map_files = sorted((source_root / "data/maps").glob("*/map.json"))
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
