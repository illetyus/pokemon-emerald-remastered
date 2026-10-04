#!/usr/bin/env python3
"""Build a deterministic, package-safe R5 Unreal render payload."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
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

        packaged_palettes: list[str] = []
        for index, palette_rel in enumerate(source["palette_files"]):
            packaged = tileset_dir / "palettes" / f"{index:02d}.pal"
            palette_sha = _copy_exact(
                source_root / palette_rel,
                output_root / packaged,
            )
            packaged_palettes.append(packaged.as_posix())
            file_hashes[packaged.as_posix()] = palette_sha

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
            "palette_files": packaged_palettes,
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
                "palette_files": packaged_palettes,
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
