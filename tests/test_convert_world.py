import json
import struct
import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from convert_world import convert_world  # noqa: E402


class ConvertWorldTests(unittest.TestCase):
    def test_lossless_map_conversion(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "source"
            out = Path(temp) / "out"

            (root / "data/layouts/TestTown").mkdir(parents=True)
            (root / "data/maps/TestTown").mkdir(parents=True)

            (root / "data/layouts/layouts.json").write_text(
                json.dumps(
                    {
                        "layouts": [
                            {
                                "id": "LAYOUT_TEST_TOWN",
                                "name": "TestTown_Layout",
                                "width": 2,
                                "height": 2,
                                "primary_tileset": "Primary",
                                "secondary_tileset": "Secondary",
                                "blockdata_filepath": "data/layouts/TestTown/map.bin",
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )

            (root / "data/layouts/TestTown/map.bin").write_bytes(
                struct.pack("<6H", 1, 2, 0x1234, 0xFFFF, 0x1111, 0x2222)
            )

            (root / "data/maps/TestTown/map.json").write_text(
                json.dumps(
                    {
                        "id": "MAP_TEST_TOWN",
                        "name": "TestTown",
                        "layout": "LAYOUT_TEST_TOWN",
                        "music": "MUS_TEST",
                        "weather": "WEATHER_NONE",
                        "map_type": "MAP_TYPE_TOWN",
                        "allow_running": True,
                        "connections": [
                            {"map": "MAP_TEST_ROUTE", "offset": 0, "direction": "up"}
                        ],
                        "object_events": [
                            {
                                "graphics_id": "OBJ_TEST",
                                "x": 1,
                                "y": 1,
                                "script": "Test_EventScript",
                                "flag": "FLAG_TEST",
                            }
                        ],
                        "warp_events": [],
                        "coord_events": [],
                        "bg_events": [],
                    }
                ),
                encoding="utf-8",
            )

            manifest = convert_world(root, out)

            self.assertEqual(manifest["map_count"], 1)
            converted = json.loads((out / "maps/TestTown.json").read_text())
            self.assertEqual(converted["layout"]["source_word_count"], 6)
            self.assertEqual(converted["layout"]["active_word_count"], 4)
            self.assertEqual(converted["layout"]["raw_blocks_u16"], [1, 2, 0x1234, 0xFFFF])
            self.assertEqual(converted["layout"]["trailing_words_u16"], [0x1111, 0x2222])
            self.assertEqual(converted["layout"]["metatile_ids_u16"], [1, 2, 0x234, 0x3FF])
            self.assertEqual(converted["layout"]["collision_u8"], [0, 0, 0, 3])
            self.assertEqual(converted["layout"]["elevation_u8"], [0, 0, 1, 15])
            self.assertEqual(converted["map"]["object_events"][0]["flag"], "FLAG_TEST")
            self.assertEqual(converted["map"]["connections"][0]["direction"], "up")


if __name__ == "__main__":
    unittest.main()
