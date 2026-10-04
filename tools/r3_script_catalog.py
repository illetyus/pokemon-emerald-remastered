#!/usr/bin/env python3
"""Build the full R3 map/script ownership and label catalog."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

from convert_scripts import (
    ScriptConversionError,
    _classification_for,
    _index_script_sources,
    _macro_names,
    _script_targets,
)
from r3_source_catalog import SourceCatalogError, build_source_catalog


SCHEMA_VERSION = 1
NULL_SCRIPT_TOKENS = {"", "0", "0x0", "0X0", "NULL", "null"}


class R3ScriptCatalogError(ValueError):
    pass


def _load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _normalize_script_ref(value: Any) -> str | None:
    if value is None or value == 0:
        return None
    if not isinstance(value, str):
        raise R3ScriptCatalogError(f"invalid script reference {value!r}")
    token = value.strip()
    if token in NULL_SCRIPT_TOKENS:
        return None
    return token


def _owner_map_from_path(path: Path) -> str | None:
    parts = path.as_posix().split("/")
    if len(parts) == 4 and parts[0:2] == ["data", "maps"] and parts[3] == "scripts.inc":
        return parts[2]
    return None


def build_script_catalog(source_root: Path) -> dict[str, Any]:
    source_root = Path(source_root)

    try:
        source_catalog = build_source_catalog(source_root)
        labels, _labels_by_file = _index_script_sources(
            source_root,
            [],
            active_manifest_only=True,
        )
    except (SourceCatalogError, ScriptConversionError) as exc:
        raise R3ScriptCatalogError(str(exc)) from exc

    macro_names = _macro_names(source_root)
    maps_by_name = {item["name"]: item for item in source_catalog["maps"]}
    global_shared_script_sources = source_catalog.get(
        "global_shared_script_sources", {}
    )

    resolved_ownership: dict[str, dict[str, Any]] = {}

    def resolve_ownership(map_name: str, stack: tuple[str, ...] = ()) -> dict[str, Any]:
        if map_name in resolved_ownership:
            return resolved_ownership[map_name]
        if map_name in stack:
            cycle = " -> ".join((*stack, map_name))
            raise R3ScriptCatalogError(f"shared script ownership cycle: {cycle}")

        item = maps_by_name[map_name]
        ownership = item["script_ownership"]
        kind = ownership["kind"]

        if kind == "own":
            result = {
                "kind": "own",
                "owner": map_name,
                "source": f"data/maps/{map_name}/scripts.inc",
            }
        elif kind == "shared":
            owner = ownership["owner"]
            if owner in maps_by_name:
                terminal = resolve_ownership(owner, (*stack, map_name))
                if terminal["kind"] == "none" or terminal["source"] is None:
                    raise R3ScriptCatalogError(
                        f"{map_name}: shared script owner {owner!r} "
                        "has no script source"
                    )
                result = {
                    "kind": "shared",
                    "owner": terminal["owner"],
                    "source": terminal["source"],
                }
            elif owner in global_shared_script_sources:
                result = {
                    "kind": "shared",
                    "owner": owner,
                    "source": global_shared_script_sources[owner],
                }
            else:
                raise R3ScriptCatalogError(
                    f"{map_name}: unknown shared script owner {owner!r}"
                )
        else:
            result = {
                "kind": "none",
                "owner": None,
                "source": None,
            }

        resolved_ownership[map_name] = result
        return result

    map_entries = [
        {
            "map": item["name"],
            "map_id": item["id"],
            "script_ownership": resolve_ownership(item["name"]),
        }
        for item in source_catalog["maps"]
    ]

    command_summary: dict[str, dict[str, Any]] = {}
    label_entries: list[dict[str, Any]] = []

    for label in sorted(labels):
        path, line, commands = labels[label]
        owner_map = _owner_map_from_path(path)
        label_entries.append(
            {
                "script_id": label,
                "source": str(path).replace("\\", "/"),
                "line": line,
                "owner_map": owner_map,
            }
        )

        is_non_script_data = "_Movement_" in label or "_Text_" in label
        for command in commands:
            if is_non_script_data:
                classification = "SOURCE_ONLY"
            else:
                classification = _classification_for(command.name, macro_names)
                if classification is None:
                    classification = "SOURCE_ONLY"

            runtime_status = (
                "deferred"
                if classification == "DEFERRED"
                else "source_only"
                if classification == "SOURCE_ONLY"
                else "r2_or_adapter"
            )
            existing = command_summary.get(command.name)
            if existing is None:
                command_summary[command.name] = {
                    "conversion_classification": classification,
                    "runtime_status": runtime_status,
                    "occurrences": 1,
                }
            else:
                if existing["conversion_classification"] != classification:
                    raise R3ScriptCatalogError(
                        f"command {command.name} has inconsistent classifications"
                    )
                existing["occurrences"] += 1

            if not is_non_script_data:
                for target in _script_targets(command):
                    if re.fullmatch(r"(?:0x[0-9A-Fa-f]+|\d+)", target):
                        continue
                    if target not in labels:
                        raise R3ScriptCatalogError(
                            f"{command.path}:{command.line}: {command.label}: "
                            f"unresolved script target {target}"
                        )

    event_refs: list[dict[str, Any]] = []
    for map_item in source_catalog["maps"]:
        map_name = map_item["name"]
        map_path = source_root / "data/maps" / map_name / "map.json"
        map_doc = _load_json(map_path)

        shared_events_map = map_doc.get("shared_events_map")
        if shared_events_map is not None:
            if shared_events_map not in maps_by_name:
                raise R3ScriptCatalogError(
                    f"{map_name}: unknown shared events owner {shared_events_map!r}"
                )
            event_path = (
                source_root / "data/maps" / shared_events_map / "map.json"
            )
            event_doc = _load_json(event_path)
        else:
            event_path = map_path
            event_doc = map_doc

        for event_key in ("object_events", "coord_events", "bg_events"):
            events = event_doc.get(event_key) or []
            if not isinstance(events, list):
                raise R3ScriptCatalogError(
                    f"{event_path}: {event_key} must be an array or null"
                )
            for index, event in enumerate(events):
                if not isinstance(event, dict):
                    raise R3ScriptCatalogError(
                        f"{event_path}: {event_key}[{index}] must be an object"
                    )
                script_id = _normalize_script_ref(event.get("script"))
                if script_id is None:
                    continue
                if script_id not in labels:
                    raise R3ScriptCatalogError(
                        f"{map_name}: unresolved event script {script_id} "
                        f"in {event_key}[{index}]"
                    )
                source_path, source_line, _ = labels[script_id]
                event_refs.append(
                    {
                        "map": map_name,
                        "event_source_map": shared_events_map or map_name,
                        "event_kind": event_key,
                        "event_index": index,
                        "script_id": script_id,
                        "script_source": str(source_path).replace("\\", "/"),
                        "script_line": source_line,
                    }
                )

    source_files = sorted(
        {
            str(path).replace("\\", "/")
            for path, _line, _commands in labels.values()
        }
    )

    return {
        "schema_version": SCHEMA_VERSION,
        "map_count": source_catalog["map_count"],
        "map_script_file_count": source_catalog["map_script_file_count"],
        "script_source_file_count": len(source_files),
        "script_label_count": len(label_entries),
        "maps": map_entries,
        "labels": label_entries,
        "commands": {
            name: command_summary[name] for name in sorted(command_summary)
        },
        "event_script_references": sorted(
            event_refs,
            key=lambda item: (
                item["map"],
                item["event_kind"],
                item["event_index"],
                item["script_id"],
            ),
        ),
        "source_files": source_files,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    catalog = build_script_catalog(args.source_root.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(catalog, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
