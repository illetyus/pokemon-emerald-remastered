#!/usr/bin/env python3
"""Build a deterministic, package-safe R5 Unreal render payload."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import zlib
from pathlib import Path
from typing import Any

try:
    from tools.build_r5_metatile_descriptors import build_descriptors
except ModuleNotFoundError:
    from build_r5_metatile_descriptors import build_descriptors

SCHEMA_VERSION = 1
SOURCE_REPOSITORY = "illetyus/pokezumrut-vanillaplus"
SOURCE_COMMIT = "70db90c9077aed1272e746fc2537d9f12b95a91c"


def _json_bytes(value: Any) -> bytes:
    return (
        json.dumps(value, indent=2, sort_keys=True) + "\n"
    ).encode("utf-8")


def _sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _paeth_predictor(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def _decode_indexed_png(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    signature = b"\x89PNG\r\n\x1a\n"
    if not data.startswith(signature):
        raise ValueError(f"{path}: not a PNG")

    position = len(signature)
    width = height = -1
    bit_depth = color_type = interlace = -1
    palette_entries = 0
    compressed = bytearray()

    while position + 12 <= len(data):
        length = struct.unpack(">I", data[position:position + 4])[0]
        chunk_type = data[position + 4:position + 8]
        payload_start = position + 8
        payload_end = payload_start + length
        crc_end = payload_end + 4

        if crc_end > len(data):
            raise ValueError(f"{path}: truncated PNG chunk")

        payload = data[payload_start:payload_end]

        if chunk_type == b"IHDR":
            if length != 13:
                raise ValueError(f"{path}: invalid IHDR length")
            (
                width,
                height,
                bit_depth,
                color_type,
                compression,
                filter_method,
                interlace,
            ) = struct.unpack(">IIBBBBB", payload)

            if compression != 0 or filter_method != 0:
                raise ValueError(f"{path}: unsupported PNG compression/filter method")
        elif chunk_type == b"PLTE":
            if length % 3:
                raise ValueError(f"{path}: invalid PLTE length")
            palette_entries = length // 3
        elif chunk_type == b"IDAT":
            compressed.extend(payload)
        elif chunk_type == b"IEND":
            break

        position = crc_end

    if width <= 0 or height <= 0:
        raise ValueError(f"{path}: missing/invalid IHDR")
    if bit_depth != 8 or color_type != 3 or interlace != 0:
        raise ValueError(
            f"{path}: expected non-interlaced 8-bit indexed PNG, "
            f"got bit_depth={bit_depth} color_type={color_type} interlace={interlace}"
        )
    if palette_entries != 16:
        raise ValueError(
            f"{path}: expected 16 PNG palette entries, got {palette_entries}"
        )
    if not compressed:
        raise ValueError(f"{path}: missing IDAT")

    raw = zlib.decompress(bytes(compressed))
    stride = width
    expected = height * (stride + 1)
    if len(raw) != expected:
        raise ValueError(
            f"{path}: decompressed byte count {len(raw)} != {expected}"
        )

    pixels = bytearray()
    previous = bytearray(stride)
    position = 0

    for row_index in range(height):
        filter_type = raw[position]
        position += 1
        scanline = bytearray(raw[position:position + stride])
        position += stride

        for x in range(stride):
            left = scanline[x - 1] if x > 0 else 0
            up = previous[x]
            upper_left = previous[x - 1] if x > 0 else 0

            if filter_type == 0:
                value = scanline[x]
            elif filter_type == 1:
                value = (scanline[x] + left) & 0xFF
            elif filter_type == 2:
                value = (scanline[x] + up) & 0xFF
            elif filter_type == 3:
                value = (
                    scanline[x] + ((left + up) // 2)
                ) & 0xFF
            elif filter_type == 4:
                value = (
                    scanline[x]
                    + _paeth_predictor(left, up, upper_left)
                ) & 0xFF
            else:
                raise ValueError(
                    f"{path}: unsupported PNG filter {filter_type} "
                    f"on row {row_index}"
                )

            scanline[x] = value

        if any(index >= 16 for index in scanline):
            raise ValueError(
                f"{path}: indexed tile sheet references palette index >= 16"
            )

        pixels.extend(scanline)
        previous = scanline

    return width, height, bytes(pixels)


def _parse_jasc_palette(path: Path) -> list[tuple[int, int, int]]:
    lines = [
        line.strip()
        for line in path.read_text(encoding="ascii").splitlines()
        if line.strip()
    ]
    if len(lines) != 19 or lines[:3] != ["JASC-PAL", "0100", "16"]:
        raise ValueError(f"{path}: invalid JASC-PAL-0100 palette")

    colors: list[tuple[int, int, int]] = []
    for row in lines[3:]:
        parts = row.split()
        if len(parts) != 3:
            raise ValueError(f"{path}: invalid RGB row {row!r}")
        rgb = tuple(int(part, 10) for part in parts)
        if any(channel < 0 or channel > 255 for channel in rgb):
            raise ValueError(f"{path}: RGB channel outside 0..255")
        colors.append(rgb)  # type: ignore[arg-type]

    if len(colors) != 16:
        raise ValueError(f"{path}: expected 16 colors")
    return colors


def _build_palette_lut(source_root: Path, palette_files: list[str]) -> bytes:
    if len(palette_files) != 16:
        raise ValueError("R5 palette LUT requires exactly 16 palettes")

    rgba = bytearray()
    for palette_rel in palette_files:
        for red, green, blue in _parse_jasc_palette(
            source_root / palette_rel
        ):
            rgba.extend((red, green, blue, 255))

    if len(rgba) != 16 * 16 * 4:
        raise AssertionError("R5 palette LUT byte count mismatch")
    return bytes(rgba)


def _safe_tileset_dir(tileset_id: str) -> str:
    if not tileset_id.startswith("gTileset_"):
        raise ValueError(f"unexpected tileset id {tileset_id!r}")
    suffix = tileset_id.removeprefix("gTileset_")
    if not suffix or any(not (ch.isalnum() or ch in "_-") for ch in suffix):
        raise ValueError(f"unsafe tileset id {tileset_id!r}")
    return tileset_id


def _write_bytes(path: Path, data: bytes) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return _sha256_bytes(data)


def _copy_exact(source: Path, target: Path) -> str:
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    return _sha256_file(target)


def build_render_package(
    source_root: Path,
    output_root: Path,
    *,
    source_repository: str = SOURCE_REPOSITORY,
    source_commit: str = SOURCE_COMMIT,
) -> dict[str, Any]:
    descriptors = build_descriptors(source_root)

    if output_root.exists():
        shutil.rmtree(output_root)
    output_root.mkdir(parents=True, exist_ok=True)

    package_tilesets: list[dict[str, Any]] = []
    file_hashes: dict[str, str] = {}

    for source in descriptors["tilesets"]:
        tileset_id = source["id"]
        tileset_dir = Path("tilesets") / _safe_tileset_dir(tileset_id)

        packaged_tiles = tileset_dir / "tiles.png"
        tiles_sha = _copy_exact(
            source_root / source["tiles_png"],
            output_root / packaged_tiles,
        )
        file_hashes[packaged_tiles.as_posix()] = tiles_sha

        index_width, index_height, tile_indices = _decode_indexed_png(
            source_root / source["tiles_png"]
        )
        if (
            index_width != source["tiles_png_width"]
            or index_height != source["tiles_png_height"]
        ):
            raise ValueError(
                f"{tileset_id}: indexed PNG dimensions disagree with source manifest"
            )

        packaged_indices = tileset_dir / "tiles.idx8"
        indices_sha = _write_bytes(
            output_root / packaged_indices,
            tile_indices,
        )
        file_hashes[packaged_indices.as_posix()] = indices_sha

        packaged_palettes: list[str] = []
        for index, palette_rel in enumerate(source["palette_files"]):
            packaged = tileset_dir / "palettes" / f"{index:02d}.pal"
            palette_sha = _copy_exact(
                source_root / palette_rel,
                output_root / packaged,
            )
            packaged_palettes.append(packaged.as_posix())
            file_hashes[packaged.as_posix()] = palette_sha

        palette_lut = _build_palette_lut(
            source_root,
            source["palette_files"],
        )
        packaged_palette_lut = tileset_dir / "palettes.rgba8"
        palette_lut_sha = _write_bytes(
            output_root / packaged_palette_lut,
            palette_lut,
        )
        file_hashes[packaged_palette_lut.as_posix()] = palette_lut_sha

        descriptor = {
            "schema_version": SCHEMA_VERSION,
            "id": tileset_id,
            "is_secondary": source["is_secondary"],
            "metatile_asset_root_source": source["metatile_asset_root"],
            "visual_asset_root_source": source["visual_asset_root"],
            "tile_symbol": source["tile_symbol"],
            "palette_symbol": source["palette_symbol"],
            "tiles_png": packaged_tiles.as_posix(),
            "tiles_png_width": source["tiles_png_width"],
            "tiles_png_height": source["tiles_png_height"],
            "tile_count": source["tile_count"],
            "tile_indices_file": packaged_indices.as_posix(),
            "tile_indices_sha256": indices_sha,
            "palette_files": packaged_palettes,
            "palette_lut_file": packaged_palette_lut.as_posix(),
            "palette_lut_sha256": palette_lut_sha,
            "palette_lut_width": 16,
            "palette_lut_height": 16,
            "metatile_count": source["metatile_count"],
            "source_fingerprints": source["fingerprints"],
            "metatiles": source["metatiles"],
        }

        descriptor_rel = tileset_dir / "descriptor.json"
        descriptor_sha = _write_bytes(
            output_root / descriptor_rel,
            _json_bytes(descriptor),
        )
        file_hashes[descriptor_rel.as_posix()] = descriptor_sha

        package_tilesets.append(
            {
                "id": tileset_id,
                "is_secondary": source["is_secondary"],
                "descriptor_file": descriptor_rel.as_posix(),
                "descriptor_sha256": descriptor_sha,
                "tiles_png_file": packaged_tiles.as_posix(),
                "tiles_png_sha256": tiles_sha,
                "tile_indices_file": packaged_indices.as_posix(),
                "tile_indices_sha256": indices_sha,
                "palette_files": packaged_palettes,
                "palette_lut_file": packaged_palette_lut.as_posix(),
                "palette_lut_sha256": palette_lut_sha,
                "metatile_count": source["metatile_count"],
            }
        )

    package_tilesets.sort(key=lambda item: item["id"])
    sorted_hashes = dict(sorted(file_hashes.items()))

    content_digest = hashlib.sha256()
    for relative, digest in sorted_hashes.items():
        content_digest.update(relative.encode("utf-8"))
        content_digest.update(b"\0")
        content_digest.update(bytes.fromhex(digest))

    manifest = {
        "schema_version": SCHEMA_VERSION,
        "source_repository": source_repository,
        "source_commit": source_commit,
        "tileset_count": len(package_tilesets),
        "tilesets": package_tilesets,
        "file_count": len(sorted_hashes),
        "files_sha256": sorted_hashes,
        "content_sha256": content_digest.hexdigest(),
    }

    manifest_bytes = _json_bytes(manifest)
    (output_root / "manifest.json").write_bytes(manifest_bytes)
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_root", type=Path)
    parser.add_argument(
        "--source-repository",
        default=SOURCE_REPOSITORY,
    )
    parser.add_argument(
        "--source-commit",
        default=SOURCE_COMMIT,
    )
    args = parser.parse_args()

    manifest = build_render_package(
        args.source_root,
        args.output_root,
        source_repository=args.source_repository,
        source_commit=args.source_commit,
    )

    print(
        "R5 render package: "
        f"{manifest['tileset_count']} tilesets, "
        f"{manifest['file_count']} payload files, "
        f"sha256={manifest['content_sha256']} -> "
        f"{args.output_root}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
