from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from tools.build_r5_render_package import build_render_package

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "vendor" / "vanillaplus"


class R5RenderPackageTests(unittest.TestCase):
    def build(self, root: Path) -> dict:
        return build_render_package(SOURCE, root)

    def test_full_real_tileset_package_is_self_contained(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "render"
            manifest = self.build(out)

            self.assertEqual(manifest["schema_version"], 1)
            self.assertGreater(manifest["tileset_count"], 60)
            self.assertEqual(
                manifest["tileset_count"],
                len(manifest["tilesets"]),
            )
            self.assertGreater(manifest["file_count"], 1000)
            self.assertEqual(len(manifest["content_sha256"]), 64)

            for item in manifest["tilesets"]:
                descriptor = out / item["descriptor_file"]
                tiles_png = out / item["tiles_png_file"]
                tile_indices = out / item["tile_indices_file"]
                palette_lut = out / item["palette_lut_file"]

                self.assertTrue(descriptor.is_file(), item["id"])
                self.assertTrue(tiles_png.is_file(), item["id"])
                self.assertTrue(tile_indices.is_file(), item["id"])
                self.assertTrue(palette_lut.is_file(), item["id"])
                self.assertEqual(len(item["palette_files"]), 16)

                for palette in item["palette_files"]:
                    self.assertTrue((out / palette).is_file(), item["id"])

                document = json.loads(
                    descriptor.read_text(encoding="utf-8")
                )
                self.assertEqual(document["id"], item["id"])
                self.assertEqual(
                    document["tiles_png"],
                    item["tiles_png_file"],
                )
                self.assertEqual(
                    document["tiles_index8"],
                    item["tiles_index8_file"],
                )

                indexed_pixels = tiles_index8.read_bytes()
                self.assertEqual(
                    len(indexed_pixels),
                    document["tiles_png_width"]
                    * document["tiles_png_height"],
                    item["id"],
                )
                if indexed_pixels:
                    self.assertLessEqual(
                        max(indexed_pixels),
                        15,
                        item["id"],
                    )
                self.assertEqual(
                    document["palette_files"],
                    item["palette_files"],
                )
                self.assertEqual(
                    document["tile_indices_file"],
                    item["tile_indices_file"],
                )
                self.assertEqual(
                    document["palette_lut_file"],
                    item["palette_lut_file"],
                )

                index_bytes = tile_indices.read_bytes()
                self.assertEqual(
                    len(index_bytes),
                    document["tiles_png_width"]
                    * document["tiles_png_height"],
                    item["id"],
                )
                self.assertLess(max(index_bytes, default=0), 16, item["id"])

                lut_bytes = palette_lut.read_bytes()
                self.assertEqual(len(lut_bytes), 16 * 16 * 4, item["id"])

                self.assertEqual(
                    len(document["metatiles"]),
                    item["metatile_count"],
                )

    def test_general_palette_lut_preserves_real_jasc_colors(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "render"
            manifest = self.build(out)
            by_id = {item["id"]: item for item in manifest["tilesets"]}
            general = by_id["gTileset_General"]

            lut = (out / general["palette_lut_file"]).read_bytes()
            self.assertEqual(lut[:4], bytes((24, 41, 82, 255)))

            descriptor = json.loads(
                (out / general["descriptor_file"]).read_text(encoding="utf-8")
            )
            self.assertEqual(descriptor["palette_lut_width"], 16)
            self.assertEqual(descriptor["palette_lut_height"], 16)

    def test_package_paths_do_not_escape_output_root(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "render"
            manifest = self.build(out)
            resolved_root = out.resolve()

            self.assertTrue(
                all(
                    "tiles.index8" in entry["tiles_index8_file"]
                    for entry in manifest["tilesets"]
                )
            )

            for relative in manifest["files_sha256"]:
                candidate = (out / relative).resolve()
                self.assertTrue(
                    candidate.is_relative_to(resolved_root),
                    relative,
                )
                self.assertNotIn("..", Path(relative).parts)
                self.assertFalse(Path(relative).is_absolute())

    def test_split_source_roots_remain_distinct_after_packaging(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "render"
            manifest = self.build(out)
            by_id = {item["id"]: item for item in manifest["tilesets"]}

            brown = json.loads(
                (out / by_id["gTileset_SecretBaseBrownCave"]["descriptor_file"])
                .read_text(encoding="utf-8")
            )
            tree = json.loads(
                (out / by_id["gTileset_SecretBaseTree"]["descriptor_file"])
                .read_text(encoding="utf-8")
            )

            self.assertEqual(
                brown["metatile_asset_root_source"],
                tree["metatile_asset_root_source"],
            )
            self.assertNotEqual(
                brown["visual_asset_root_source"],
                tree["visual_asset_root_source"],
            )
            self.assertNotEqual(
                brown["tiles_png"],
                tree["tiles_png"],
            )

    def test_package_is_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            first = self.build(root / "a")
            second = self.build(root / "b")

            self.assertEqual(first, second)
            self.assertEqual(
                (root / "a" / "manifest.json").read_bytes(),
                (root / "b" / "manifest.json").read_bytes(),
            )

            for relative in first["files_sha256"]:
                self.assertEqual(
                    (root / "a" / relative).read_bytes(),
                    (root / "b" / relative).read_bytes(),
                    relative,
                )


if __name__ == "__main__":
    unittest.main()
