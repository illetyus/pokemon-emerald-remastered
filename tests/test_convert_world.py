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
            (root / "data/maps/AliasTown").mkdir(parents=True)
            (root / "src/data/tilesets").mkdir(parents=True)
            (root / "include/constants").mkdir(parents=True)
            (root / "data/tilesets/primary/test_primary").mkdir(parents=True)
            (root / "data/tilesets/secondary/test_secondary").mkdir(parents=True)

            (root / "data/maps/map_groups.json").write_text(
                json.dumps(
                    {
                        "group_order": ["gMapGroup_Test"],
                        "gMapGroup_Test": ["TestTown", "AliasTown"],
                    }
                ),
                encoding="utf-8",
            )

            (root / "include/constants/flags.h").write_text(
                "#define FLAG_TEST 0x0123\n",
                encoding="utf-8",
            )
            (root / "include/constants/vars.h").write_text(
                "#define VARS_START 0x4000\n#define VAR_TEST (VARS_START + 7)\n",
                encoding="utf-8",
            )
            (root / "include/constants/opponents.h").write_text(
                "#define TRAINER_TEST 42\n",
                encoding="utf-8",
            )
            (root / "include/constants/weather.h").write_text(
                "#define WEATHER_NONE 0\n"
                "#define WEATHER_TEST 9\n",
                encoding="utf-8",
            )
            (root / "include/constants/map_types.h").write_text(
                "#define MAP_TYPE_TOWN 1\n"
                "#define MAP_TYPE_INDOOR 8\n",
                encoding="utf-8",
            )
            (root / "include/constants/items.h").write_text(
                "#define ITEM_TEST 16\n",
                encoding="utf-8",
            )
            (root / "include/constants/event_bg.h").write_text(
                "#define BG_EVENT_PLAYER_FACING_ANY 0\n"
                "#define BG_EVENT_PLAYER_FACING_NORTH 1\n"
                "#define BG_EVENT_PLAYER_FACING_SOUTH 2\n"
                "#define BG_EVENT_PLAYER_FACING_EAST 3\n"
                "#define BG_EVENT_PLAYER_FACING_WEST 4\n",
                encoding="utf-8",
            )
            (root / "include/constants/maps.h").write_text(
                "#define WARP_ID_DYNAMIC 0x7F\n",
                encoding="utf-8",
            )
            (root / "include/constants/layouts.h").write_text(
                "#define LAYOUT_TEST_TOWN 1\n",
                encoding="utf-8",
            )
            (root / "include/constants/secret_bases.h").write_text(
                "#define SECRET_BASE_TEST 33\n",
                encoding="utf-8",
            )
            (root / "include/constants/berry.h").write_text(
                "#define BERRY_TREE_TEST 52\n",
                encoding="utf-8",
            )

            (root / "data/layouts/layouts.json").write_text(
                json.dumps(
                    {
                        "layouts": [
                            {
                                "id": "LAYOUT_TEST_TOWN",
                                "name": "TestTown_Layout",
                                "width": 2,
                                "height": 2,
                                "primary_tileset": "gTileset_TestPrimary",
                                "secondary_tileset": "gTileset_TestSecondary",
                                "border_filepath": "data/layouts/TestTown/border.bin",
                                "blockdata_filepath": "data/layouts/TestTown/map.bin",
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )

            (root / "src/data/tilesets/headers.h").write_text(
                """
const struct Tileset gTileset_TestPrimary =
{
    .isSecondary = FALSE,
    .metatileAttributes = gMetatileAttributes_PrimaryStorageName,
};
const struct Tileset gTileset_TestSecondary =
{
    .isSecondary = TRUE,
    .metatileAttributes = gMetatileAttributes_SecondaryStorageName,
};
""",
                encoding="utf-8",
            )
            (root / "src/data/tilesets/metatiles.h").write_text(
                """
const u16 gMetatileAttributes_PrimaryStorageName[] = INCBIN_U16("data/tilesets/primary/test_primary/metatile_attributes.bin");
const u16 gMetatileAttributes_SecondaryStorageName[] = INCBIN_U16("data/tilesets/secondary/test_secondary/metatile_attributes.bin");
""",
                encoding="utf-8",
            )

            (root / "data/tilesets/primary/test_primary/metatile_attributes.bin").write_bytes(
                struct.pack("<4H", 0x0001, 0x1002, 0xF003, 0x2004)
            )
            (root / "data/tilesets/secondary/test_secondary/metatile_attributes.bin").write_bytes(
                struct.pack("<3H", 0x0005, 0x3006, 0xF007)
            )

            (root / "data/layouts/TestTown/map.bin").write_bytes(
                struct.pack("<6H", 1, 2, 0x1234, 0xFFFF, 0x1111, 0x2222)
            )
            (root / "data/layouts/TestTown/border.bin").write_bytes(
                struct.pack("<6H", 0x000A, 0x000B, 0x000C, 0x000D, 0xAAAA, 0xBBBB)
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
                            },
                            {
                                "graphics_id": "OBJ_BERRY_TREE",
                                "x": 0,
                                "y": 1,
                                "trainer_sight_or_berry_tree_id": "BERRY_TREE_TEST",
                                "script": "BerryTreeScript",
                                "flag": "0",
                            },
                        ],
                        "warp_events": [
                            {
                                "x": 0,
                                "y": 1,
                                "elevation": 0,
                                "dest_map": "MAP_TEST_TOWN",
                                "dest_warp_id": "2",
                            },
                            {
                                "x": 1,
                                "y": 1,
                                "elevation": 0,
                                "dest_map": "MAP_DYNAMIC",
                                "dest_warp_id": "WARP_ID_DYNAMIC",
                            },
                        ],
                        "coord_events": [
                            {
                                "type": "trigger",
                                "x": 1,
                                "y": 0,
                                "elevation": 0,
                                "var": "VAR_TEST",
                                "var_value": "3",
                                "script": "Test_CoordScript",
                            },
                            {
                                "type": "weather",
                                "x": 0,
                                "y": 0,
                                "elevation": 0,
                                "weather": "WEATHER_TEST",
                            },
                        ],
                        "bg_events": [
                            {
                                "type": "sign",
                                "x": 0,
                                "y": 0,
                                "elevation": 0,
                                "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH",
                                "script": "Test_SignScript",
                            },
                            {
                                "type": "hidden_item",
                                "x": 1,
                                "y": 1,
                                "elevation": 3,
                                "item": "ITEM_TEST",
                                "flag": "FLAG_TEST",
                            },
                            {
                                "type": "secret_base",
                                "x": 1,
                                "y": 0,
                                "elevation": 0,
                                "secret_base_id": "SECRET_BASE_TEST",
                            },
                        ],
                    }
                ),
                encoding="utf-8",
            )


            (root / "data/maps/AliasTown/map.json").write_text(
                json.dumps(
                    {
                        "id": "MAP_ALIAS_TOWN",
                        "name": "AliasTown",
                        "layout": "LAYOUT_TEST_TOWN",
                        "music": "MUS_TEST",
                        "weather": "WEATHER_NONE",
                        "map_type": "MAP_TYPE_INDOOR",
                        "allow_running": False,
                        "connections": None,
                        "shared_events_map": "TestTown",
                        "shared_scripts_map": "TestTown",
                    }
                ),
                encoding="utf-8",
            )

            manifest = convert_world(root, out)

            self.assertEqual(manifest["map_count"], 2)
            self.assertEqual(manifest["maps"][0]["group_num"], 0)
            self.assertEqual(manifest["maps"][0]["map_num"], 1)
            self.assertEqual(manifest["maps"][0]["layout_num"], 1)
            self.assertEqual(manifest["maps"][1]["group_num"], 0)
            self.assertEqual(manifest["maps"][1]["map_num"], 0)
            self.assertEqual(manifest["maps"][1]["layout_num"], 1)
            converted = json.loads((out / "maps/TestTown.json").read_text())
            self.assertEqual(converted["map"]["group_name"], "gMapGroup_Test")
            self.assertEqual(converted["map"]["group_num"], 0)
            self.assertEqual(converted["map"]["map_num"], 0)
            self.assertEqual(converted["map"]["layout_num"], 1)
            self.assertEqual(converted["map"]["weather_id"], 0)
            self.assertEqual(converted["map"]["map_type_id"], 1)
            self.assertEqual(converted["layout"]["source_word_count"], 6)
            self.assertEqual(converted["layout"]["active_word_count"], 4)
            self.assertEqual(converted["layout"]["border_source_word_count"], 6)
            self.assertEqual(converted["layout"]["border_active_words_u16"], [0x000A, 0x000B, 0x000C, 0x000D])
            self.assertEqual(converted["layout"]["border_trailing_words_u16"], [0xAAAA, 0xBBBB])
            self.assertEqual(converted["layout"]["raw_blocks_u16"], [1, 2, 0x1234, 0xFFFF])
            self.assertEqual(converted["layout"]["trailing_words_u16"], [0x1111, 0x2222])
            self.assertEqual(converted["layout"]["metatile_ids_u16"], [1, 2, 0x234, 0x3FF])
            self.assertEqual(converted["layout"]["collision_u8"], [0, 0, 0, 3])
            self.assertEqual(converted["layout"]["elevation_u8"], [0, 0, 1, 15])
            self.assertEqual(
                converted["source"]["primary_metatile_attributes"],
                "data/tilesets/primary/test_primary/metatile_attributes.bin",
            )
            self.assertEqual(
                converted["source"]["secondary_metatile_attributes"],
                "data/tilesets/secondary/test_secondary/metatile_attributes.bin",
            )
            self.assertEqual(
                converted["layout"]["primary_metatile_attributes_u16"],
                [0x0001, 0x1002, 0xF003, 0x2004],
            )
            self.assertEqual(
                converted["layout"]["secondary_metatile_attributes_u16"],
                [0x0005, 0x3006, 0xF007],
            )
            self.assertEqual(
                converted["layout"]["primary_metatile_behavior_u8"],
                [1, 2, 3, 4],
            )
            self.assertEqual(
                converted["layout"]["primary_metatile_layer_u8"],
                [0, 1, 15, 2],
            )
            self.assertEqual(converted["map"]["object_events"][0]["flag"], "FLAG_TEST")
            self.assertEqual(converted["map"]["object_events"][0]["flag_id"], 0x123)
            self.assertEqual(converted["map"]["object_events"][0]["local_id"], 1)
            self.assertEqual(converted["map"]["object_events"][1]["local_id"], 2)
            self.assertEqual(
                converted["map"]["object_events"][1]["trainer_sight_or_berry_tree_id_u16"],
                52,
            )
            self.assertEqual(converted["map"]["warp_events"][0]["dest_group_num"], 0)
            self.assertEqual(converted["map"]["warp_events"][0]["dest_map_num"], 0)
            self.assertEqual(converted["map"]["warp_events"][0]["dest_warp_id_u16"], 2)
            self.assertFalse(converted["map"]["warp_events"][0]["dynamic_target"])
            self.assertTrue(converted["map"]["warp_events"][1]["dynamic_target"])
            self.assertEqual(converted["map"]["warp_events"][1]["dest_warp_id_u16"], 0x7F)
            self.assertNotIn("dest_group_num", converted["map"]["warp_events"][1])
            self.assertNotIn("dest_map_num", converted["map"]["warp_events"][1])
            self.assertEqual(converted["map"]["coord_events"][0]["var_id"], 0x4007)
            self.assertEqual(converted["map"]["coord_events"][0]["var_value_u16"], 3)
            self.assertEqual(converted["map"]["coord_events"][1]["weather_id"], 9)
            self.assertEqual(converted["map"]["bg_events"][0]["kind_id"], 1)
            self.assertEqual(converted["map"]["bg_events"][0]["player_facing_dir_id"], 1)
            self.assertEqual(converted["map"]["bg_events"][1]["kind_id"], 7)
            self.assertEqual(converted["map"]["bg_events"][1]["item_id"], 16)
            self.assertEqual(converted["map"]["bg_events"][1]["flag_id"], 0x123)
            self.assertEqual(converted["map"]["bg_events"][2]["kind_id"], 8)
            self.assertEqual(converted["map"]["bg_events"][2]["secret_base_id_u16"], 33)
            self.assertEqual(converted["map"]["connections"][0]["direction"], "up")

            alias_converted = json.loads(
                (out / "maps/AliasTown.json").read_text()
            )
            self.assertEqual(
                alias_converted["map"]["shared_events_map"],
                "TestTown",
            )
            self.assertEqual(
                alias_converted["map"]["shared_scripts_map"],
                "TestTown",
            )
            self.assertEqual(alias_converted["map"]["weather_id"], 0)
            self.assertEqual(alias_converted["map"]["map_type_id"], 8)
            self.assertEqual(
                alias_converted["source"]["shared_events_json"],
                "data/maps/TestTown/map.json",
            )
            self.assertEqual(alias_converted["map"]["connections"], [])
            self.assertEqual(
                alias_converted["map"]["object_events"],
                converted["map"]["object_events"],
            )
            self.assertEqual(
                alias_converted["map"]["warp_events"],
                converted["map"]["warp_events"],
            )
            self.assertEqual(
                alias_converted["map"]["coord_events"],
                converted["map"]["coord_events"],
            )
            self.assertEqual(
                alias_converted["map"]["bg_events"],
                converted["map"]["bg_events"],
            )


if __name__ == "__main__":
    unittest.main()
