from __future__ import annotations

import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ACQUISITION = ROOT / "data" / "r6" / "npc_asset_acquisition.json"
INVENTORY = ROOT / "data" / "r6" / "npc_source_inventory.json"


class R6NpcAssetAcquisitionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data = json.loads(ACQUISITION.read_text(encoding="utf-8"))
        cls.packages = cls.data["packages"]
        cls.by_asset = {p["asset_id"]: p for p in cls.packages}
        cls.findings = cls.data["identity_findings"]

    def test_all_selected_packages_are_recorded_once(self) -> None:
        inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
        selected_ids = {
            r["source_asset_id"]
            for r in inventory["records"]
            if r["status"] in (
                "oras_public_exact",
                "oras_public_combined_needs_inspection",
            )
        }
        self.assertEqual(len(selected_ids), 43)
        self.assertEqual(set(self.by_asset), selected_ids)
        self.assertEqual(len(self.packages), 43)

    def test_private_rehash_and_integrity_closure_is_recorded(self) -> None:
        v = self.data["verification"]
        self.assertEqual(v["private_repository_visibility_verified"], "private")
        self.assertTrue(v["all_private_archive_sha256_verified"])
        self.assertTrue(v["all_zip_integrity_verified"])
        self.assertTrue(v["all_packages_have_skin_controller"])
        self.assertEqual(v["packages_with_embedded_animation"], 0)

    def test_every_package_hash_is_sha256_and_no_private_paths_are_public(self) -> None:
        for package in self.packages:
            self.assertRegex(package["source_archive_sha256"], r"^[0-9a-f]{64}$")
        serialized = json.dumps(self.data, sort_keys=True)
        for forbidden in (
            "pokemon-emerald-remastered-assets",
            "/home/",
            "/Users/",
            "/tmp/",
            "R6/NPCs/source/",
        ):
            self.assertNotIn(forbidden, serialized)

    def test_swimmer_package_only_closes_female_source(self) -> None:
        self.assertEqual(
            self.findings["OBJ_EVENT_GFX_SWIMMER_F"]["resolution"],
            "package_model_explicit",
        )
        self.assertEqual(
            self.findings["OBJ_EVENT_GFX_SWIMMER_M"]["resolution"],
            "source_gap",
        )

    def test_interviewers_do_not_absorb_generic_male_reporter(self) -> None:
        self.assertEqual(
            self.findings["OBJ_EVENT_GFX_REPORTER_F"]["source_role"],
            "Gabby",
        )
        self.assertEqual(
            self.findings["OBJ_EVENT_GFX_CAMERAMAN"]["source_role"],
            "Ty",
        )
        self.assertEqual(
            self.findings["OBJ_EVENT_GFX_REPORTER_M"]["resolution"],
            "source_gap",
        )

    def test_multi_model_packages_remain_explicitly_unresolved(self) -> None:
        self.assertEqual(self.by_asset[296325]["model_count"], 2)
        self.assertEqual(self.by_asset[299514]["model_count"], 2)
        self.assertEqual(self.by_asset[299537]["model_count"], 2)
        self.assertEqual(self.by_asset[296336]["model_count"], 2)
        self.assertEqual(self.by_asset[299541]["model_count"], 3)
        for name in (
            "OBJ_EVENT_GFX_LIZA",
            "OBJ_EVENT_GFX_TATE",
            "OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M",
            "OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F",
            "OBJ_EVENT_GFX_CYCLING_TRIATHLETE_M",
            "OBJ_EVENT_GFX_CYCLING_TRIATHLETE_F",
            "OBJ_EVENT_GFX_WALLY",
            "OBJ_EVENT_GFX_YOUNGSTER",
        ):
            self.assertIn("unresolved", self.findings[name]["resolution"])


if __name__ == "__main__":
    unittest.main()
