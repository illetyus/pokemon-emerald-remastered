#!/usr/bin/env python3
"""Build a deterministic R5 tileset render-source manifest from Vanilla+."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
METATILE_BYTES = 16


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    if data[12:16] != b"IHDR":
        raise ValueError(f"{path}: PNG has no leading IHDR")
    width, height = struct.unpack(">II", data[16:24])
    if width <= 0 or height <= 0:
        raise ValueError(f"{path}: invalid PNG dimensions")
    return width, height


def symbol_path_index(text: str, c_type: str, prefix: str) -> dict[str, str]:
    pattern = re.compile(
        rf"const\s+{re.escape(c_type)}\s+"
        rf"({re.escape(prefix)}[A-Za-z0-9_]+)\[\]\s*=\s*"
        rf"INCBIN_{re.escape(c_type.upper())}\(\"([^\"]+)\"\);"
    )
    return {match.group(1): match.group(2) for match in pattern.finditer(text)}


def parse_tileset_headers(source_root: Path) -> list[dict[str, Any]]:
    headers_path = source_root / "src/data/tilesets/headers.h"
    metatiles_path = source_root / "src/data/tilesets/metatiles.h"

    headers = headers_path.read_text(encoding="utf-8")
    metatiles = metatiles_path.read_text(encoding="utf-8")

    metatile_paths = symbol_path_index(
        metatiles,
        "u16",
        "gMetatiles_",
    )
    attribute_paths = symbol_path_index(
        metatiles,
        "u16",
        "gMetatileAttributes_",
    )

    tileset_pattern = re.compile(
        r"const\s+struct\s+Tileset\s+"
        r"(gTileset_[A-Za-z0-9_]+)\s*=\s*\{(.*?)\};",
        re.DOTALL,
    )

    def field(body: str, name: str) -> str:
        match = re.search(
            rf"\.{re.escape(name)}\s*=\s*([^,\n]+)",
            body,
        )
        if not match:
            raise ValueError(
                f"{headers_path}: tileset body missing .{name}"
            )
        return match.group(1).strip()

    records: list[dict[str, Any]] = []
    seen: set[str] = set()

    for match in tileset_pattern.finditer(headers):
        tileset = match.group(1)
        body = match.group(2)

        if tileset in seen:
            raise ValueError(f"{headers_path}: duplicate {tileset}")
        seen.add(tileset)

        compressed_token = field(body, "isCompressed")
        secondary_token = field(body, "isSecondary")
        metatile_symbol = field(body, "metatiles")
        attribute_symbol = field(body, "metatileAttributes")
        callback = field(body, "callback")

        if compressed_token not in {"TRUE", "FALSE"}:
            raise ValueError(
                f"{tileset}: unknown isCompressed {compressed_token}"
            )
        if secondary_token not in {"TRUE", "FALSE"}:
            raise ValueError(
                f"{tileset}: unknown isSecondary {secondary_token}"
            )

        if metatile_symbol not in metatile_paths:
            raise ValueError(
                f"{tileset}: unresolved metatile symbol {metatile_symbol}"
            )
        if attribute_symbol not in attribute_paths:
            raise ValueError(
                f"{tileset}: unresolved attribute symbol {attribute_symbol}"
            )

        metatile_rel = Path(metatile_paths[metatile_symbol])
        attribute_rel = Path(attribute_paths[attribute_symbol])
        asset_root = metatile_rel.parent

        if attribute_rel.parent != asset_root:
            raise ValueError(
                f"{tileset}: metatile/attribute roots differ"
            )

        metatile_file = source_root / metatile_rel
        attribute_file = source_root / attribute_rel
        tiles_png = source_root / asset_root / "tiles.png"
        palettes_dir = source_root / asset_root / "palettes"

        for required in (
            metatile_file,
            attribute_file,
            tiles_png,
            palettes_dir,
        ):
            if not required.exists():
                raise FileNotFoundError(
                    f"{tileset}: missing source asset {required}"
                )

        attribute_bytes = attribute_file.stat().st_size
        metatile_bytes = metatile_file.stat().st_size

        if attribute_bytes % 2:
            raise ValueError(
                f"{attribute_file}: attributes are not u16 aligned"
            )

        metatile_count = attribute_bytes // 2
        if metatile_bytes != metatile_count * METATILE_BYTES:
            raise ValueError(
                f"{tileset}: metatile bytes {metatile_bytes} do not match "
                f"{metatile_count} * {METATILE_BYTES}"
            )

        width, height = png_dimensions(tiles_png)
        if width % 8 or height % 8:
            raise ValueError(
                f"{tiles_png}: tile sheet dimensions must be 8px aligned"
            )

        palettes = sorted(
            path for path in palettes_dir.iterdir()
            if path.is_file() and path.suffix == ".gbapal"
        )
        if not palettes:
            raise ValueError(
                f"{tileset}: no .gbapal palettes in {palettes_dir}"
            )

        records.append(
            {
                "id": tileset,
                "is_compressed": compressed_token == "TRUE",
                "is_secondary": secondary_token == "TRUE",
                "callback": None if callback == "NULL" else callback,
                "asset_root": asset_root.as_posix(),
                "tiles_png": (asset_root / "tiles.png").as_posix(),
                "tiles_png_width": width,
                "tiles_png_height": height,
                "tile_count": (width // 8) * (height // 8),
                "metatiles_bin": metatile_rel.as_posix(),
                "metatile_attributes_bin": attribute_rel.as_posix(),
                "metatile_count": metatile_count,
                "palette_files": [
                    path.relative_to(source_root).as_posix()
                    for path in palettes
                ],
                "fingerprints": {
                    "tiles_png_sha256": sha256_file(tiles_png),
                    "metatiles_sha256": sha256_file(metatile_file),
                    "metatile_attributes_sha256": sha256_file(attribute_file),
                    "palettes_sha256": hashlib.sha256(
                        b"".join(
                            bytes.fromhex(sha256_file(path))
                            for path in palettes
                        )
                    ).hexdigest(),
                },
            }
        )

    if not records:
        raise ValueError(f"{headers_path}: no tilesets found")

    records.sort(key=lambda item: item["id"])
    return records


def build_manifest(source_root: Path) -> dict[str, Any]:
    records = parse_tileset_headers(source_root)
    return {
        "schema_version": SCHEMA_VERSION,
        "source": "Vanilla+ tileset source assets",
        "tileset_count": len(records),
        "tilesets": records,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_json", type=Path)
    args = parser.parse_args()

    manifest = build_manifest(args.source_root)
    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(
        f"R5 tileset manifest: {manifest['tileset_count']} tilesets -> "
        f"{args.output_json}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
