from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from tools.build_r5_tileset_manifest import build_manifest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "vendor" / "vanillaplus"


class R5TilesetManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.manifest = build_manifest(SOURCE)
        cls.by_id = {
            item["id"]: item
            for item in cls.manifest["tilesets"]
        }

    def test_real_source_manifest_is_nontrivial_and_unique(self) -> None:
        self.assertEqual(self.manifest["schema_version"], 1)
        self.assertEqual(
            self.manifest["tileset_count"],
            len(self.manifest["tilesets"]),
        )
        self.assertGreater(self.manifest["tileset_count"], 60)
        self.assertEqual(
            len(self.by_id),
            self.manifest["tileset_count"],
        )

    def test_general_primary_tileset_geometry(self) -> None:
        general = self.by_id["gTileset_General"]
        self.assertFalse(general["is_secondary"])
        self.assertEqual(general["metatile_count"], 512)
        self.assertEqual(
            general["asset_root"],
            "data/tilesets/primary/general",
        )
        self.assertEqual(
            general["tiles_png"],
            "data/tilesets/primary/general/tiles.png",
        )
        self.assertEqual(len(general["palette_files"]), 16)

    def test_brendans_house_secondary_tileset_geometry(self) -> None:
        house = self.by_id["gTileset_BrendansMaysHouse"]
        self.assertTrue(house["is_secondary"])
        self.assertEqual(house["metatile_count"], 196)
        self.assertEqual(
            house["asset_root"],
            "data/tilesets/secondary/brendans_mays_house",
        )
        self.assertEqual(len(house["palette_files"]), 16)

    def test_all_source_assets_and_fingerprints_are_present(self) -> None:
        for item in self.manifest["tilesets"]:
            for key in (
                "tiles_png",
                "metatiles_bin",
                "metatile_attributes_bin",
            ):
                self.assertTrue((SOURCE / item[key]).is_file(), item["id"])

            self.assertGreater(item["metatile_count"], 0)
            self.assertGreater(item["tile_count"], 0)
            self.assertEqual(item["tiles_png_width"] % 8, 0)
            self.assertEqual(item["tiles_png_height"] % 8, 0)

            for digest in item["fingerprints"].values():
                self.assertEqual(len(digest), 64)
                int(digest, 16)

    def test_manifest_is_deterministic_json(self) -> None:
        first = json.dumps(
            self.manifest,
            sort_keys=True,
            separators=(",", ":"),
        )
        second = json.dumps(
            build_manifest(SOURCE),
            sort_keys=True,
            separators=(",", ":"),
        )
        self.assertEqual(first, second)


if __name__ == "__main__":
    unittest.main()
