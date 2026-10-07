#!/usr/bin/env python3
"""Pre-import validation of private, triangulated centimetre OBJ LODs.

No Unreal import/cook claim: the local AssetSet remains disabled until its real
engine import is checked. Reports contain hashes/counts, never payloads/paths.
"""
from __future__ import annotations
import argparse
import math
from pathlib import Path
import re
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from tools.build_r7_environment_package import digest, encoded, strict_json


def integer(value: object, lower: int, upper: int) -> int:
    if type(value) is not int or not lower <= value <= upper:
        raise ValueError("invalid bounded integer")
    return value


def obj_geometry(path: Path) -> tuple[int, int]:
    vertices = 0
    triangles = 0
    materials: set[str] = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        fields = line.split()
        if not fields or fields[0].startswith("#"):
            continue
        if fields[0] == "v":
            if len(fields) != 4:
                raise ValueError("OBJ must have three-coordinate vertices")
            values = [float(field) for field in fields[1:]]
            if not all(math.isfinite(value) and abs(value) <= 1000 for value in values):
                raise ValueError("non-finite or oversized centimetre geometry")
            vertices += 1
        elif fields[0] == "f":
            if len(fields) != 4:
                raise ValueError("OBJ must be triangulated")
            indexes = [int(field.split("/")[0]) for field in fields[1:]]
            if len(set(indexes)) != 3 or any(index == 0 or index > vertices or index < -vertices for index in indexes):
                raise ValueError("OBJ face references invalid vertices")
            triangles += 1
        elif fields[0] == "usemtl":
            if len(fields) != 2:
                raise ValueError("invalid material name")
            materials.add(fields[1])
    if triangles == 0 or vertices == 0:
        raise ValueError("empty geometry")
    return triangles, max(1, len(materials))


def png_dimension(path: Path) -> int:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("texture must be PNG")
    offset = 8
    dimension = 0
    seen_idat = False
    ended = False
    while offset + 12 <= len(data):
        size = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        end = offset + 8 + size
        if end + 4 > len(data):
            raise ValueError("truncated PNG")
        if zlib.crc32(data[offset + 4:end]) & 0xFFFFFFFF != struct.unpack_from(">I", data, end)[0]:
            raise ValueError("PNG CRC mismatch")
        if offset == 8:
            if kind != b"IHDR" or size != 13:
                raise ValueError("PNG header missing")
            width, height = struct.unpack_from(">II", data, offset + 8)
            dimension = max(integer(width, 1, 1024), integer(height, 1, 1024))
        seen_idat = seen_idat or kind == b"IDAT"
        offset = end + 4
        if kind == b"IEND":
            ended = True
            break
    if not dimension or not seen_idat or not ended or offset != len(data):
        raise ValueError("incomplete PNG")
    return dimension


def checked_file(root: Path, record: dict) -> Path:
    path = (root / record["path"]).resolve()
    expected = record["sha256"]
    if not isinstance(expected, str) or not re.fullmatch("[0-9a-f]{64}", expected):
        raise ValueError("invalid source/output hash")
    if digest(path.read_bytes()) != expected:
        raise ValueError("source/output hash mismatch")
    return path


def validate_assets(binding_file: Path, package_manifest: Path) -> dict:
    document = strict_json(binding_file)
    if document.get("schema_version") != 1 or type(document.get("schema_version")) is not int:
        raise ValueError("unsupported local binding schema")
    manifest = strict_json(package_manifest)
    known = {item["identity"] for item in manifest["identities"] if item["model_status"] != "descriptor_missing"}
    root = binding_file.parent
    report = []
    seen: set[str] = set()
    for record in document["models"]:
        identity = record["identity"]
        if identity not in known or identity in seen:
            raise ValueError("unknown or duplicate exact identity")
        seen.add(identity)
        source = checked_file(root, record["source"])
        provenance_path = checked_file(root, record["provenance"])
        provenance = strict_json(provenance_path)
        if provenance.get("identity") != identity or provenance.get("source_sha256") != digest(source.read_bytes()):
            raise ValueError("provenance identity/hash mismatch")
        if not isinstance(provenance.get("source_url"), str) or not provenance["source_url"].startswith("https://"):
            raise ValueError("provenance source URL missing")
        if record.get("units") != "centimetres" or record.get("pivot") != "tile-ground":
            raise ValueError("normalization units/pivot not declared")
        lods = record["lods"]
        if len(lods) != 3:
            raise ValueError("three LODs required")
        counts, material_counts = zip(*(obj_geometry(checked_file(root, lod)) for lod in lods))
        if any(count > maximum for count, maximum in zip(counts, (6000, 3000, 1500))) or list(counts) != sorted(counts, reverse=True):
            raise ValueError("LOD triangle budget exceeded")
        if max(material_counts) > 2:
            raise ValueError("material budget exceeded")
        textures = record.get("textures", [])
        dimensions = [png_dimension(checked_file(root, texture)) for texture in textures]
        cull = record["cull_distance"]
        if type(cull) not in (int, float) or not math.isfinite(cull) or not 1000 <= cull <= 4000:
            raise ValueError("cull budget invalid")
        report.append({"identity": identity, "source_sha256": record["source"]["sha256"],
            "provenance_sha256": record["provenance"]["sha256"],
            "normalized_sha256": digest(encoded({"lod_sha256": [item["sha256"] for item in lods],
                "texture_sha256": [item["sha256"] for item in textures], "units": "centimetres", "pivot": "tile-ground"})),
            "lod_triangles": list(counts), "material_slots": max(material_counts),
            "max_texture_dimension": max(dimensions, default=1), "cull_distance": cull,
            "engine_import_status": "R18_PENDING", "import_validated": False})
    return {"schema_version": 1, "status": "PREIMPORT_VERIFIED", "models": sorted(report, key=lambda item: item["identity"])}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bindings", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = validate_assets(args.bindings, args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(encoded(result))
    print(f"R7 pre-import verified: {len(result['models'])} exact local bindings; engine import pending")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
