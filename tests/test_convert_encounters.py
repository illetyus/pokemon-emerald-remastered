import json
import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from convert_encounters import EncounterConversionError, convert_encounters  # noqa: E402


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value), encoding="utf-8")


class ConvertEncountersTests(unittest.TestCase):
    def make_source(self, root: Path) -> None:
        write_json(
            root / "data/maps/map_groups.json",
            {
                "group_order": ["gMapGroup_Test"],
                "gMapGroup_Test": ["RouteTest"],
            },
        )
        write_json(
            root / "data/layouts/layouts.json",
            {"layouts": [{"id": "LAYOUT_ROUTE_TEST"}]},
        )
        write_json(
            root / "data/maps/RouteTest/map.json",
            {
                "id": "MAP_ROUTE_TEST",
                "name": "RouteTest",
                "layout": "LAYOUT_ROUTE_TEST",
            },
        )
        (root / "include/constants").mkdir(parents=True, exist_ok=True)
        (root / "include/constants/species.h").write_text(
            "#define SPECIES_NONE 0\n"
            "#define SPECIES_TESTMON 25\n"
            "#define SPECIES_OTHERMON 26\n",
            encoding="utf-8",
        )
        write_json(
            root / "src/data/wild_encounters.json",
            {
                "wild_encounter_groups": [
                    {
                        "label": "gWildMonHeaders",
                        "for_maps": True,
                        "fields": [
                            {
                                "type": "land_mons",
                                "encounter_rates": [60, 40],
                            },
                            {
                                "type": "fishing_mons",
                                "encounter_rates": [70, 30],
                                "groups": {"old_rod": [0], "good_rod": [1]},
                            },
                        ],
                        "encounters": [
                            {
                                "map": "MAP_ROUTE_TEST",
                                "base_label": "gRouteTest",
                                "land_mons": {
                                    "encounter_rate": 20,
                                    "mons": [
                                        {
                                            "min_level": 2,
                                            "max_level": 3,
                                            "species": "SPECIES_TESTMON",
                                        },
                                        {
                                            "min_level": 4,
                                            "max_level": 4,
                                            "species": "SPECIES_OTHERMON",
                                        },
                                    ],
                                },
                                "fishing_mons": {
                                    "encounter_rate": 30,
                                    "mons": [
                                        {
                                            "min_level": 5,
                                            "max_level": 10,
                                            "species": "SPECIES_TESTMON",
                                        },
                                        {
                                            "min_level": 10,
                                            "max_level": 15,
                                            "species": "SPECIES_OTHERMON",
                                        },
                                    ],
                                },
                            }
                        ],
                    },
                    {
                        "label": "gFacilityWildMons",
                        "for_maps": False,
                        "encounters": [
                            {
                                "base_label": "gFacility_1",
                                "land_mons": {
                                    "encounter_rate": 4,
                                    "mons": [
                                        {
                                            "min_level": 5,
                                            "max_level": 5,
                                            "species": "SPECIES_TESTMON",
                                        }
                                    ],
                                },
                            }
                        ],
                    },
                ]
            },
        )

    def test_preserves_groups_and_resolves_map_and_species_identity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)

            result = convert_encounters(root)

            self.assertEqual(result["group_count"], 2)
            self.assertEqual(result["map_encounter_count"], 1)
            mapped = result["groups"][0]["encounters"][0]
            self.assertEqual(mapped["map"], "MAP_ROUTE_TEST")
            self.assertEqual(mapped["map_name"], "RouteTest")
            self.assertEqual(mapped["group_num"], 0)
            self.assertEqual(mapped["map_num"], 0)
            self.assertEqual(mapped["land_mons"]["mons"][0]["species_id"], 25)
            self.assertEqual(mapped["fishing_mons"]["mons"][1]["species_id"], 26)
            self.assertEqual(
                result["groups"][0]["fields"][1]["groups"],
                {"old_rod": [0], "good_rod": [1]},
            )
            self.assertFalse(result["groups"][1]["for_maps"])
            self.assertEqual(
                result["groups"][1]["encounters"][0]["land_mons"]["mons"][0]["species_id"],
                25,
            )

    def test_unknown_map_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "src/data/wild_encounters.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["wild_encounter_groups"][0]["encounters"][0]["map"] = "MAP_MISSING"
            write_json(path, doc)

            with self.assertRaises(EncounterConversionError) as caught:
                convert_encounters(root)

            self.assertIn("MAP_MISSING", str(caught.exception))

    def test_unknown_species_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "src/data/wild_encounters.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["wild_encounter_groups"][0]["encounters"][0]["land_mons"]["mons"][0]["species"] = "SPECIES_MISSING"
            write_json(path, doc)

            with self.assertRaises(EncounterConversionError) as caught:
                convert_encounters(root)

            self.assertIn("SPECIES_MISSING", str(caught.exception))

    def test_invalid_level_range_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "src/data/wild_encounters.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            mon = doc["wild_encounter_groups"][0]["encounters"][0]["land_mons"]["mons"][0]
            mon["min_level"] = 10
            mon["max_level"] = 5
            write_json(path, doc)

            with self.assertRaises(EncounterConversionError) as caught:
                convert_encounters(root)

            self.assertIn("level range", str(caught.exception))

    def test_wrong_slot_count_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "src/data/wild_encounters.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["wild_encounter_groups"][0]["encounters"][0]["land_mons"]["mons"].pop()
            write_json(path, doc)

            with self.assertRaises(EncounterConversionError) as caught:
                convert_encounters(root)

            self.assertIn("slot count", str(caught.exception))

    def test_invalid_encounter_rate_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "src/data/wild_encounters.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["wild_encounter_groups"][0]["encounters"][0]["land_mons"]["encounter_rate"] = 101
            write_json(path, doc)

            with self.assertRaises(EncounterConversionError) as caught:
                convert_encounters(root)

            self.assertIn("encounter_rate", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
