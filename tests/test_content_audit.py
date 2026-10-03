import json
import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from audit_generated_content import audit  # noqa: E402


class ContentAuditTests(unittest.TestCase):
    def test_valid_two_map_world(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()

            manifest = {
                "schema_version": 1,
                "map_count": 2,
                "maps": [
                    {"id": "MAP_A", "file": "maps/A.json", "group_num": 0, "map_num": 0},
                    {"id": "MAP_B", "file": "maps/B.json", "group_num": 0, "map_num": 1},
                ],
            }
            (root / "manifest.json").write_text(json.dumps(manifest))

            for name, map_id, target, target_num in [
                ("A", "MAP_A", "MAP_B", 1),
                ("B", "MAP_B", "MAP_A", 0),
            ]:
                doc = {
                    "schema_version": 1,
                    "map": {
                        "id": map_id,
                        "group_num": 0,
                        "map_num": 0 if map_id == "MAP_A" else 1,
                        "connections": [
                            {
                                "map": target,
                                "dest_group_num": 0,
                                "dest_map_num": target_num,
                            }
                        ],
                        "object_events": [],
                        "warp_events": [],
                        "coord_events": [],
                    },
                    "layout": {
                        "width": 1,
                        "height": 1,
                        "source_word_count": 1,
                        "active_word_count": 1,
                        "raw_blocks_u16": [0],
                        "trailing_words_u16": [],
                        "border_source_word_count": 4,
                        "border_active_words_u16": [1, 2, 3, 4],
                        "border_trailing_words_u16": [],
                        "metatile_ids_u16": [0],
                        "collision_u8": [0],
                        "elevation_u8": [0],
                        "primary_metatile_attributes_u16": [0],
                        "secondary_metatile_attributes_u16": [0],
                        "primary_metatile_behavior_u8": [0],
                        "secondary_metatile_behavior_u8": [0],
                        "primary_metatile_layer_u8": [0],
                        "secondary_metatile_layer_u8": [0],
                    },
                }
                (root / f"maps/{name}.json").write_text(json.dumps(doc))

            self.assertEqual(audit(root), [])

    def test_dynamic_warp_is_not_treated_as_catalog_map(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()

            manifest = {
                "schema_version": 1,
                "map_count": 1,
                "maps": [
                    {
                        "id": "MAP_A",
                        "file": "maps/A.json",
                        "group_num": 0,
                        "map_num": 0,
                    }
                ],
            }
            (root / "manifest.json").write_text(json.dumps(manifest))

            doc = {
                "schema_version": 1,
                "map": {
                    "id": "MAP_A",
                    "group_num": 0,
                    "map_num": 0,
                    "connections": [],
                    "object_events": [],
                    "warp_events": [
                        {
                            "dest_map": "MAP_DYNAMIC",
                            "dest_warp_id_u16": 0x7F,
                            "dynamic_target": True,
                        }
                    ],
                    "coord_events": [],
                },
                "layout": {
                    "width": 1,
                    "height": 1,
                    "source_word_count": 1,
                    "active_word_count": 1,
                    "raw_blocks_u16": [0],
                    "trailing_words_u16": [],
                    "border_source_word_count": 4,
                    "border_active_words_u16": [1, 2, 3, 4],
                    "border_trailing_words_u16": [],
                    "metatile_ids_u16": [0],
                    "collision_u8": [0],
                    "elevation_u8": [0],
                    "primary_metatile_attributes_u16": [0],
                    "secondary_metatile_attributes_u16": [0],
                    "primary_metatile_behavior_u8": [0],
                    "secondary_metatile_behavior_u8": [0],
                    "primary_metatile_layer_u8": [0],
                    "secondary_metatile_layer_u8": [0],
                },
            }
            (root / "maps/A.json").write_text(json.dumps(doc))

            self.assertEqual(audit(root), [])

    def test_unknown_connection_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()

            manifest = {
                "schema_version": 1,
                "map_count": 1,
                "maps": [{"id": "MAP_A", "file": "maps/A.json", "group_num": 0, "map_num": 0}],
            }
            (root / "manifest.json").write_text(json.dumps(manifest))

            doc = {
                "schema_version": 1,
                "map": {
                    "id": "MAP_A",
                    "group_num": 0,
                    "map_num": 0,
                    "connections": [{"map": "MAP_MISSING"}],
                    "warp_events": [],
                },
                "layout": {
                    "width": 1,
                    "height": 1,
                    "source_word_count": 1,
                    "active_word_count": 1,
                    "raw_blocks_u16": [0],
                    "trailing_words_u16": [],
                    "border_source_word_count": 4,
                    "border_active_words_u16": [1, 2, 3, 4],
                    "border_trailing_words_u16": [],
                },
            }
            (root / "maps/A.json").write_text(json.dumps(doc))

            errors = audit(root)
            self.assertTrue(any("unknown map" in item for item in errors))


    def test_missing_numeric_event_identity_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()

            manifest = {
                "schema_version": 1,
                "map_count": 1,
                "maps": [
                    {
                        "id": "MAP_A",
                        "file": "maps/A.json",
                        "group_num": 0,
                        "map_num": 0,
                    }
                ],
            }
            (root / "manifest.json").write_text(json.dumps(manifest))

            doc = {
                "schema_version": 1,
                "map": {
                    "id": "MAP_A",
                    "group_num": 0,
                    "map_num": 0,
                    "connections": [],
                    "object_events": [
                        {
                            "local_id": 1,
                            "flag": "0",
                            "flag_id": 0,
                            "trainer_sight_or_berry_tree_id": "BERRY_TREE_TEST",
                        }
                    ],
                    "warp_events": [],
                    "coord_events": [],
                    "bg_events": [
                        {
                            "type": "sign",
                            "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH",
                            "kind_id": -1,
                        }
                    ],
                },
                "layout": {
                    "width": 1,
                    "height": 1,
                    "source_word_count": 1,
                    "active_word_count": 1,
                    "raw_blocks_u16": [0],
                    "trailing_words_u16": [],
                    "border_source_word_count": 4,
                    "border_active_words_u16": [1, 2, 3, 4],
                    "border_trailing_words_u16": [],
                    "metatile_ids_u16": [0],
                    "collision_u8": [0],
                    "elevation_u8": [0],
                    "primary_metatile_attributes_u16": [0],
                    "secondary_metatile_attributes_u16": [0],
                    "primary_metatile_behavior_u8": [0],
                    "secondary_metatile_behavior_u8": [0],
                    "primary_metatile_layer_u8": [0],
                    "secondary_metatile_layer_u8": [0],
                },
            }
            (root / "maps/A.json").write_text(json.dumps(doc))

            errors = audit(root)

            self.assertTrue(
                any("numeric trainer/berry id" in item for item in errors)
            )
            self.assertTrue(
                any("numeric facing id" in item for item in errors)
            )


if __name__ == "__main__":
    unittest.main()
