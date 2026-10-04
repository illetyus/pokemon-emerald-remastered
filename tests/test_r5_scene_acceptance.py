from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.build_r5_scene_acceptance import build_scene_acceptance

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "vendor" / "vanillaplus"


class R5SceneAcceptanceTests(unittest.TestCase):
    def test_real_brendan_house_to_route101_render_path(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "r5-scene-acceptance.json"
            report = build_scene_acceptance(SOURCE, output)

            self.assertEqual(report["status"], "PASS")
            self.assertEqual(report["world_map_count"], 518)
            self.assertGreater(report["render_tileset_count"], 60)

            path = report["acceptance_path"]
            self.assertEqual(
                path["start_map"],
                "MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F",
            )
            self.assertEqual(path["house_exit"]["x"], 8)
            self.assertEqual(path["house_exit"]["y"], 8)
            self.assertEqual(
                path["house_exit"]["dest_map"],
                "MAP_LITTLEROOT_TOWN",
            )
            self.assertEqual(
                path["north_connection"]["direction"],
                "up",
            )
            self.assertEqual(
                path["north_connection"]["dest_map"],
                "MAP_ROUTE101",
            )
            self.assertEqual(path["end_map"], "MAP_ROUTE101")

            maps = {item["id"]: item for item in report["maps"]}
            self.assertEqual(len(maps), 3)

            for item in maps.values():
                self.assertEqual(
                    item["render_plane_instances"],
                    item["tile_count"] * 2,
                    item["id"],
                )
                self.assertEqual(
                    item["render_quadrant_entries"],
                    item["tile_count"] * 8,
                    item["id"],
                )
                self.assertGreater(
                    item["unique_render_metatiles"],
                    0,
                    item["id"],
                )
                self.assertGreaterEqual(
                    len(item["used_tilesets"]),
                    1,
                    item["id"],
                )

            self.assertEqual(len(report["render_content_sha256"]), 64)
            self.assertTrue(output.is_file())


if __name__ == "__main__":
    unittest.main()
