#!/usr/bin/env python3
"""Build the deterministic R3 source-world catalog from a Vanilla+ tree."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1


class SourceCatalogError(ValueError):
    pass


def _load_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SourceCatalogError(f"missing source file: {path}") from exc


def _map_files(source_root: Path) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for path in sorted((source_root / "data/maps").glob("*/map.json")):
        name = path.parent.name
        if name in result:
            raise SourceCatalogError(f"duplicate map directory: {name}")
        result[name] = path
    return result


def _script_files(source_root: Path) -> set[str]:
    return {
        path.parent.name
        for path in (source_root / "data/maps").glob("*/scripts.inc")
        if path.is_file()
    }


def build_source_catalog(
    source_root: Path,
    *,
    source_commit: str | None = None,
) -> dict[str, Any]:
    source_root = Path(source_root)
    groups_path = source_root / "data/maps/map_groups.json"
    layouts_path = source_root / "data/layouts/layouts.json"

    groups = _load_json(groups_path)
    layouts_doc = _load_json(layouts_path)

    group_order = groups.get("group_order")
    if not isinstance(group_order, list):
        raise SourceCatalogError(f"{groups_path}: group_order must be an array")

    layout_entries = layouts_doc.get("layouts")
    if not isinstance(layout_entries, list):
        raise SourceCatalogError(f"{layouts_path}: layouts must be an array")

    layout_ids: list[str] = []
    seen_layouts: set[str] = set()
    for index, entry in enumerate(layout_entries):
        if not isinstance(entry, dict) or not isinstance(entry.get("id"), str):
            raise SourceCatalogError(
                f"{layouts_path}: layout index {index} has no string id"
            )
        layout_id = entry["id"]
        if layout_id in seen_layouts:
            raise SourceCatalogError(f"duplicate layout id: {layout_id}")
        seen_layouts.add(layout_id)
        layout_ids.append(layout_id)

    files_by_name = _map_files(source_root)
    script_names = _script_files(source_root)

    ordered_names: list[str] = []
    seen_names: set[str] = set()
    for group_num, group_name in enumerate(group_order):
        members = groups.get(group_name)
        if not isinstance(group_name, str) or not isinstance(members, list):
            raise SourceCatalogError(
                f"{groups_path}: invalid group entry {group_name!r}"
            )
        for map_num, map_name in enumerate(members):
            if not isinstance(map_name, str):
                raise SourceCatalogError(
                    f"{groups_path}: {group_name}[{map_num}] is not a map name"
                )
            if map_name in seen_names:
                raise SourceCatalogError(
                    f"{groups_path}: duplicate map name {map_name}"
                )
            seen_names.add(map_name)
            ordered_names.append(map_name)

    missing = [name for name in ordered_names if name not in files_by_name]
    if missing:
        raise SourceCatalogError(
            f"{missing[0]}: missing map.json referenced by map_groups"
        )

    extras = sorted(set(files_by_name) - seen_names)
    if extras:
        raise SourceCatalogError(
            f"{extras[0]}: map.json is not present in map_groups"
        )

    maps: list[dict[str, Any]] = []
    seen_ids: set[str] = set()

    for group_num, group_name in enumerate(group_order):
        members = groups[group_name]
        for map_num, map_name in enumerate(members):
            path = files_by_name[map_name]
            doc = _load_json(path)

            if doc.get("name") != map_name:
                raise SourceCatalogError(
                    f"{path}: map name {doc.get('name')!r} does not match {map_name!r}"
                )

            map_id = doc.get("id")
            if not isinstance(map_id, str) or not map_id:
                raise SourceCatalogError(f"{path}: missing map id")
            if map_id in seen_ids:
                raise SourceCatalogError(f"duplicate map id: {map_id}")
            seen_ids.add(map_id)

            layout_id = doc.get("layout")
            if layout_id not in seen_layouts:
                raise SourceCatalogError(
                    f"{path}: unknown layout {layout_id!r}"
                )

            shared_scripts_map = doc.get("shared_scripts_map")
            if shared_scripts_map is not None:
                if (
                    not isinstance(shared_scripts_map, str)
                    or shared_scripts_map not in seen_names
                ):
                    raise SourceCatalogError(
                        f"{path}: invalid shared_scripts_map {shared_scripts_map!r}"
                    )

            if map_name in script_names:
                ownership = {"kind": "own", "owner": map_name}
            elif shared_scripts_map is not None:
                ownership = {"kind": "shared", "owner": shared_scripts_map}
            else:
                ownership = {"kind": "none", "owner": None}

            maps.append(
                {
                    "id": map_id,
                    "name": map_name,
                    "group_name": group_name,
                    "group_num": group_num,
                    "map_num": map_num,
                    "layout": layout_id,
                    "script_ownership": ownership,
                }
            )

    orphan_scripts = sorted(script_names - seen_names)
    if orphan_scripts:
        raise SourceCatalogError(
            f"{orphan_scripts[0]}: scripts.inc has no map in map_groups"
        )

    return {
        "schema_version": SCHEMA_VERSION,
        "source_commit": source_commit,
        "group_count": len(group_order),
        "map_count": len(maps),
        "layout_count": len(layout_ids),
        "map_script_file_count": len(script_names),
        "group_order": list(group_order),
        "layout_ids": layout_ids,
        "maps": maps,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--source-commit")
    args = parser.parse_args()

    catalog = build_source_catalog(
        args.source_root.resolve(),
        source_commit=args.source_commit,
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(catalog, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
