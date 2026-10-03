import json
import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from r3_script_catalog import R3ScriptCatalogError, build_script_catalog  # noqa: E402


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


class R3ScriptCatalogTests(unittest.TestCase):
    def make_source(self, root: Path) -> None:
        write_json(
            root / "data/maps/map_groups.json",
            {
                "group_order": ["gMapGroup_Test"],
                "gMapGroup_Test": ["MapA", "MapB", "MapC"],
            },
        )
        write_json(
            root / "data/layouts/layouts.json",
            {
                "layouts": [
                    {"id": "LAYOUT_A"},
                    {"id": "LAYOUT_B"},
                    {"id": "LAYOUT_C"},
                ]
            },
        )
        write_json(
            root / "data/maps/MapA/map.json",
            {
                "id": "MAP_A",
                "name": "MapA",
                "layout": "LAYOUT_A",
                "object_events": [
                    {"script": "MapA_EventScript_Talk"},
                    {"script": "0x0"},
                ],
                "coord_events": [],
                "bg_events": [],
            },
        )
        write_json(
            root / "data/maps/MapB/map.json",
            {
                "id": "MAP_B",
                "name": "MapB",
                "layout": "LAYOUT_B",
                "shared_scripts_map": "MapA",
                "object_events": [],
                "coord_events": [
                    {"script": "Shared_EventScript_Global"},
                ],
                "bg_events": [],
            },
        )
        write_json(
            root / "data/maps/MapC/map.json",
            {
                "id": "MAP_C",
                "name": "MapC",
                "layout": "LAYOUT_C",
                "object_events": [],
                "coord_events": [],
                "bg_events": [],
            },
        )

        (root / "data/maps/MapA/scripts.inc").write_text(
            """
MapA_MapScripts::
    map_script MAP_SCRIPT_ON_LOAD, MapA_OnLoad
    .byte 0

MapA_OnLoad::
    call Shared_EventScript_Global
    end

MapA_EventScript_Talk::
    deferredknown 7
    end
""".lstrip(),
            encoding="utf-8",
        )
        (root / "data/scripts").mkdir(parents=True, exist_ok=True)
        (root / "data/scripts/common.inc").write_text(
            """
Shared_EventScript_Global::
    end
""".lstrip(),
            encoding="utf-8",
        )
        (root / "asm/macros").mkdir(parents=True, exist_ok=True)
        (root / "asm/macros/event.inc").write_text(
            """
    .macro map_script kind:req, script:req
    .endm
    .macro call script:req
    .endm
    .macro end
    .endm
    .macro deferredknown value:req
    .endm
""".lstrip(),
            encoding="utf-8",
        )

    def test_map_ownership_and_event_script_resolution(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)

            catalog = build_script_catalog(root)
            by_map = {item["map"]: item for item in catalog["maps"]}

            self.assertEqual(
                by_map["MapA"]["script_ownership"],
                {"kind": "own", "owner": "MapA", "source": "data/maps/MapA/scripts.inc"},
            )
            self.assertEqual(
                by_map["MapB"]["script_ownership"],
                {"kind": "shared", "owner": "MapA", "source": "data/maps/MapA/scripts.inc"},
            )
            self.assertEqual(
                by_map["MapC"]["script_ownership"],
                {"kind": "none", "owner": None, "source": None},
            )

            refs = {
                (item["map"], item["script_id"])
                for item in catalog["event_script_references"]
            }
            self.assertIn(("MapA", "MapA_EventScript_Talk"), refs)
            self.assertIn(("MapB", "Shared_EventScript_Global"), refs)
            self.assertFalse(any(script_id == "0x0" for _, script_id in refs))

    def test_command_classification_reports_deferred_separately(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)

            catalog = build_script_catalog(root)
            commands = catalog["commands"]

            self.assertEqual(commands["end"]["conversion_classification"], "CORE")
            self.assertEqual(commands["end"]["runtime_status"], "r2_or_adapter")
            self.assertEqual(
                commands["deferredknown"]["conversion_classification"],
                "DEFERRED",
            )
            self.assertEqual(commands["deferredknown"]["runtime_status"], "deferred")

    def test_unresolved_event_script_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "data/maps/MapC/map.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["bg_events"] = [{"script": "Missing_EventScript"}]
            write_json(path, doc)

            with self.assertRaises(R3ScriptCatalogError) as caught:
                build_script_catalog(root)

            self.assertIn("Missing_EventScript", str(caught.exception))
            self.assertIn("MapC", str(caught.exception))

    def test_invalid_shared_script_owner_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            path = root / "data/maps/MapB/map.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["shared_scripts_map"] = "MissingMap"
            write_json(path, doc)

            with self.assertRaises(R3ScriptCatalogError) as caught:
                build_script_catalog(root)

            self.assertIn("MissingMap", str(caught.exception))

    def test_unresolved_branch_target_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            (root / "data/maps/MapA/scripts.inc").write_text(
                """
MapA_EventScript_Talk::
    goto Missing_Label
""".lstrip(),
                encoding="utf-8",
            )
            event_macros = root / "asm/macros/event.inc"
            event_macros.write_text(
                event_macros.read_text(encoding="utf-8")
                + "\n    .macro goto script:req\n    .endm\n",
                encoding="utf-8",
            )

            with self.assertRaises(R3ScriptCatalogError) as caught:
                build_script_catalog(root)

            self.assertIn("Missing_Label", str(caught.exception))

    def test_duplicate_script_label_is_a_hard_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source(root)
            (root / "data/scripts/duplicate.inc").write_text(
                "MapA_EventScript_Talk::\n    end\n",
                encoding="utf-8",
            )

            with self.assertRaises(R3ScriptCatalogError) as caught:
                build_script_catalog(root)

            self.assertIn("duplicate label", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
