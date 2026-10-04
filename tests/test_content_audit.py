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
                    {"id": "MAP_A", "file": "maps/A.json", "group_num": 0, "map_num": 0, "layout_num": 1},
                    {"id": "MAP_B", "file": "maps/B.json", "group_num": 0, "map_num": 1, "layout_num": 1},
                ],
            }
            (root / "manifest.json").write_text(json.dumps(manifest))

            for name, map_id, target, target_num, direction, direction_id in [
                ("A", "MAP_A", "MAP_B", 1, "dive", 5),
                ("B", "MAP_B", "MAP_A", 0, "emerge", 6),
            ]:
                doc = {
                    "schema_version": 1,
                    "map": {
                        "id": map_id,
                        "group_num": 0,
                        "map_num": 0 if map_id == "MAP_A" else 1,
                        "layout_num": 1,
                        "weather_id": 0,
                        "map_type_id": 1,
                        "connections": [
                            {
                                "map": target,
                                "direction": direction,
                                "direction_id": direction_id,
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
                        "layout_num": 1,
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
                    "layout_num": 1,
                    "weather_id": 0,
                    "map_type_id": 1,
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
                "maps": [{"id": "MAP_A", "file": "maps/A.json", "group_num": 0, "map_num": 0, "layout_num": 1}],
            }
            (root / "manifest.json").write_text(json.dumps(manifest))

            doc = {
                "schema_version": 1,
                "map": {
                    "id": "MAP_A",
                    "group_num": 0,
                    "map_num": 0,
                    "layout_num": 1,
                    "weather_id": 0,
                    "map_type_id": 1,
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
                        "layout_num": 1,
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
                    "layout_num": 1,
                    "weather_id": 0,
                    "map_type_id": 1,
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


    def test_r3_numeric_metadata_and_null_script_are_audited(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()

            (root / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map_count": 1,
                        "maps": [
                            {
                                "id": "MAP_A",
                                "file": "maps/A.json",
                                "group_num": 0,
                                "map_num": 0,
                                "layout_num": 1,
                            }
                        ],
                    }
                )
            )

            doc = {
                "schema_version": 1,
                "map": {
                    "id": "MAP_A",
                    "group_num": 0,
                    "map_num": 0,
                    "layout_num": 1,
                    "music": "MUS_TEST",
                    "music_id": None,
                    "region_map_section": "MAPSEC_TEST",
                    "region_map_section_id": None,
                    "weather_id": 0,
                    "map_type_id": 1,
                    "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
                    "battle_scene_id": None,
                    "connections": [
                        {
                            "map": "MAP_A",
                            "direction": "up",
                            "dest_group_num": 0,
                            "dest_map_num": 0,
                        }
                    ],
                    "object_events": [
                        {
                            "local_id": 1,
                            "script": "0x0",
                            "script_id": "0x0",
                        }
                    ],
                    "warp_events": [],
                    "coord_events": [],
                    "bg_events": [],
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

            self.assertTrue(any("music_id" in item for item in errors))
            self.assertTrue(any("region_map_section_id" in item for item in errors))
            self.assertTrue(any("battle_scene_id" in item for item in errors))
            self.assertTrue(any("direction_id" in item for item in errors))
            self.assertTrue(any("null script sentinel" in item for item in errors))


    def test_layout_catalog_count_and_usage_are_audited(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()
            (root / "layouts").mkdir()

            (root / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map_count": 1,
                        "layout_count": 2,
                        "layouts_file": "layouts.json",
                        "maps": [
                            {
                                "id": "MAP_A",
                                "name": "A",
                                "file": "maps/A.json",
                                "group_num": 0,
                                "map_num": 0,
                                "layout": "LAYOUT_A",
                                "layout_num": 1,
                            }
                        ],
                    }
                )
            )
            (root / "maps/A.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map": {
                            "id": "MAP_A",
                            "name": "A",
                            "group_num": 0,
                            "map_num": 0,
                            "layout": "LAYOUT_A",
                            "layout_num": 1,
                            "weather_id": 0,
                            "map_type_id": 1,
                            "connections": [],
                            "object_events": [],
                            "warp_events": [],
                            "coord_events": [],
                            "bg_events": [],
                        },
                        "layout": {
                            "id": "LAYOUT_A",
                            "layout_num": 1,
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
                )
            )
            (root / "layouts.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "layout_count": 1,
                        "layouts": [
                            {
                                "id": "LAYOUT_A",
                                "layout_num": 1,
                                "file": "layouts/LAYOUT_A.json",
                                "used_by_maps": ["A"],
                            }
                        ],
                    }
                )
            )
            (root / "layouts/LAYOUT_A.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "used_by_maps": ["A"],
                        "layout": {
                            "id": "LAYOUT_A",
                            "layout_num": 1,
                            "width": 1,
                            "height": 1,
                        },
                    }
                )
            )

            errors = audit(root)

            self.assertTrue(
                any("layout_count" in item for item in errors),
                errors,
            )


    def test_r3_script_catalog_ownership_and_event_refs_are_audited(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()
            (root / "scripts").mkdir()

            ownership = {
                "kind": "own",
                "owner": "A",
                "source": "data/maps/A/scripts.inc",
            }
            (root / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map_count": 1,
                        "maps": [
                            {
                                "id": "MAP_A",
                                "name": "A",
                                "file": "maps/A.json",
                                "group_num": 0,
                                "map_num": 0,
                                "layout_num": 1,
                                "script_ownership": ownership,
                            }
                        ],
                        "scripts_file": "scripts/manifest.json",
                        "script_label_count": 1,
                        "script_source_file_count": 1,
                    }
                )
            )
            (root / "scripts/manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map_count": 1,
                        "script_label_count": 1,
                        "script_source_file_count": 1,
                        "maps": [
                            {
                                "map": "A",
                                "map_id": "MAP_A",
                                "script_ownership": ownership,
                            }
                        ],
                        "labels": [
                            {
                                "script_id": "Known_EventScript",
                                "source": "data/maps/A/scripts.inc",
                                "line": 1,
                                "owner_map": "A",
                            }
                        ],
                        "commands": {},
                        "event_script_references": [],
                        "source_files": ["data/maps/A/scripts.inc"],
                    }
                )
            )

            doc = {
                "schema_version": 1,
                "map": {
                    "id": "MAP_A",
                    "name": "A",
                    "group_num": 0,
                    "map_num": 0,
                    "layout_num": 1,
                    "weather_id": 0,
                    "map_type_id": 1,
                    "script_ownership": {
                        "kind": "none",
                        "owner": None,
                        "source": None,
                    },
                    "connections": [],
                    "object_events": [
                        {
                            "local_id": 1,
                            "script": "Missing_EventScript",
                            "script_id": "Missing_EventScript",
                        }
                    ],
                    "warp_events": [],
                    "coord_events": [],
                    "bg_events": [],
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
                any("script ownership" in item for item in errors),
                errors,
            )
            self.assertTrue(
                any("unknown script_id" in item for item in errors),
                errors,
            )


    def test_r3_encounter_catalog_linkage_is_audited(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()

            (root / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map_count": 1,
                        "maps": [
                            {
                                "id": "MAP_A",
                                "name": "A",
                                "file": "maps/A.json",
                                "group_num": 0,
                                "map_num": 0,
                                "layout_num": 1,
                            }
                        ],
                        "encounters_file": "encounters.json",
                        "encounter_group_count": 1,
                        "map_encounter_count": 1,
                    }
                )
            )
            (root / "maps/A.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map": {
                            "id": "MAP_A",
                            "name": "A",
                            "group_num": 0,
                            "map_num": 0,
                            "layout_num": 1,
                            "weather_id": 0,
                            "map_type_id": 1,
                            "connections": [],
                            "object_events": [],
                            "warp_events": [],
                            "coord_events": [],
                            "bg_events": [],
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
                )
            )
            (root / "encounters.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "group_count": 1,
                        "map_encounter_count": 1,
                        "groups": [
                            {
                                "label": "gWild",
                                "for_maps": True,
                                "fields": [
                                    {
                                        "type": "land_mons",
                                        "encounter_rates": [100],
                                    }
                                ],
                                "encounters": [
                                    {
                                        "map": "MAP_MISSING",
                                        "map_name": "Missing",
                                        "group_num": 9,
                                        "map_num": 9,
                                        "land_mons": {
                                            "encounter_rate": 20,
                                            "mons": [
                                                {
                                                    "min_level": 5,
                                                    "max_level": 3,
                                                    "species": "SPECIES_TEST",
                                                    "species_id": None,
                                                }
                                            ],
                                        },
                                    }
                                ],
                            }
                        ],
                    }
                )
            )

            errors = audit(root)

            self.assertTrue(any("encounter map" in item for item in errors), errors)
            self.assertTrue(any("species_id" in item for item in errors), errors)
            self.assertTrue(any("level range" in item for item in errors), errors)


    def test_r3_provenance_and_map_fingerprint_tamper_are_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()

            (root / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "map_count": 1,
                        "maps": [
                            {
                                "id": "MAP_A",
                                "name": "A",
                                "file": "maps/A.json",
                                "group_num": 0,
                                "map_num": 0,
                                "layout_num": 1,
                            }
                        ],
                        "provenance_file": "provenance.json",
                    }
                )
            )
            (root / "maps/A.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "fingerprint_sha256": "bad",
                        "map": {
                            "id": "MAP_A",
                            "name": "A",
                            "group_num": 0,
                            "map_num": 0,
                            "layout_num": 1,
                            "weather_id": 0,
                            "map_type_id": 1,
                            "connections": [],
                            "object_events": [],
                            "warp_events": [],
                            "coord_events": [],
                            "bg_events": [],
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
                )
            )
            (root / "provenance.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "source_repository": "test/source",
                        "source_commit": "abc123",
                        "source_counts": {"map_count": 1},
                        "package_sha256": "0" * 64,
                        "file_sha256": {},
                    }
                )
            )

            errors = audit(root)

            self.assertTrue(
                any("map fingerprint" in item for item in errors),
                errors,
            )
            self.assertTrue(
                any("package fingerprint" in item for item in errors),
                errors,
            )


if __name__ == "__main__":
    unittest.main()
