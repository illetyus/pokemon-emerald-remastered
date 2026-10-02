#!/usr/bin/env python3
"""Convert pokeemerald/Vanilla+ world files into remaster intermediate JSON.

This tool intentionally preserves raw metatile words rather than guessing at
rendering semantics. Gameplay and presentation decoding are separate phases.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def read_u16_le(path: Path, expected_words: int | None = None) -> list[int]:
    data = path.read_bytes()
    if len(data) % 2:
        raise ValueError(f"{path}: block data length is not 16-bit aligned")

    words = list(struct.unpack(f"<{len(data) // 2}H", data))
    if expected_words is not None and len(words) != expected_words:
        raise ValueError(
            f"{path}: expected {expected_words} blocks, got {len(words)}"
        )
    return words


def normalize_event(event: dict[str, Any]) -> dict[str, Any]:
    # Keep source names/flags/scripts as symbolic identifiers. They are part of
    # the gameplay contract and should not be baked into renderer logic.
    return dict(event)


def convert_map(
    source_root: Path,
    map_path: Path,
    layouts: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    source = load_json(map_path)
    layout_id = source["layout"]

    if layout_id not in layouts:
        raise KeyError(f"{map_path}: unknown layout {layout_id}")

    layout = layouts[layout_id]
    width = int(layout["width"])
    height = int(layout["height"])

    block_path = source_root / layout["blockdata_filepath"]
    blocks = read_u16_le(block_path, expected_words=width * height)

    return {
        "schema_version": SCHEMA_VERSION,
        "source": {
            "map_json": str(map_path.relative_to(source_root)).replace("\\", "/"),
            "blockdata": str(block_path.relative_to(source_root)).replace("\\", "/"),
        },
        "map": {
            "id": source["id"],
            "name": source["name"],
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
            "primary_tileset": layout.get("primary_tileset"),
            "secondary_tileset": layout.get("secondary_tileset"),
            "raw_blocks_u16": blocks,
        },
    }


def convert_world(source_root: Path, output_root: Path) -> dict[str, Any]:
    layouts_doc = load_json(source_root / "data/layouts/layouts.json")
    layouts = {entry["id"]: entry for entry in layouts_doc["layouts"]}

    map_files = sorted((source_root / "data/maps").glob("*/map.json"))
    output_maps = output_root / "maps"
    output_maps.mkdir(parents=True, exist_ok=True)

    manifest_maps: list[dict[str, Any]] = []

    for map_path in map_files:
        converted = convert_map(source_root, map_path, layouts)
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
