#!/usr/bin/env python3
"""Decode Vanilla+ metatile binaries into deterministic R5 render descriptors."""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path
from typing import Any

from build_r5_tileset_manifest import build_manifest

SCHEMA_VERSION = 1

TILE_ID_MASK = 0x03FF
H_FLIP_MASK = 0x0400
V_FLIP_MASK = 0x0800
PALETTE_MASK = 0xF000
PALETTE_SHIFT = 12

LAYER_PLANES = {
    0: ("middle", "top"),
    1: ("bottom", "middle"),
    2: ("bottom", "top"),
}


def read_u16_le(path: Path) -> list[int]:
    data = path.read_bytes()
    if len(data) % 2:
        raise ValueError(f"{path}: data is not u16 aligned")
    return list(struct.unpack(f"<{len(data) // 2}H", data))


def decode_entry(word: int, entry_index: int) -> dict[str, Any]:
    quadrant = entry_index % 4
    return {
        "entry_index": entry_index,
        "source_layer": entry_index // 4,
        "quadrant": quadrant,
        "x": quadrant % 2,
        "y": quadrant // 2,
        "tile_id_raw": word & TILE_ID_MASK,
        "h_flip": bool(word & H_FLIP_MASK),
        "v_flip": bool(word & V_FLIP_MASK),
        "palette": (word & PALETTE_MASK) >> PALETTE_SHIFT,
        "raw_u16": word,
    }


def decode_tileset(
    source_root: Path,
    source_record: dict[str, Any],
) -> dict[str, Any]:
    metatile_path = source_root / source_record["metatiles_bin"]
    attrs_path = source_root / source_record["metatile_attributes_bin"]

    words = read_u16_le(metatile_path)
    attrs = read_u16_le(attrs_path)

    if len(words) != len(attrs) * 8:
        raise ValueError(
            f"{source_record['id']}: expected 8 u16 entries per metatile, "
            f"got {len(words)} words for {len(attrs)} attributes"
        )

    metatiles: list[dict[str, Any]] = []
    for local_id, attr in enumerate(attrs):
        layer_type = (attr >> 12) & 0xF
        if layer_type not in LAYER_PLANES:
            raise ValueError(
                f"{source_record['id']} metatile {local_id}: "
                f"unsupported layer type {layer_type}"
            )

        start = local_id * 8
        entries = [
            decode_entry(word, entry_index)
            for entry_index, word in enumerate(words[start:start + 8])
        ]

        metatiles.append(
            {
                "local_metatile_id": local_id,
                "behavior": attr & 0x00FF,
                "layer_type": layer_type,
                "render_planes": list(LAYER_PLANES[layer_type]),
                "attribute_u16": attr,
                "entries": entries,
            }
        )

    return {
        "id": source_record["id"],
        "is_secondary": source_record["is_secondary"],
        "asset_root": source_record["asset_root"],
        "tiles_png": source_record["tiles_png"],
        "tiles_png_width": source_record["tiles_png_width"],
        "tiles_png_height": source_record["tiles_png_height"],
        "tile_count": source_record["tile_count"],
        "palette_files": source_record["palette_files"],
        "metatile_count": source_record["metatile_count"],
        "fingerprints": source_record["fingerprints"],
        "metatiles": metatiles,
    }


def build_descriptors(source_root: Path) -> dict[str, Any]:
    source_manifest = build_manifest(source_root)
    tilesets = [
        decode_tileset(source_root, record)
        for record in source_manifest["tilesets"]
    ]
    return {
        "schema_version": SCHEMA_VERSION,
        "entry_format": {
            "tile_id_mask": TILE_ID_MASK,
            "h_flip_mask": H_FLIP_MASK,
            "v_flip_mask": V_FLIP_MASK,
            "palette_mask": PALETTE_MASK,
            "palette_shift": PALETTE_SHIFT,
            "entries_per_metatile": 8,
        },
        "layer_planes": {
            str(key): list(value)
            for key, value in sorted(LAYER_PLANES.items())
        },
        "tileset_count": len(tilesets),
        "tilesets": tilesets,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_json", type=Path)
    args = parser.parse_args()

    descriptors = build_descriptors(args.source_root)
    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(
        json.dumps(descriptors, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    metatile_count = sum(
        item["metatile_count"]
        for item in descriptors["tilesets"]
    )
    print(
        f"R5 metatile descriptors: {descriptors['tileset_count']} tilesets, "
        f"{metatile_count} metatiles -> {args.output_json}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
