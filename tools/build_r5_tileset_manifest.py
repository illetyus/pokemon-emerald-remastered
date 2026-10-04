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


def validate_jasc_palette(path: Path) -> None:
    lines = [
        line.strip()
        for line in path.read_text(encoding="ascii").splitlines()
        if line.strip()
    ]

    if len(lines) != 19:
        raise ValueError(
            f"{path}: expected JASC-PAL header plus 16 RGB rows, "
            f"got {len(lines)} non-empty lines"
        )
    if lines[0] != "JASC-PAL":
        raise ValueError(f"{path}: palette is not JASC-PAL")
    if lines[1] != "0100":
        raise ValueError(f"{path}: unsupported JASC-PAL version {lines[1]!r}")
    if lines[2] != "16":
        raise ValueError(f"{path}: expected 16 palette colors, got {lines[2]!r}")

    for index, line in enumerate(lines[3:]):
        parts = line.split()
        if len(parts) != 3:
            raise ValueError(
                f"{path}: RGB row {index} must have exactly 3 channels"
            )
        try:
            channels = [int(part, 10) for part in parts]
        except ValueError as exc:
            raise ValueError(
                f"{path}: RGB row {index} contains a non-integer channel"
            ) from exc
        if any(channel < 0 or channel > 255 for channel in channels):
            raise ValueError(
                f"{path}: RGB row {index} channel outside 0..255"
            )


def symbol_path_index(text: str, c_type: str, prefix: str) -> dict[str, str]:
    pattern = re.compile(
        rf"const\s+{re.escape(c_type)}\s+"
        rf"({re.escape(prefix)}[A-Za-z0-9_]+)\[\]\s*=\s*"
        rf"INCBIN_{re.escape(c_type.upper())}\(\"([^\"]+)\"\);"
    )
    return {match.group(1): match.group(2) for match in pattern.finditer(text)}


def palette_path_index(text: str) -> dict[str, list[str]]:
    pattern = re.compile(
        r"const\s+u16\s+"
        r"(gTilesetPalettes_[A-Za-z0-9_]+)\[\]\[16\]\s*=\s*"
        r"\{(.*?)\};",
        re.DOTALL,
    )
    incbin = re.compile(r'INCBIN_U16\("([^"]+)"\)')
    result: dict[str, list[str]] = {}

    for match in pattern.finditer(text):
        paths = incbin.findall(match.group(2))
        if paths:
            result[match.group(1)] = paths

    return result


def source_palette_path(compiled_path: str) -> Path:
    path = Path(compiled_path)
    if path.suffix != ".gbapal":
        raise ValueError(
            f"{compiled_path}: expected compiled .gbapal palette path"
        )
    return path.with_suffix(".pal")


def resolve_visual_asset_root(
    source_root: Path,
    tileset: str,
    metatile_root: Path,
    tile_symbol: str,
    palette_symbol: str,
    tile_paths: dict[str, str],
    palette_paths: dict[str, list[str]],
) -> tuple[Path, list[Path]]:
    tile_compiled = tile_paths.get(tile_symbol)
    palette_compiled = palette_paths.get(palette_symbol)

    if tile_compiled is not None:
        visual_root = Path(tile_compiled).parent
    else:
        visual_root = metatile_root

    tiles_png = source_root / visual_root / "tiles.png"
    palettes_dir = source_root / visual_root / "palettes"

    if palette_compiled is not None:
        source_palettes = [
            source_palette_path(path)
            for path in palette_compiled
        ]
        palette_roots = {path.parent.parent for path in source_palettes}
        if len(palette_roots) != 1:
            raise ValueError(
                f"{tileset}: palette symbol {palette_symbol} spans "
                f"multiple asset roots: {sorted(map(str, palette_roots))}"
            )
        palette_root = next(iter(palette_roots))
        if palette_root != visual_root:
            raise ValueError(
                f"{tileset}: tile root {visual_root} and palette root "
                f"{palette_root} disagree"
            )
    else:
        source_palettes = [
            visual_root / "palettes" / f"{index:02d}.pal"
            for index in range(16)
        ]

    if not tiles_png.is_file() or not palettes_dir.is_dir():
        raise FileNotFoundError(
            f"{tileset}: unresolved visual source root {visual_root}; "
            f"tile symbol={tile_symbol}, palette symbol={palette_symbol}"
        )

    return visual_root, source_palettes


def parse_tileset_headers(source_root: Path) -> list[dict[str, Any]]:
    headers_path = source_root / "src/data/tilesets/headers.h"
    metatiles_path = source_root / "src/data/tilesets/metatiles.h"
    graphics_path = source_root / "src/data/tilesets/graphics.h"

    headers = headers_path.read_text(encoding="utf-8")
    metatiles = metatiles_path.read_text(encoding="utf-8")
    graphics = graphics_path.read_text(encoding="utf-8")

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

    tile_paths = symbol_path_index(
        graphics,
        "u32",
        "gTilesetTiles_",
    )
    palette_paths = palette_path_index(graphics)

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
        tile_symbol = field(body, "tiles")
        palette_symbol = field(body, "palettes")
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

        visual_root, palette_rel_paths = resolve_visual_asset_root(
            source_root,
            tileset,
            asset_root,
            tile_symbol,
            palette_symbol,
            tile_paths,
            palette_paths,
        )
        tiles_png = source_root / visual_root / "tiles.png"
        palettes_dir = source_root / visual_root / "palettes"

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

        palettes = [
            source_root / path
            for path in palette_rel_paths
        ]
        expected_palette_names = [
            f"{index:02d}.pal"
            for index in range(16)
        ]
        palette_names = [path.name for path in palettes]

        if palette_names != expected_palette_names:
            raise ValueError(
                f"{tileset}: expected source palettes 00.pal..15.pal, "
                f"got {palette_names}"
            )

        for palette in palettes:
            if not palette.is_file():
                raise FileNotFoundError(
                    f"{tileset}: missing source palette {palette}"
                )
            validate_jasc_palette(palette)

        records.append(
            {
                "id": tileset,
                "is_compressed": compressed_token == "TRUE",
                "is_secondary": secondary_token == "TRUE",
                "callback": None if callback == "NULL" else callback,
                "metatile_asset_root": asset_root.as_posix(),
                "visual_asset_root": visual_root.as_posix(),
                "tile_symbol": tile_symbol,
                "palette_symbol": palette_symbol,
                "tiles_png": (visual_root / "tiles.png").as_posix(),
                "tiles_png_width": width,
                "tiles_png_height": height,
                "tile_count": (width // 8) * (height // 8),
                "metatiles_bin": metatile_rel.as_posix(),
                "metatile_attributes_bin": attribute_rel.as_posix(),
                "metatile_count": metatile_count,
                "palette_format": "JASC-PAL-0100",
                "palette_color_count": 16,
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
