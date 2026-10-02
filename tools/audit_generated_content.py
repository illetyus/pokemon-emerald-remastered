#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    manifest_path = root / "manifest.json"

    if not manifest_path.is_file():
        return [f"missing manifest: {manifest_path}"]

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    maps = manifest.get("maps", [])

    if manifest.get("map_count") != len(maps):
        errors.append("manifest map_count does not match maps array")

    known_ids = {entry.get("id") for entry in maps}
    known_files = set()

    for entry in maps:
        rel = entry.get("file")
        if not rel:
            errors.append(f"map {entry.get('id')} has no file")
            continue

        if rel in known_files:
            errors.append(f"duplicate map file: {rel}")
        known_files.add(rel)

        path = root / rel
        if not path.is_file():
            errors.append(f"missing map file: {rel}")
            continue

        doc = json.loads(path.read_text(encoding="utf-8"))
        map_doc = doc.get("map", {})
        layout = doc.get("layout", {})

        if doc.get("schema_version") != 1:
            errors.append(f"{rel}: unsupported schema version")

        width = int(layout.get("width", 0))
        height = int(layout.get("height", 0))
        blocks = layout.get("raw_blocks_u16", [])
        trailing = layout.get("trailing_words_u16", [])
        source_word_count = int(layout.get("source_word_count", len(blocks) + len(trailing)))
        active_word_count = int(layout.get("active_word_count", len(blocks)))
        border_active = layout.get("border_active_words_u16", [])
        border_trailing = layout.get("border_trailing_words_u16", [])
        border_source_word_count = int(
            layout.get(
                "border_source_word_count",
                len(border_active) + len(border_trailing),
            )
        )

        metatile_ids = layout.get("metatile_ids_u16", [])
        collision = layout.get("collision_u8", [])
        elevation = layout.get("elevation_u8", [])

        primary_attrs = layout.get("primary_metatile_attributes_u16", [])
        secondary_attrs = layout.get("secondary_metatile_attributes_u16", [])
        primary_behavior = layout.get("primary_metatile_behavior_u8", [])
        secondary_behavior = layout.get("secondary_metatile_behavior_u8", [])
        primary_layer = layout.get("primary_metatile_layer_u8", [])
        secondary_layer = layout.get("secondary_metatile_layer_u8", [])

        if width <= 0 or height <= 0:
            errors.append(f"{rel}: invalid dimensions")
        elif active_word_count != width * height:
            errors.append(
                f"{rel}: active_word_count {active_word_count} != {width}x{height}"
            )
        elif len(blocks) != active_word_count:
            errors.append(
                f"{rel}: active block count {len(blocks)} != {active_word_count}"
            )

        if source_word_count != len(blocks) + len(trailing):
            errors.append(
                f"{rel}: source_word_count does not match active + trailing words"
            )

        if len(border_active) != 4:
            errors.append(
                f"{rel}: Emerald border must expose exactly 4 active words"
            )

        if border_source_word_count != len(border_active) + len(border_trailing):
            errors.append(
                f"{rel}: border_source_word_count does not match active + trailing words"
            )

        for name, values in (
            ("metatile_ids_u16", metatile_ids),
            ("collision_u8", collision),
            ("elevation_u8", elevation),
        ):
            if len(values) != active_word_count:
                errors.append(
                    f"{rel}: {name} count {len(values)} != active word count "
                    f"{active_word_count}"
                )

        for side, attrs, behavior, layer in (
            ("primary", primary_attrs, primary_behavior, primary_layer),
            ("secondary", secondary_attrs, secondary_behavior, secondary_layer),
        ):
            if not attrs:
                errors.append(f"{rel}: {side} metatile attributes are empty")
            if len(attrs) > 512:
                errors.append(
                    f"{rel}: {side} metatile attributes exceed 512 entries"
                )
            if len(behavior) != len(attrs):
                errors.append(
                    f"{rel}: {side} behavior count does not match attributes"
                )
            if len(layer) != len(attrs):
                errors.append(
                    f"{rel}: {side} layer count does not match attributes"
                )

        for metatile_id in metatile_ids:
            if not isinstance(metatile_id, int) or metatile_id < 0 or metatile_id >= 1024:
                errors.append(f"{rel}: invalid metatile id {metatile_id!r}")
                continue

            if metatile_id < 512:
                if metatile_id >= len(primary_attrs):
                    errors.append(
                        f"{rel}: primary metatile {metatile_id} has no attribute entry"
                    )
            else:
                secondary_index = metatile_id - 512
                if secondary_index >= len(secondary_attrs):
                    errors.append(
                        f"{rel}: secondary metatile {metatile_id} has no attribute entry"
                    )

        if map_doc.get("id") != entry.get("id"):
            errors.append(f"{rel}: manifest/map id mismatch")

        for connection in map_doc.get("connections", []):
            target = connection.get("map")
            if target and target not in known_ids:
                errors.append(
                    f"{rel}: connection references unknown map {target}"
                )

        for warp in map_doc.get("warp_events", []):
            target = warp.get("dest_map")
            if target and target not in known_ids:
                errors.append(
                    f"{rel}: warp references unknown map {target}"
                )

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("generated_root", type=Path)
    args = parser.parse_args()

    errors = audit(args.generated_root)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print("Generated content audit passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
