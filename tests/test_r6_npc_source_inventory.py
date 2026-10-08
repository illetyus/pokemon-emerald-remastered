from __future__ import annotations

import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INVENTORY = ROOT / "data" / "r6" / "npc_source_inventory.json"
PLAYER_STATES = ROOT / "data" / "r6" / "player_presentation_states.json"


class R6NpcSourceInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data = json.loads(INVENTORY.read_text(encoding="utf-8"))
        cls.records = cls.data["records"]
        cls.by_name = {r["graphics_name"]: r for r in cls.records}

    def test_all_non_player_human_identities_are_planned(self) -> None:
        players = json.loads(PLAYER_STATES.read_text(encoding="utf-8"))
        player_names = {r["graphics_name"] for r in players["records"]}
        self.assertEqual(self.data["identity_count"], 121)
        self.assertEqual(len(self.records), 121)
        self.assertEqual(len(self.by_name), 121)
        self.assertTrue(player_names.isdisjoint(self.by_name))

    def test_source_catalog_counts_are_pinned(self) -> None:
        sections = self.data["source_catalog"]["human_sections"]
        self.assertEqual(sections["Unique NPCs (Overworld)"], 33)
        self.assertEqual(sections["Trainer Classes"], 50)
        self.assertFalse(
            self.data["source_catalog"]["generic_overworld_npc_section_present"]
        )

    def test_core_named_hoenn_characters_have_direct_oras_packages(self) -> None:
        expected = {
            "OBJ_EVENT_GFX_PROF_BIRCH": 296314,
            "OBJ_EVENT_GFX_ROXANNE": 296327,
            "OBJ_EVENT_GFX_BRAWLY": 296311,
            "OBJ_EVENT_GFX_WATTSON": 296330,
            "OBJ_EVENT_GFX_FLANNERY": 296297,
            "OBJ_EVENT_GFX_NORMAN": 296313,
            "OBJ_EVENT_GFX_WINONA": 296310,
            "OBJ_EVENT_GFX_WALLACE": 296329,
            "OBJ_EVENT_GFX_STEVEN": 296334,
            "OBJ_EVENT_GFX_WALLY": 296336,
            "OBJ_EVENT_GFX_ARCHIE": 296332,
            "OBJ_EVENT_GFX_MAXIE": 296333,
            "OBJ_EVENT_GFX_MOM": 296685,
        }
        for name, asset_id in expected.items():
            with self.subTest(name=name):
                rec = self.by_name[name]
                self.assertEqual(rec["status"], "oras_public_exact")
                self.assertEqual(rec["source_asset_id"], asset_id)

    def test_combined_packages_are_not_silently_treated_as_exact(self) -> None:
        for name in (
            "OBJ_EVENT_GFX_LIZA",
            "OBJ_EVENT_GFX_TATE",
            "OBJ_EVENT_GFX_REPORTER_M",
            "OBJ_EVENT_GFX_REPORTER_F",
            "OBJ_EVENT_GFX_CAMERAMAN",
            "OBJ_EVENT_GFX_SWIMMER_M",
            "OBJ_EVENT_GFX_SWIMMER_F",
        ):
            self.assertEqual(
                self.by_name[name]["status"],
                "oras_public_combined_needs_inspection",
            )

    def test_rivals_reuse_player_base_without_copying_private_paths(self) -> None:
        for name in (
            "OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL",
            "OBJ_EVENT_GFX_RIVAL_MAY_NORMAL",
            "OBJ_EVENT_GFX_LINK_BRENDAN",
            "OBJ_EVENT_GFX_LINK_MAY",
        ):
            rec = self.by_name[name]
            self.assertEqual(rec["status"], "reuse_verified_player_base")
            self.assertTrue(rec["reuse_model_id"].startswith("character."))
            self.assertNotIn("/", rec["reuse_model_id"])
            self.assertNotIn("\\", rec["reuse_model_id"])

    def test_known_legacy_characters_are_explicitly_deferred(self) -> None:
        for name in (
            "OBJ_EVENT_GFX_ANABEL",
            "OBJ_EVENT_GFX_TUCKER",
            "OBJ_EVENT_GFX_GRETA",
            "OBJ_EVENT_GFX_SPENSER",
            "OBJ_EVENT_GFX_NOLAND",
            "OBJ_EVENT_GFX_LUCY",
            "OBJ_EVENT_GFX_JUAN",
            "OBJ_EVENT_GFX_SCOTT",
            "OBJ_EVENT_GFX_RED",
            "OBJ_EVENT_GFX_LEAF",
            "OBJ_EVENT_GFX_BRANDON",
        ):
            self.assertEqual(
                self.by_name[name]["status"],
                "secondary_source_required",
            )

    def test_no_inventory_record_contains_private_machine_paths(self) -> None:
        serialized = json.dumps(self.data, sort_keys=True)
        for forbidden in ("/home/", "/Users/", "/tmp/", "C:\\"):
            self.assertNotIn(forbidden, serialized)


if __name__ == "__main__":
    unittest.main()
