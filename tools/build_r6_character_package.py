#!/usr/bin/env python3
"""Stage and verify R6 metadata independently of any asset vault or network."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

try:
    from tools.build_r6_character_manifest import (
        FALLBACK_BY_KIND, ManifestError, build_manifest, canonical_sha256, strict_json_loads, validate_record,
    )
except ModuleNotFoundError:
    from build_r6_character_manifest import (
        FALLBACK_BY_KIND, ManifestError, build_manifest, canonical_sha256, strict_json_loads, validate_record,
    )


def json_bytes(value: object) -> bytes:
    return (json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + "\n").encode()


def build_coverage(manifest: dict, data_root: Path) -> dict:
    plan = strict_json_loads((data_root / "npc_presentation_plan.json").read_text())
    player = strict_json_loads((data_root / "player_presentation_states.json").read_text())
    decisions = {r["graphics_name"]: r for r in plan["records"]}
    players = {r["graphics_name"]: r for r in player["records"]}
    human_names = {r["graphics_name"] for r in manifest["entries"] if r["presentation_kind"] == "human"}
    if set(decisions) & set(players) or set(decisions) | set(players) != human_names:
        raise ManifestError("human coverage plan does not match active source identities")
    records = []
    for entry in manifest["entries"]:
        name = entry["graphics_name"]
        decision = decisions.get(name)
        if decision and (decision["graphics_id"] != entry["graphics_id"]
                         or decision["model_id"] != entry["model_id"]):
            raise ManifestError(f"{name}: plan/manifest identity or model mismatch")
        if entry["presentation_kind"] == "human":
            selection = "player_base_source" if name in players else decision["decision"]
        else:
            selection = "r14_owned_placeholder" if entry["presentation_kind"] == "pokemon_overworld" else "explicit_object_placeholder"
        records.append({"graphics_id":entry["graphics_id"], "graphics_name":name,
                        "presentation_id":entry["presentation_id"], "model_id":entry["model_id"],
                        "presentation_kind":entry["presentation_kind"], "selection":selection,
                        "fallback_id":entry["fallback_id"],
                        "runtime_default":"visible_placeholder",
                        "normalized_output_available":bool(entry["normalized_sha256"]),
                        "animation_clip_available":False})
    return {"schema_version":1, "identity_count":len(records), "kind_counts":manifest["kind_counts"],
            "human_source_selected":sum(r["presentation_kind"]=="human" and not r["model_id"].startswith("fallback.") for r in records),
            "human_explicit_source_gaps_or_deferred":sum(r["presentation_kind"]=="human" and r["model_id"].startswith("fallback.") for r in records),
            "missing_local_assets_do_not_block_gameplay":True,
            "integrity_boundary":"build_time_verify_before_cook_runtime_checks_structure_only",
            "unreal_runtime_status":"untested", "android_status":"untested", "records":records}


def verify_package(output: Path) -> dict:
    raw = (output / "manifest.json").read_bytes()
    expected = (output / "manifest.sha256").read_text().strip()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise ManifestError("staged manifest byte SHA-256 mismatch")
    manifest = strict_json_loads(raw.decode("utf-8"))
    entries = manifest["entries"]
    if type(manifest["schema_version"]) is not int or manifest["schema_version"] != 1:
        raise ManifestError("invalid staged schema")
    if type(manifest["identity_count"]) is not int or manifest["identity_count"] != len(entries):
        raise ManifestError("invalid staged count")
    if not entries or len(entries) > 65535 or any(type(manifest[f]) is not int for f in ("min_graphics_id", "max_graphics_id")) or manifest["min_graphics_id"] != 0 or manifest["max_graphics_id"] != len(entries)-1:
        raise ManifestError("invalid staged identity bounds")
    for index, entry in enumerate(entries):
        if type(entry["graphics_id"]) is not int or entry["graphics_id"] != index:
            raise ManifestError("invalid staged numeric identity")
        validate_record(entry)
    if manifest["fallbacks"] != FALLBACK_BY_KIND:
        raise ManifestError("invalid staged fallback contract")
    kind_counts = {kind:sum(e["presentation_kind"] == kind for e in entries) for kind in FALLBACK_BY_KIND}
    fallback_counts = {fallback:sum(e["fallback_id"] == fallback for e in entries) for fallback in FALLBACK_BY_KIND.values()}
    if manifest["kind_counts"] != kind_counts or manifest["fallback_counts"] != fallback_counts:
        raise ManifestError("staged aggregate counts mismatch")
    for field in ("graphics_name", "presentation_id"):
        if len({e[field] for e in entries}) != len(entries):
            raise ManifestError(f"duplicate staged {field}")
    if canonical_sha256({"entries":entries,"fallbacks":manifest["fallbacks"]}) != manifest["content_sha256"]:
        raise ManifestError("staged manifest canonical content SHA-256 mismatch")
    audit = strict_json_loads((output / "coverage.json").read_text())
    if audit["identity_count"] != len(entries) or len(audit["records"]) != len(entries):
        raise ManifestError("staged coverage count mismatch")
    selected = sum(e["presentation_kind"] == "human" and not e["model_id"].startswith("fallback.") for e in entries)
    if audit["kind_counts"] != kind_counts or audit["human_source_selected"] != selected or audit["human_explicit_source_gaps_or_deferred"] != kind_counts["human"]-selected:
        raise ManifestError("staged coverage summary mismatch")
    for entry, record in zip(entries, audit["records"]):
        for field in ("graphics_id", "graphics_name", "presentation_id", "model_id", "fallback_id", "presentation_kind"):
            if record[field] != entry[field]:
                raise ManifestError(f"staged coverage {field} mismatch")
    return audit


def build_package(root: Path, output: Path) -> dict:
    manifest = build_manifest(root / "vendor/vanillaplus", root / "data/r6/character_presentation_overrides.json")
    audit = build_coverage(manifest, root / "data/r6")
    output.mkdir(parents=True, exist_ok=True)
    raw = json_bytes(manifest)
    (output / "manifest.json").write_bytes(raw)
    (output / "manifest.sha256").write_text(hashlib.sha256(raw).hexdigest() + "\n")
    (output / "coverage.json").write_bytes(json_bytes(audit))
    verify_package(output)
    return audit


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    audit = verify_package(args.output) if args.verify else build_package(args.repo, args.output)
    print(f"R6 package verified: {audit['identity_count']} identities; "
          f"{audit['human_source_selected']} human source selections; "
          f"{audit['human_explicit_source_gaps_or_deferred']} explicit human source gaps/deferred.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, KeyError, TypeError) as exc:
        print(f"R6 package error: {exc}", file=sys.stderr)
        raise SystemExit(1)
