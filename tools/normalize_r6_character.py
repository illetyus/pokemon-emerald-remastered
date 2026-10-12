"""Validate actual local R6 normalization outputs without claiming engine import."""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re

class NormalizationError(ValueError):
    pass

def validate_receipt(receipt: dict, output_root: Path) -> dict:
    def require(condition: bool, message: str) -> None:
        if not condition:
            raise NormalizationError(message)
    require(isinstance(receipt, dict) and type(receipt.get("schema_version")) is int and receipt.get("schema_version") == 1, "Unsupported receipt")
    require(isinstance(receipt.get("model_id"), str) and bool(re.fullmatch(r"[a-z0-9][a-z0-9._-]*", receipt["model_id"])), "Invalid model identity")
    for key in ("source_archive_sha256", "source_member_sha256"):
        require(isinstance(receipt.get(key), str) and bool(re.fullmatch(r"[0-9a-f]{64}", receipt[key])), "Invalid source hash")
    require(receipt.get("target_unit") == "centimeter"
            and receipt.get("target_up_axis") == "Z"
            and receipt.get("target_forward_axis") == "X", "Unnormalized coordinate contract")
    require(receipt.get("root_motion_enabled") is False, "Root motion cannot own gameplay")
    require(receipt.get("animation_origin") in ("project_authored", "verified_exact_source"), "Unknown clip provenance")
    for field, low, high in (("material_slots", 1, 4), ("max_weights_per_vertex", 1, 4)):
        value = receipt.get(field)
        require(type(value) is int and low <= value <= high, "Exceeded material/weight contract")
    for field in ("height_cm", "foot_min_cm"):
        value = receipt.get(field)
        require(type(value) in (int, float) and math.isfinite(value), "Invalid measured geometry")
    require(0 < receipt["height_cm"] <= 400 and abs(receipt["foot_min_cm"]) <= 0.5, "Invalid height or ground origin")
    outputs = receipt.get("outputs")
    require(isinstance(outputs, dict) and bool(outputs), "Missing actual normalized outputs")
    root = output_root.resolve()
    for name, expected in outputs.items():
        require(isinstance(name, str) and bool(name) and ":" not in name and "\\" not in name, "Invalid output path")
        relative = Path(name)
        path = (root / relative).resolve()
        require(not relative.is_absolute() and path.is_relative_to(root) and path.is_file(), "Missing/escaped output")
        require(isinstance(expected, str) and bool(re.fullmatch(r"[0-9a-f]{64}", expected)), "Invalid output hash")
        with path.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        require(path.stat().st_size > 0 and digest == expected, "Output hash drift")
    require(isinstance(receipt.get("mesh"), str) and receipt["mesh"] in outputs, "Missing actual mesh export")
    clips = receipt.get("clips", {})
    require(isinstance(clips, dict), "Invalid clip collection")
    for semantic in ("idle", "walk"):
        clip = clips.get(semantic)
        require(isinstance(clip, dict) and isinstance(clip.get("file"), str) and clip["file"] in outputs, "Missing actual clip")
        for key in ("frame_count", "sample_rate", "changed_tracks"):
            require(type(clip.get(key)) is int and clip[key] > 0, "Invalid clip evidence")
        require(clip["frame_count"] > 1 and clip.get("loop_closed") is True, "Nonlooping or reference-only clip")
        displacement = clip.get("root_xy_displacement_cm")
        require(type(displacement) in (int, float) and math.isfinite(displacement)
                and abs(displacement) <= 1e-5, "Clip changes authoritative root position")
    require(receipt.get("unreal_import_validated") is False
            and receipt.get("packaged_runtime_validated") is False, "Normalization cannot certify Unreal/runtime")
    return receipt

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("receipt", type=Path)
    parser.add_argument("--root", required=True, type=Path)
    args = parser.parse_args()
    validate_receipt(json.loads(args.receipt.read_text(encoding="utf-8")), args.root)
    print("R6 NORMALIZED_OUTPUTS_VERIFIED; Unreal import/runtime are separate gates")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
