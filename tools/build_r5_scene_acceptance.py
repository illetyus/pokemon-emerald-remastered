#!/usr/bin/env python3
"""Build the real R5 Brendan house -> Littleroot -> Route 101 scene report."""

from __future__ import annotations

import argparse
import json
import sys
import tempfile
from pathlib import Path
from typing import Any

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

try:
    from tools.build_r5_render_package import build_render_package
    from tools.convert_world import convert_world
except ModuleNotFoundError:
    from build_r5_render_package import build_render_package
    from convert_world import convert_world

SOURCE_REPOSITORY = "illetyus/pokezumrut-vanillaplus"
SOURCE_COMMIT = "70db90c9077aed1272e746fc2537d9f12b95a91c"

ACCEPTANCE_MAPS = (
    "LittlerootTown_BrendansHouse_1F",
    "LittlerootTown",
    "Route101",
)


def _load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _load_descriptor(
    render_root: Path,
    manifest_by_id: dict[str, dict[str, Any]],
    tileset_id: str,
    cache: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    if tileset_id in cache:
        return cache[tileset_id]

    entry = manifest_by_id.get(tileset_id)
    if entry is None:
        raise AssertionError(f"render package missing tileset {tileset_id}")

    descriptor = _load_json(render_root / entry["descriptor_file"])
    if descriptor["id"] != tileset_id:
        raise AssertionError(
            f"descriptor identity mismatch: expected {tileset_id}, "
            f"got {descriptor.get('id')}"
        )

    index_path = render_root / descriptor["tiles_index8"]
    palette_path = render_root / descriptor["palette_lut_file"]

    expected_index_bytes = (
        int(descriptor["tiles_png_width"])
        * int(descriptor["tiles_png_height"])
    )
    if index_path.stat().st_size != expected_index_bytes:
        raise AssertionError(
            f"{tileset_id}: index payload size mismatch"
        )
    if palette_path.stat().st_size != 16 * 16 * 4:
        raise AssertionError(
            f"{tileset_id}: palette LUT size mismatch"
        )

    cache[tileset_id] = descriptor
    return descriptor


def _resolve_metatile(
    render_root: Path,
    manifest_by_id: dict[str, dict[str, Any]],
    descriptor_cache: dict[str, dict[str, Any]],
    *,
    primary_tileset: str,
    secondary_tileset: str | int | None,
    global_metatile_id: int,
) -> tuple[str, int, dict[str, Any]]:
    if global_metatile_id < 512:
        tileset_id = primary_tileset
        local_id = global_metatile_id
    else:
        if secondary_tileset in (None, 0, "0", ""):
            raise AssertionError(
                f"metatile {global_metatile_id} requires a secondary tileset"
            )
        tileset_id = str(secondary_tileset)
        local_id = global_metatile_id - 512

    descriptor = _load_descriptor(
        render_root,
        manifest_by_id,
        tileset_id,
        descriptor_cache,
    )

    metatiles = descriptor["metatiles"]
    if local_id < 0 or local_id >= len(metatiles):
        raise AssertionError(
            f"{tileset_id}: local metatile {local_id} out of range"
        )

    metatile = metatiles[local_id]
    if int(metatile["local_metatile_id"]) != local_id:
        raise AssertionError(
            f"{tileset_id}: non-dense local metatile id {local_id}"
        )

    planes = metatile["render_planes"]
    if len(planes) != 2:
        raise AssertionError(
            f"{tileset_id}[{local_id}]: expected exactly two render planes"
        )

    entries = metatile["entries"]
    if len(entries) != 8:
        raise AssertionError(
            f"{tileset_id}[{local_id}]: expected exactly eight tile entries"
        )

    for source_layer in (0, 1):
        layer_entries = [
            entry
            for entry in entries
            if int(entry["source_layer"]) == source_layer
        ]
        quadrants = sorted(int(entry["quadrant"]) for entry in layer_entries)
        if quadrants != [0, 1, 2, 3]:
            raise AssertionError(
                f"{tileset_id}[{local_id}] layer {source_layer}: "
                f"quadrants={quadrants}"
            )

    return tileset_id, local_id, metatile


def _map_scene_summary(
    map_doc: dict[str, Any],
    render_root: Path,
    manifest_by_id: dict[str, dict[str, Any]],
    descriptor_cache: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    layout = map_doc["layout"]
    map_info = map_doc["map"]

    width = int(layout["width"])
    height = int(layout["height"])
    metatile_ids = [int(value) for value in layout["metatile_ids_u16"]]

    if len(metatile_ids) != width * height:
        raise AssertionError(
            f"{map_info['id']}: tile count does not match layout dimensions"
        )

    primary = str(layout["primary_tileset"])
    secondary = layout.get("secondary_tileset")

    used_tilesets: set[str] = set()
    unique_metatiles: set[tuple[str, int]] = set()
    plane_instances = 0
    quadrant_entries = 0

    for metatile_id in metatile_ids:
        tileset_id, local_id, metatile = _resolve_metatile(
            render_root,
            manifest_by_id,
            descriptor_cache,
            primary_tileset=primary,
            secondary_tileset=secondary,
            global_metatile_id=metatile_id,
        )
        used_tilesets.add(tileset_id)
        unique_metatiles.add((tileset_id, local_id))
        plane_instances += len(metatile["render_planes"])
        quadrant_entries += len(metatile["entries"])

    if plane_instances != len(metatile_ids) * 2:
        raise AssertionError(
            f"{map_info['id']}: render-plane instance count mismatch"
        )
    if quadrant_entries != len(metatile_ids) * 8:
        raise AssertionError(
            f"{map_info['id']}: render quadrant-entry count mismatch"
        )

    return {
        "id": map_info["id"],
        "name": map_info["name"],
        "group_num": int(map_info["group_num"]),
        "map_num": int(map_info["map_num"]),
        "width": width,
        "height": height,
        "tile_count": len(metatile_ids),
        "primary_tileset": primary,
        "secondary_tileset": secondary,
        "used_tilesets": sorted(used_tilesets),
        "unique_render_metatiles": len(unique_metatiles),
        "render_plane_instances": plane_instances,
        "render_quadrant_entries": quadrant_entries,
    }


def build_scene_acceptance(
    source_root: Path,
    output_json: Path,
) -> dict[str, Any]:
    with tempfile.TemporaryDirectory() as temp:
        temp_root = Path(temp)
        world_root = temp_root / "world"
        render_root = temp_root / "render"

        world_manifest = convert_world(
            source_root,
            world_root,
            source_commit=SOURCE_COMMIT,
            source_repository=SOURCE_REPOSITORY,
        )
        render_manifest = build_render_package(
            source_root,
            render_root,
            source_commit=SOURCE_COMMIT,
            source_repository=SOURCE_REPOSITORY,
        )

        manifest_by_id = {
            item["id"]: item for item in render_manifest["tilesets"]
        }
        descriptor_cache: dict[str, dict[str, Any]] = {}

        documents = {
            name: _load_json(world_root / "maps" / f"{name}.json")
            for name in ACCEPTANCE_MAPS
        }

        house = documents["LittlerootTown_BrendansHouse_1F"]
        town = documents["LittlerootTown"]
        route101 = documents["Route101"]

        house_warp = next(
            (
                warp
                for warp in house["map"]["warp_events"]
                if int(warp["x"]) == 8 and int(warp["y"]) == 8
            ),
            None,
        )
        if house_warp is None:
            raise AssertionError("real Brendan house exit warp (8,8) missing")

        if house_warp["dest_map"] != town["map"]["id"]:
            raise AssertionError(
                "Brendan house exit no longer targets Littleroot"
            )

        north_connection = next(
            (
                connection
                for connection in town["map"]["connections"]
                if connection["direction"] == "up"
            ),
            None,
        )
        if north_connection is None:
            raise AssertionError("Littleroot north connection missing")

        if north_connection["map"] != route101["map"]["id"]:
            raise AssertionError(
                "Littleroot north connection no longer targets Route101"
            )

        map_summaries = [
            _map_scene_summary(
                documents[name],
                render_root,
                manifest_by_id,
                descriptor_cache,
            )
            for name in ACCEPTANCE_MAPS
        ]

        report = {
            "schema_version": 1,
            "status": "PASS",
            "source_repository": SOURCE_REPOSITORY,
            "source_commit": SOURCE_COMMIT,
            "world_map_count": int(world_manifest["counts"]["map_count"]),
            "render_tileset_count": int(render_manifest["tileset_count"]),
            "render_content_sha256": render_manifest["content_sha256"],
            "acceptance_path": {
                "start_map": house["map"]["id"],
                "house_exit": {
                    "x": 8,
                    "y": 8,
                    "dest_map": house_warp["dest_map"],
                    "dest_warp_id": int(house_warp["dest_warp_id_u16"]),
                },
                "town_map": town["map"]["id"],
                "north_connection": {
                    "direction": north_connection["direction"],
                    "dest_map": north_connection["map"],
                    "offset": int(north_connection["offset"]),
                },
                "end_map": route101["map"]["id"],
            },
            "maps": map_summaries,
            "totals": {
                "tile_count": sum(item["tile_count"] for item in map_summaries),
                "render_plane_instances": sum(
                    item["render_plane_instances"] for item in map_summaries
                ),
                "render_quadrant_entries": sum(
                    item["render_quadrant_entries"] for item in map_summaries
                ),
                "loaded_tileset_descriptors": len(descriptor_cache),
            },
        }

        output_json.parent.mkdir(parents=True, exist_ok=True)
        output_json.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_json", type=Path)
    args = parser.parse_args()

    report = build_scene_acceptance(
        args.source_root,
        args.output_json,
    )
    print(
        "R5 real scene acceptance: "
        f"{report['status']}, "
        f"{report['totals']['tile_count']} tiles, "
        f"{report['totals']['render_plane_instances']} planes, "
        f"render_sha256={report['render_content_sha256']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
