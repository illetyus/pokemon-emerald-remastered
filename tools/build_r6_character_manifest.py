#!/usr/bin/env python3
"""Build the deterministic R6 character/NPC presentation manifest."""

from __future__ import annotations

import hashlib
import json
import math
import re
import sys
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
OVERRIDES_MANIFEST_PATH = "data/r6/character_presentation_overrides.json"
ALLOWED_KINDS = {"human", "pokemon_overworld", "special_object"}
LOGICAL_ID_RE = re.compile(r"^[a-z0-9][a-z0-9_.-]*$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")

PRE_239_POKEMON = {
    "RAYQUAZA_STILL",
    "VIGOROTH_CARRYING_BOX",
    "VIGOROTH_FACING_AWAY",
    "ZIGZAGOON_1",
    "LATIAS",
    "LATIOS",
    "KYOGRE_FRONT",
    "GROUDON_FRONT",
    "REGIROCK",
    "REGICE",
    "REGISTEEL",
    "SKITTY",
    "KECLEON",
    "KYOGRE_ASLEEP",
    "GROUDON_ASLEEP",
    "RAYQUAZA",
    "ZIGZAGOON_2",
    "PIKACHU",
    "AZUMARILL",
    "WINGULL",
    "AZURILL",
    "POOCHYENA",
    "KYOGRE_SIDE",
    "GROUDON_SIDE",
    "KIRLIA",
    "DUSCLOPS",
    "SUDOWOODO",
    "MEW",
    "DEOXYS",
    "LUGIA",
    "HOOH",
}

SPECIAL_OBJECT_NAMES = {
    "ITEM_BALL",
    "BERRY_TREE",
    "BERRY_TREE_EARLY_STAGES",
    "BERRY_TREE_LATE_STAGES",
    "CUTTABLE_TREE",
    "BREAKABLE_ROCK",
    "PUSHABLE_BOULDER",
    "MR_BRINEYS_BOAT",
    "TRUCK",
    "BIRCHS_BAG",
    "MOVING_BOX",
    "CABLE_CAR",
    "SS_TIDAL",
    "SUBMARINE_SHADOW",
    "FOSSIL",
    "KECLEON_BRIDGE_SHADOW",
    "TRICK_HOUSE_STATUE",
    "DEOXYS_TRIANGLE",
    "EXPANDING_SPARKLE",
}

FALLBACK_BY_KIND = {
    "human": "fallback.human",
    "pokemon_overworld": "fallback.pokemon_overworld",
    "special_object": "fallback.special_object",
}

SKELETON_BY_KIND = {
    "human": "special_human",
    "pokemon_overworld": "none",
    "special_object": "none",
}

LOD_BY_KIND = {
    "human": "mobile_character",
    "pokemon_overworld": "mobile_character",
    "special_object": "mobile_object",
}

OVERRIDABLE_FIELDS = {
    "presentation_id",
    "presentation_kind",
    "source_family",
    "skeleton_family",
    "model_id",
    "material_ids",
    "animation_set_id",
    "scale",
    "ground_offset_cm",
    "yaw_offset_deg",
    "lod_profile",
    "fallback_id",
    "provenance_id",
    "source_sha256",
    "normalized_sha256",
}


class ManifestError(ValueError):
    pass


def strict_json_loads(text: str) -> Any:
    def object_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result = {}
        for key, value in pairs:
            if key in result:
                raise ManifestError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def constant(value: str) -> None:
        raise ManifestError(f"non-finite JSON constant: {value}")

    return json.loads(text, object_pairs_hook=object_pairs, parse_constant=constant)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def canonical_sha256(value: Any) -> str:
    payload = json.dumps(
        value,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
        allow_nan=False,
    ).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def pokemon_expansion_enabled(config_text: str) -> bool:
    return re.search(
        r"^\s*#\s*define\s+POKEMON_EXPANSION\b",
        config_text,
        re.MULTILINE,
    ) is not None


def select_pokemon_expansion_branch(text: str, enabled: bool) -> str:
    start_marker = "#ifndef POKEMON_EXPANSION"
    start = text.find(start_marker)
    if start < 0:
        return text

    else_pos = text.find("#else", start + len(start_marker))
    if else_pos < 0:
        raise ManifestError("event_objects.h POKEMON_EXPANSION block has no #else")

    end_pos = text.find("#endif", else_pos + len("#else"))
    if end_pos < 0:
        raise ManifestError("event_objects.h POKEMON_EXPANSION block has no #endif")

    chosen = (
        text[else_pos + len("#else"):end_pos]
        if enabled
        else text[start + len(start_marker):else_pos]
    )
    return text[:start] + chosen + text[end_pos + len("#endif"):]


def parse_graphics_definitions(vendor: Path) -> tuple[list[tuple[str, int]], bool]:
    config_path = vendor / "include/config.h"
    event_path = vendor / "include/constants/event_objects.h"
    config_text = config_path.read_text(encoding="utf-8")
    event_text = event_path.read_text(encoding="utf-8")
    expansion = pokemon_expansion_enabled(config_text)
    active = select_pokemon_expansion_branch(event_text, expansion)

    definitions = [
        (match.group(1), int(match.group(2), 10))
        for match in re.finditer(
            r"^#define\s+(OBJ_EVENT_GFX_[A-Z0-9_]+)\s+(\d+)\s*$",
            active,
            re.MULTILINE,
        )
    ]
    if not definitions:
        raise ManifestError("no OBJ_EVENT_GFX identities found")

    names = [name for name, _ in definitions]
    ids = [graphics_id for _, graphics_id in definitions]
    if len(names) != len(set(names)):
        raise ManifestError("duplicate active OBJ_EVENT_GFX symbolic identity")
    if len(ids) != len(set(ids)):
        raise ManifestError("duplicate active OBJ_EVENT_GFX numeric identity")

    definitions.sort(key=lambda item: (item[1], item[0]))
    ordered_ids = [graphics_id for _, graphics_id in definitions]
    expected = list(range(ordered_ids[0], ordered_ids[-1] + 1))
    if ordered_ids != expected:
        raise ManifestError("active OBJ_EVENT_GFX ids are not dense")
    return definitions, expansion


def graphics_suffix(graphics_name: str) -> str:
    prefix = "OBJ_EVENT_GFX_"
    if not graphics_name.startswith(prefix):
        raise ManifestError(f"invalid graphics name: {graphics_name}")
    return graphics_name[len(prefix):]


def is_special_object_name(suffix: str) -> bool:
    return (
        suffix in SPECIAL_OBJECT_NAMES
        or suffix.endswith("_DOLL")
        or suffix.endswith("_CUSHION")
    )


def classify_identity(graphics_name: str, graphics_id: int) -> str:
    suffix = graphics_suffix(graphics_name)
    if suffix in PRE_239_POKEMON:
        return "pokemon_overworld"
    if 239 <= graphics_id <= 489 or 515 <= graphics_id <= 679:
        return "pokemon_overworld"
    if 490 <= graphics_id <= 514 or graphics_id == 680:
        return "special_object"
    if is_special_object_name(suffix):
        return "special_object"
    return "human"


def default_record(graphics_name: str, graphics_id: int) -> dict[str, Any]:
    kind = classify_identity(graphics_name, graphics_id)
    suffix = graphics_suffix(graphics_name).lower()
    fallback = FALLBACK_BY_KIND[kind]
    return {
        "schema_version": SCHEMA_VERSION,
        "graphics_id": graphics_id,
        "graphics_name": graphics_name,
        "presentation_id": f"{kind}.{suffix}",
        "presentation_kind": kind,
        "source_family": "project_placeholder",
        "skeleton_family": SKELETON_BY_KIND[kind],
        "model_id": fallback,
        "material_ids": [],
        "animation_set_id": fallback,
        "scale": 1.0,
        "ground_offset_cm": 0.0,
        "yaw_offset_deg": 0.0,
        "lod_profile": LOD_BY_KIND[kind],
        "fallback_id": fallback,
        "provenance_id": "project.placeholder.r6",
        "source_sha256": [],
        "normalized_sha256": [],
    }


def load_overrides(path: Path) -> dict[str, dict[str, Any]]:
    data = strict_json_loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or type(data.get("schema_version")) is not int or data["schema_version"] != SCHEMA_VERSION:
        raise ManifestError("unsupported R6 override schema")
    overrides = data.get("overrides")
    if not isinstance(overrides, dict):
        raise ManifestError("R6 overrides must be an object")
    for name, fields in overrides.items():
        if not isinstance(name, str) or not isinstance(fields, dict):
            raise ManifestError("R6 override entries must be objects keyed by graphics name")
        unknown = set(fields) - OVERRIDABLE_FIELDS
        if unknown:
            raise ManifestError(
                f"{name}: unknown override fields: {sorted(unknown)}"
            )
    return overrides


def validate_logical_id(value: Any, field: str, graphics_name: str) -> None:
    if not isinstance(value, str) or not LOGICAL_ID_RE.fullmatch(value):
        raise ManifestError(f"{graphics_name}: invalid {field}: {value!r}")


def validate_hashes(values: Any, field: str, graphics_name: str) -> None:
    if not isinstance(values, list):
        raise ManifestError(f"{graphics_name}: {field} must be an array")
    for value in values:
        if not isinstance(value, str) or not SHA256_RE.fullmatch(value):
            raise ManifestError(f"{graphics_name}: invalid {field} hash")


def validate_record(record: dict[str, Any]) -> None:
    name = record["graphics_name"]
    if type(record["schema_version"]) is not int or record["schema_version"] != SCHEMA_VERSION:
        raise ManifestError(f"{name}: wrong record schema")
    if type(record["graphics_id"]) is not int or not 0 <= record["graphics_id"] <= 65535:
        raise ManifestError(f"{name}: invalid graphics_id")
    if not isinstance(name, str) or not re.fullmatch(r"OBJ_EVENT_GFX_[A-Z0-9_]+", name):
        raise ManifestError("invalid graphics_name")
    if record["presentation_kind"] not in ALLOWED_KINDS:
        raise ManifestError(f"{name}: invalid presentation_kind")
    for field in (
        "presentation_id",
        "source_family",
        "skeleton_family",
        "model_id",
        "animation_set_id",
        "lod_profile",
        "fallback_id",
        "provenance_id",
    ):
        validate_logical_id(record[field], field, name)
    materials = record["material_ids"]
    if not isinstance(materials, list):
        raise ManifestError(f"{name}: material_ids must be an array")
    for material in materials:
        validate_logical_id(material, "material_ids", name)
    for field in ("scale", "ground_offset_cm", "yaw_offset_deg"):
        value = record[field]
        if not isinstance(value, (int, float)) or isinstance(value, bool):
            raise ManifestError(f"{name}: {field} must be numeric")
        if not math.isfinite(value) or abs(value) > 1.0e6:
            raise ManifestError(f"{name}: {field} must be finite and bounded")
    if float(record["scale"]) <= 0.0:
        raise ManifestError(f"{name}: scale must be positive")
    validate_hashes(record["source_sha256"], "source_sha256", name)
    validate_hashes(record["normalized_sha256"], "normalized_sha256", name)
    if record["source_family"] != "project_placeholder" and (
        not record["source_sha256"] or record["provenance_id"] == "project.placeholder.r6"
    ):
        raise ManifestError(f"{name}: non-placeholder source needs hash and provenance")
    if record["normalized_sha256"] and not record["source_sha256"]:
        raise ManifestError(f"{name}: normalized output needs source hash")


def build_manifest(vendor: Path, overrides_path: Path) -> dict[str, Any]:
    definitions, expansion = parse_graphics_definitions(vendor)
    overrides = load_overrides(overrides_path)
    source_names = {name for name, _ in definitions}
    unknown_overrides = sorted(set(overrides) - source_names)
    if unknown_overrides:
        raise ManifestError(
            f"overrides reference unknown graphics identities: {unknown_overrides}"
        )

    entries: list[dict[str, Any]] = []
    seen_presentation_ids: set[str] = set()
    for graphics_name, graphics_id in definitions:
        record = default_record(graphics_name, graphics_id)
        record.update(overrides.get(graphics_name, {}))
        validate_record(record)
        presentation_id = record["presentation_id"]
        if presentation_id in seen_presentation_ids:
            raise ManifestError(f"duplicate presentation_id: {presentation_id}")
        seen_presentation_ids.add(presentation_id)
        entries.append(record)

    kind_counts = {
        kind: sum(entry["presentation_kind"] == kind for entry in entries)
        for kind in sorted(ALLOWED_KINDS)
    }
    fallback_counts = {
        fallback: sum(entry["fallback_id"] == fallback for entry in entries)
        for fallback in sorted(set(FALLBACK_BY_KIND.values()))
    }
    payload_for_hash = {
        "entries": entries,
        "fallbacks": FALLBACK_BY_KIND,
    }
    return {
        "schema_version": SCHEMA_VERSION,
        "source_configuration": {
            "pokemon_expansion": expansion,
        },
        "source_files": [
            {
                "path": "include/config.h",
                "sha256": sha256_file(vendor / "include/config.h"),
            },
            {
                "path": "include/constants/event_objects.h",
                "sha256": sha256_file(
                    vendor / "include/constants/event_objects.h"
                ),
            },
            {
                "path": OVERRIDES_MANIFEST_PATH,
                "sha256": sha256_file(overrides_path),
            },
        ],
        "identity_count": len(entries),
        "min_graphics_id": entries[0]["graphics_id"],
        "max_graphics_id": entries[-1]["graphics_id"],
        "kind_counts": kind_counts,
        "fallback_counts": fallback_counts,
        "fallbacks": dict(sorted(FALLBACK_BY_KIND.items())),
        "content_sha256": canonical_sha256(payload_for_hash),
        "entries": entries,
    }


def resolve_entry(manifest: dict[str, Any], graphics_id: int) -> dict[str, Any] | None:
    if not isinstance(graphics_id, int) or isinstance(graphics_id, bool) or graphics_id < 0:
        return None
    entries = manifest.get("entries", [])
    if (
        isinstance(graphics_id, int)
        and not isinstance(graphics_id, bool)
        and 0 <= graphics_id < len(entries)
        and entries[graphics_id].get("graphics_id") == graphics_id
    ):
        return entries[graphics_id]
    for entry in entries:
        if entry.get("graphics_id") == graphics_id:
            return entry
    return None


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: build_r6_character_manifest.py "
            "<vendor> <overrides.json> <manifest.json>",
            file=sys.stderr,
        )
        return 2

    vendor = Path(sys.argv[1])
    overrides_path = Path(sys.argv[2])
    output_path = Path(sys.argv[3])
    manifest = build_manifest(vendor, overrides_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True, allow_nan=False) + "\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ManifestError, OSError, json.JSONDecodeError) as exc:
        print(f"R6 character manifest error: {exc}", file=sys.stderr)
        raise SystemExit(1)
