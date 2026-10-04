import json
import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from r3_source_catalog import SourceCatalogError, build_source_catalog  # noqa: E402


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


class R3SourceCatalogTests(unittest.TestCase):
    def make_source(self, root: Path) -> None:
        write_json(
            root / "data/maps/map_groups.json",
            {
                "group_order": ["gMapGroup_Test", "gMapGroup_Other"],
                "gMapGroup_Test": ["TestTown", "TestRoute"],
                "gMapGroup_Other": ["TestHouse"],
            },
        )
        write_json(
            root / "data/layouts/layouts.json",
            {
                "layouts": [
                    {"id": "LAYOUT_TEST_TOWN"},
                    {"id": "LAYOUT_TEST_ROUTE"},
                    {"id": "LAYOUT_TEST_HOUSE"},
                    {"id": "LAYOUT_UNUSED"},
                ]
            },
        )
        for name, map_id, layout, shared in [
            ("TestTown", "MAP_TEST_TOWN", "LAYOUT_TEST_TOWN", None),
            ("TestRoute", "MAP_TEST_ROUTE", "LAYOUT_TEST_ROUTE", None),
            ("TestHouse", "MAP_TEST_HOUSE", "LAYOUT_TEST_HOUSE", "TestTown"),
        ]:
            payload = {
                "id": map_id,
                "name": name,
                "layout": layout,
            }
            if shared is not None:
                payload["shared_scripts_map"] = shared
            write_json(root / f"data/maps/{name}/map.json", payload)

        (root / "data/maps/TestTown/scripts.inc").write_text(
            "TestTown_EventScript::\n\tend\n",
            encoding="utf-8",
        )
        (root / "data/maps/TestRoute/scripts.inc").write_text(
            "TestRoute_EventScript::\n\tend\n",
            encoding="utf-8",
        )

    def test_catalog_counts_and_numeric_map_order_are_deterministic(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)

            catalog = build_source_catalog(root, source_commit="abc123")

            self.assertEqual(catalog["source_commit"], "abc123")
            self.assertEqual(catalog["group_count"], 2)
            self.assertEqual(catalog["map_count"], 3)
            self.assertEqual(catalog["layout_count"], 4)
            self.assertEqual(catalog["map_script_file_count"], 2)
            self.assertEqual(
                [(item["name"], item["group_num"], item["map_num"]) for item in catalog["maps"]],
                [
                    ("TestTown", 0, 0),
                    ("TestRoute", 0, 1),
                    ("TestHouse", 1, 0),
                ],
            )
            self.assertEqual(
                catalog["maps"][2]["script_ownership"],
                {"kind": "shared", "owner": "TestTown"},
            )

    def test_missing_map_json_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            (root / "data/maps/TestRoute/map.json").unlink()

            with self.assertRaises(SourceCatalogError) as caught:
                build_source_catalog(root)

            self.assertIn("TestRoute", str(caught.exception))
            self.assertIn("missing map.json", str(caught.exception))

    def test_extra_map_json_not_present_in_group_catalog_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            write_json(
                root / "data/maps/Orphan/map.json",
                {
                    "id": "MAP_ORPHAN",
                    "name": "Orphan",
                    "layout": "LAYOUT_TEST_TOWN",
                },
            )

            with self.assertRaises(SourceCatalogError) as caught:
                build_source_catalog(root)

            self.assertIn("Orphan", str(caught.exception))
            self.assertIn("map_groups", str(caught.exception))

    def test_duplicate_map_id_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            route_path = root / "data/maps/TestRoute/map.json"
            route = json.loads(route_path.read_text(encoding="utf-8"))
            route["id"] = "MAP_TEST_TOWN"
            write_json(route_path, route)

            with self.assertRaises(SourceCatalogError) as caught:
                build_source_catalog(root)

            self.assertIn("duplicate map id", str(caught.exception))

    def test_unknown_layout_reference_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            house_path = root / "data/maps/TestHouse/map.json"
            house = json.loads(house_path.read_text(encoding="utf-8"))
            house["layout"] = "LAYOUT_MISSING"
            write_json(house_path, house)

            with self.assertRaises(SourceCatalogError) as caught:
                build_source_catalog(root)

            self.assertIn("LAYOUT_MISSING", str(caught.exception))

    def test_script_ownership_distinguishes_own_shared_and_scriptless(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            (root / "data/maps/TestRoute/scripts.inc").unlink()

            catalog = build_source_catalog(root)
            by_name = {item["name"]: item for item in catalog["maps"]}

            self.assertEqual(by_name["TestTown"]["script_ownership"]["kind"], "own")
            self.assertEqual(by_name["TestHouse"]["script_ownership"]["kind"], "shared")
            self.assertEqual(by_name["TestRoute"]["script_ownership"]["kind"], "none")


    def test_global_shared_script_owner_is_supported(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "data/maps/TestHouse/map.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["shared_scripts_map"] = "SharedHub"
            write_json(path, doc)

            shared = root / "data/scripts/shared_hub.inc"
            shared.parent.mkdir(parents=True, exist_ok=True)
            shared.write_text(
                "SharedHub_MapScripts::\n    .byte 0\n",
                encoding="utf-8",
            )

            catalog = build_source_catalog(root)
            by_name = {item["name"]: item for item in catalog["maps"]}

            self.assertEqual(
                by_name["TestHouse"]["script_ownership"],
                {"kind": "shared", "owner": "SharedHub"},
            )
            self.assertEqual(
                catalog["global_shared_script_sources"]["SharedHub"],
                "data/scripts/shared_hub.inc",
            )

    def test_pinned_baseline_metadata_fixture(self):
        fixture = json.loads(
            (ROOT / "tests/fixtures/r3/vanillaplus_catalog.json").read_text(
                encoding="utf-8"
            )
        )

        self.assertEqual(
            fixture["source_commit"],
            "70db90c9077aed1272e746fc2537d9f12b95a91c",
        )
        self.assertEqual(fixture["group_count"], 34)
        self.assertEqual(fixture["map_count"], 518)
        self.assertEqual(fixture["layout_count"], 441)
        self.assertEqual(fixture["map_script_file_count"], 468)
        self.assertEqual(len(fixture["map_files"]), 518)
        self.assertEqual(len(fixture["layout_ids"]), 441)
        self.assertEqual(len(fixture["map_script_files"]), 468)


if __name__ == "__main__":
    unittest.main()
