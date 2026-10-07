#!/usr/bin/env python3
"""Deterministic, metadata-only R7 identity and full R3 layout audit package."""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))
from tools.build_r5_metatile_descriptors import build_descriptors
from tools.convert_world import build_tileset_attribute_index, convert_layout_document, build_numeric_constant_index

CONTRACT = ROOT / "data/r7/environment_contract.json"
SOURCE_COMMIT = "70db90c9077aed1272e746fc2537d9f12b95a91c"


def strict_json(path: Path) -> Any:
    def pairs(items: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in items:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result
    def invalid(value: str) -> None:
        raise ValueError(f"non-finite JSON scalar: {value}")
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=pairs, parse_constant=invalid)


def encoded(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n").encode()


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def family_hint(labels: list[str], behavior: str) -> str:
    # Explicit hints, not geometry/model selection. Generic wall/collision values
    # cannot identify buildings, trees or fences reliably.
    text = " ".join(labels)
    for terms, family in (
        (("Tree", "Shrub"), "trees"), (("Cave",), "caves"),
        (("Rock",), "rocks"), (("Fence", "Barrier"), "fences"),
        (("Sign",), "signs"), (("Door", "Roof", "House"), "buildings"),
        (("Grass",), "grass"), (("Water", "Pond", "Ocean", "Current"), "water"),
        (("PC", "Shelf", "Television", "Vase", "Trash", "Counter", "Decoration"), "indoor_props"),
    ):
        if any(term in text for term in terms):
            return family
    if behavior in {"MB_TALL_GRASS", "MB_LONG_GRASS", "MB_SHORT_GRASS", "MB_ASHGRASS", "MB_LONG_GRASS_SOUTH_EDGE"}:
        return "grass"
    if any(term in behavior for term in ("WATER", "CURRENT", "SEAWEED", "PUDDLE", "HOT_SPRINGS")):
        return "water"
    if "CAVE" in behavior:
        return "caves"
    return "unclassified"


def resolve(tilesets: dict[str, Any], primary: str, secondary: str | None, global_id: int) -> tuple[str, int]:
    if type(global_id) is not int or not 0 <= global_id <= 1023:
        raise ValueError(f"invalid metatile id {global_id}")
    tileset = primary if global_id < 512 else secondary
    local = global_id if global_id < 512 else global_id - 512
    descriptor = tilesets.get(tileset)
    if descriptor is None or local >= len(descriptor["metatiles"]):
        raise ValueError(f"missing environment identity {tileset}:{local}")
    metatile = descriptor["metatiles"][local]
    if metatile["local_metatile_id"] != local or len(metatile["entries"]) != 8 or len(metatile["render_planes"]) != 2:
        raise ValueError(f"invalid R5 fallback {tileset}:{local}")
    return str(tileset), local


def package_documents(source_root: Path) -> dict[str, Any]:
    contract = strict_json(CONTRACT)
    lock = strict_json(ROOT / "external/sources.lock.json")
    if lock["vanillaplus"]["commit"] != SOURCE_COMMIT:
        raise ValueError("R7 source pin changed")
    descriptor = build_descriptors(source_root)
    tilesets = {item["id"]: item for item in descriptor["tilesets"]}
    behavior_path = source_root / "include/constants/metatile_behaviors.h"
    label_path = source_root / "include/constants/metatile_labels.h"
    behaviors = {int(value, 0): name for name, value in re.findall(
        r"^#define\s+(MB_\w+)\s+(0x[0-9A-Fa-f]+|\d+)\s*(?://.*)?$", behavior_path.read_text(), re.M)}
    labels: dict[str, list[str]] = {}
    for name, number in re.findall(r"^#define\s+METATILE_(\w+)\s+(0x[0-9A-Fa-f]+|\d+)", label_path.read_text(), re.M):
        # Longest tileset prefix handles underscore-containing names safely.
        candidates = [key for key in tilesets if name.startswith(key.removeprefix("gTileset_") + "_")]
        if not candidates:
            continue
        tileset = max(candidates, key=len)
        value = int(number, 0)
        local = value - (512 if tilesets[tileset]["is_secondary"] else 0)
        if 0 <= local < len(tilesets[tileset]["metatiles"]):
            labels.setdefault(f"{tileset}:{local}", []).append("METATILE_" + name)
    identities = []
    for tileset in sorted(tilesets):
        for tile in tilesets[tileset]["metatiles"]:
            identity = f"{tileset}:{tile['local_metatile_id']}"
            named = sorted(labels.get(identity, []))
            behavior = behaviors.get(tile["behavior"], f"MB_UNNAMED_{tile['behavior']:02X}")
            identities.append({"identity": identity, "tileset": tileset,
                "local_metatile_id": tile["local_metatile_id"], "source_labels": named,
                "behavior_id": tile["behavior"], "behavior_name": behavior,
                "family_hint": family_hint(named, behavior), "fallback": "r5_metatile",
                "model_status": "source_missing", "render_planes": tile["render_planes"],
                "source_sha256": digest(encoded(tile))})

    layouts_path = source_root / "data/layouts/layouts.json"
    specs = strict_json(layouts_path)["layouts"]
    attrs = build_tileset_attribute_index(source_root)
    constants = build_numeric_constant_index(source_root)
    inputs = {path.relative_to(source_root).as_posix(): digest(path.read_bytes())
              for path in (behavior_path, label_path, layouts_path,
                  source_root / "include/constants/map_types.h", source_root / "include/constants/weather.h",
                  source_root / "include/constants/layouts.h", source_root / "data/maps/map_groups.json",
                  source_root / "src/data/tilesets/headers.h")}
    layouts = {}
    used: set[str] = set()
    missing_fallbacks: dict[str, dict[str, Any]] = {}

    def resolve_with_fallback(primary: str, secondary: str | None, value: int) -> str:
        tileset = primary if value < 512 else secondary
        local = value if value < 512 else value - 512
        if tileset not in tilesets or local >= len(tilesets[tileset]["metatiles"]):
            identity = f"missing:{tileset}:{local}"
            missing_fallbacks[identity] = {"identity": identity, "tileset": str(tileset),
                "local_metatile_id": local, "source_labels": [], "behavior_id": None,
                "behavior_name": None, "family_hint": "unclassified",
                "fallback": "visible_engine_plane", "model_status": "descriptor_missing",
                "render_planes": ["bottom"], "source_sha256": None}
            return identity
        tileset, local = resolve(tilesets, primary, secondary, value)
        return f"{tileset}:{local}"

    for index, spec in enumerate(specs):
        if spec["id"] in layouts:
            raise ValueError(f"duplicate layout {spec['id']}")
        if constants.get(spec["id"]) != index + 1:
            raise ValueError(f"R3 numeric layout order differs: {spec['id']}")
        layout = convert_layout_document(source_root, {**spec, "_numeric_id": index + 1}, attrs)["layout"]
        primary, secondary = layout["primary_tileset"], layout["secondary_tileset"]
        counts: Counter[str] = Counter()
        chunks: dict[tuple[int, int], set[tuple[str, int]]] = {}
        for tile_index, value in enumerate(layout["metatile_ids_u16"]):
            identity = resolve_with_fallback(primary, secondary, value)
            counts[identity] += 1
            used.add(identity)
            x, y = tile_index % layout["width"], tile_index // layout["width"]
            chunks.setdefault((x // 16, y // 16), set()).add(identity)
        for word in layout["border_active_words_u16"]:
            used.add(resolve_with_fallback(primary, secondary, word & 1023))
        layouts[spec["id"]] = {"id": spec["id"], "width": layout["width"], "height": layout["height"],
            "tile_count": layout["active_word_count"], "identity_counts": dict(sorted(counts.items())),
            "r5_plane_instances": layout["active_word_count"] * 2,
            "max_r5_components_per_chunk": max((len(values) * 2 for values in chunks.values()), default=0),
            "r3_layout_sha256": digest(encoded(layout)), "unresolved_identities": [],
            "descriptor_fallbacks": {key: count for key, count in sorted(counts.items()) if key.startswith("missing:")}}
        for field in ("blockdata_filepath", "border_filepath"):
            path = source_root / spec[field]
            inputs[path.relative_to(source_root).as_posix()] = digest(path.read_bytes())

    maps = []
    for path in sorted((source_root / "data/maps").glob("*/map.json")):
        data = strict_json(path)
        if data["layout"] not in layouts:
            raise ValueError(f"missing layout for {data['name']}")
        maps.append({"name": data["name"], "id": data["id"], "layout": data["layout"],
            "map_type": data["map_type"], "weather": data["weather"],
            "tile_count": layouts[data["layout"]]["tile_count"], "unresolved_identities": [],
            "descriptor_fallbacks": layouts[data["layout"]]["descriptor_fallbacks"],
            "sign_landmarks": [{"x": event["x"], "y": event["y"], "source_type": event["type"]}
                for event in data.get("bg_events", []) if event.get("type") == "sign"]})
        inputs[path.relative_to(source_root).as_posix()] = digest(path.read_bytes())
    by_name = {item["name"]: item for item in maps}
    if len(by_name) != len(maps):
        raise ValueError("duplicate map identity")
    representative = []
    for name in contract["representative_maps"]:
        if name not in by_name:
            raise ValueError(f"missing representative map {name}")
        representative.append(by_name[name])
    for tileset in tilesets.values():
        inputs["tileset:" + tileset["id"]] = digest(encoded(tileset["fingerprints"]))
    identities.extend(missing_fallbacks[key] for key in sorted(missing_fallbacks))
    manifest = {"schema_version": 1, "source_repository": lock["vanillaplus"]["repository"],
        "source_commit": SOURCE_COMMIT, "input_sha256": digest(encoded(inputs)),
        "contract_sha256": digest(encoded(contract)), "tileset_count": len(tilesets),
        "identity_count": len(identities), "used_identity_count": len(used),
        "identities": identities, "verified_3d_models": 0, "fallback_identity_count": len(identities),
        "source_descriptor_identity_count": sum(len(item["metatiles"]) for item in tilesets.values()),
        "descriptor_fallback_identity_count": len(missing_fallbacks),
        "family_counts": dict(sorted(Counter(item["family_hint"] for item in identities).items()))}
    audit = {"schema_version": 1, "status": "PASS", "scope": "source identity/fallback audit, not rendered runtime",
        "map_count": len(maps), "layout_count": len(layouts), "unresolved_identity_count": 0,
        "descriptor_fallback_layouts": [key for key, item in layouts.items() if item["descriptor_fallbacks"]],
        "representative_maps": representative, "maps": maps, "layouts": list(layouts.values()),
        "mobile_runtime_status": "R18_PENDING", "verified_3d_models": 0}
    return {"manifest.json": manifest, "map-audit.json": audit, "contract.json": contract,
            "source-inputs.json": inputs}


def build_package(source_root: Path, output: Path, *, verify: bool = False) -> dict[str, Any]:
    documents = package_documents(source_root)
    payloads = {name: encoded(doc) for name, doc in documents.items()}
    payloads["package-sha256.json"] = encoded({name: digest(data) for name, data in sorted(payloads.items())})
    if verify:
        for name, data in payloads.items():
            if not (output / name).is_file() or (output / name).read_bytes() != data:
                raise ValueError(f"R7 package changed or corrupted: {name}")
    else:
        output.mkdir(parents=True, exist_ok=True)
        for name, data in payloads.items():
            (output / name).write_bytes(data)
    return documents["manifest.json"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    result = build_package(args.source_root, args.output, verify=args.verify)
    print(f"R7 {'verified' if args.verify else 'built'}: {result['identity_count']} identities; all explicit R5 fallback")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
