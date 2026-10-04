from __future__ import annotations

import unittest
from pathlib import Path

from tools.build_r5_metatile_descriptors import (
    H_FLIP_MASK,
    PALETTE_SHIFT,
    TILE_ID_MASK,
    V_FLIP_MASK,
    build_descriptors,
)

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "vendor" / "vanillaplus"


class R5MetatileDescriptorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.document = build_descriptors(SOURCE)
        cls.by_id = {
            item["id"]: item
            for item in cls.document["tilesets"]
        }

    def test_real_source_descriptor_catalog(self) -> None:
        self.assertEqual(self.document["schema_version"], 1)
        self.assertGreater(self.document["tileset_count"], 60)
        self.assertEqual(
            self.document["tileset_count"],
            len(self.document["tilesets"]),
        )

    def test_primary_and_secondary_identity_is_preserved(self) -> None:
        general = self.by_id["gTileset_General"]
        house = self.by_id["gTileset_BrendansMaysHouse"]

        self.assertFalse(general["is_secondary"])
        self.assertTrue(house["is_secondary"])
        self.assertEqual(len(general["metatiles"]), 512)
        self.assertEqual(len(house["metatiles"]), 196)

    def test_every_real_metatile_has_eight_decoded_entries(self) -> None:
        for tileset in self.document["tilesets"]:
            self.assertEqual(
                tileset["metatile_count"],
                len(tileset["metatiles"]),
                tileset["id"],
            )
            for metatile in tileset["metatiles"]:
                self.assertEqual(len(metatile["entries"]), 8)
                self.assertIn(metatile["layer_type"], (0, 1, 2))
                self.assertEqual(len(metatile["render_planes"]), 2)

                for entry in metatile["entries"]:
                    self.assertGreaterEqual(entry["tile_id_raw"], 0)
                    self.assertLessEqual(entry["tile_id_raw"], TILE_ID_MASK)
                    self.assertGreaterEqual(entry["palette"], 0)
                    self.assertLessEqual(entry["palette"], 15)
                    self.assertIn(entry["source_layer"], (0, 1))
                    self.assertIn(entry["x"], (0, 1))
                    self.assertIn(entry["y"], (0, 1))

                    reconstructed = (
                        entry["tile_id_raw"]
                        | (H_FLIP_MASK if entry["h_flip"] else 0)
                        | (V_FLIP_MASK if entry["v_flip"] else 0)
                        | (entry["palette"] << PALETTE_SHIFT)
                    )
                    self.assertEqual(
                        reconstructed,
                        entry["raw_u16"],
                        tileset["id"],
                    )

    def test_layer_type_plane_mapping_matches_vanilla_contract(self) -> None:
        self.assertEqual(
            self.document["layer_planes"],
            {
                "0": ["middle", "top"],
                "1": ["bottom", "middle"],
                "2": ["bottom", "top"],
            },
        )

    def test_descriptor_build_is_deterministic(self) -> None:
        self.assertEqual(self.document, build_descriptors(SOURCE))


if __name__ == "__main__":
    unittest.main()
