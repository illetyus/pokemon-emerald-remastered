#!/usr/bin/env python3
"""Deterministic, read-only source inspection. Never exports/imports a model."""
from __future__ import annotations

import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import sys
import xml.etree.ElementTree as ET
import zipfile

NS = {"c": "http://www.collada.org/2005/11/COLLADASchema"}
MAX_EXPANDED_BYTES = 64 * 1024 * 1024


def inspect_dae(raw: bytes, filename: str) -> dict:
    if b"<!DOCTYPE" in raw.upper() or b"<!ENTITY" in raw.upper():
        raise ValueError("DTD/entity input is not allowed")
    root = ET.fromstring(raw)
    if root.tag != "{" + NS["c"] + "}COLLADA":
        raise ValueError("unsupported COLLADA root/namespace")
    joints = []
    for skin in root.findall(".//c:skin", NS):
        sources = {s.get("id"): s for s in skin.findall("c:source", NS)}
        for item in skin.findall("c:joints/c:input", NS):
            if item.get("semantic") == "JOINT":
                source = sources.get(item.get("source", "").lstrip("#"))
                if source is not None:
                    array = source.find("c:Name_array", NS)
                    if array is None:
                        array = source.find("c:IDREF_array", NS)
                    if array is not None:
                        joints.extend((array.text or "").split())
    # Hash exact rest-matrix and hierarchy XML; equality is a review aid only.
    rig_nodes = root.findall(".//c:visual_scene", NS)
    rig_bytes = b"".join(ET.tostring(node) for node in rig_nodes)
    unit = root.find("c:asset/c:unit", NS)
    up = root.find("c:asset/c:up_axis", NS)
    return {
        "source_submodel_file": Path(filename).name,
        "source_submodel_sha256": hashlib.sha256(raw).hexdigest(),
        "skin_controller_count": len(root.findall(".//c:skin", NS)),
        "joint_count": len(set(joints)),
        "joint_names_sha256": hashlib.sha256("\n".join(sorted(set(joints))).encode()).hexdigest(),
        "source_scene_sha256": hashlib.sha256(rig_bytes).hexdigest(),
        "material_count": len(root.findall(".//c:library_materials/c:material", NS)),
        "image_count": len(root.findall(".//c:library_images/c:image", NS)),
        "embedded_animation_count": len(root.findall(".//c:library_animations/c:animation", NS)),
        "declared_unit_meter": unit.get("meter") if unit is not None else None,
        "declared_up_axis": up.text.strip() if up is not None and up.text else None,
        "family_compatible_reuse_allowed": False,
        "normalization_status": "pending",
        "unreal_import_status": "untested",
    }


def inspect_archive(raw: bytes, expected_sha256: str | None = None) -> dict:
    digest = hashlib.sha256(raw).hexdigest()
    if expected_sha256 is not None and digest != expected_sha256:
        raise ValueError("source archive SHA-256 mismatch")
    with zipfile.ZipFile(io.BytesIO(raw)) as archive:
        entries = archive.infolist()
        if len(entries) > 4096 or sum(i.file_size for i in entries) > MAX_EXPANDED_BYTES:
            raise ValueError("archive expansion budget exceeded")
        if len({i.filename for i in entries}) != len(entries):
            raise ValueError("duplicate archive member")
        for item in entries:
            path = PurePosixPath(item.filename.replace("\\", "/"))
            if path.is_absolute() or ".." in path.parts or ":" in item.filename:
                raise ValueError("unsafe archive member")
            if item.flag_bits & 1 or (item.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError("encrypted/symlink archive member")
        if archive.testzip() is not None:
            raise ValueError("archive CRC failure")
        models = [inspect_dae(archive.read(i), i.filename)
                  for i in sorted(entries, key=lambda v: v.filename)
                  if i.filename.lower().endswith(".dae")]
    return {"source_archive_sha256": digest, "zip_integrity_verified": True,
            "model_count": len(models), "models": models}


def main() -> int:
    if len(sys.argv) not in (3, 4):
        print("usage: inspect_r6_character_archive.py <source.zip> <report.json> [expected-sha256]")
        return 2
    report = inspect_archive(Path(sys.argv[1]).read_bytes(), sys.argv[3] if len(sys.argv) == 4 else None)
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
