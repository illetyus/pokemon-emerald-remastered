from __future__ import annotations

import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from tools.build_r6_character_manifest import ManifestError, build_manifest

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data" / "r6"
VENDOR = ROOT / "vendor" / "vanillaplus"
OVERRIDES = DATA / "character_presentation_overrides.json"


class R6NpcModelMappingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.mapping = json.loads((DATA / "npc_model_mapping.json").read_text())
        cls.records = cls.mapping["records"]
        cls.acquisition = json.loads((DATA / "npc_asset_acquisition.json").read_text())
        cls.inventory = json.loads((DATA / "npc_source_inventory.json").read_text())
        cls.overrides = json.loads(OVERRIDES.read_text())
        cls.manifest = build_manifest(VENDOR, OVERRIDES)
        cls.entries = {r["graphics_name"]: r for r in cls.manifest["entries"]}

    def test_scope_covers_only_all_single_model_named_packages(self) -> None:
        expected = {
            p["selected_identity_names"][0]: p
            for p in self.acquisition["packages"]
            if p["section"] == "Unique NPCs (Overworld)" and p["model_count"] == 1
        }
        self.assertEqual(self.mapping["phase"], "R6-I3C-1")
        self.assertEqual(self.mapping["mapping_count"], 16)
        self.assertEqual(len(self.records), 16)
        self.assertEqual({r["graphics_name"] for r in self.records}, set(expected))
        for r in self.records:
            p = expected[r["graphics_name"]]
            self.assertEqual(r["source_asset_id"], p["asset_id"])
            self.assertEqual(r["source_submodel_file"], Path(p["model_paths"][0]).name)
            self.assertEqual(r["source_sha256"], [p["source_archive_sha256"]])
            self.assertEqual(r["source_page"], p["source_page"])
            self.assertRegex(r["source_submodel_sha256"], r"^[0-9a-f]{64}$")
            self.assertTrue(p["zip_integrity_verified"])

    def test_identity_and_named_model_cannot_be_substituted(self) -> None:
        inventory = {r["graphics_name"]: r for r in self.inventory["records"]}
        for r in self.records:
            with self.subTest(name=r["graphics_name"]):
                source = inventory[r["graphics_name"]]
                suffix = r["graphics_name"].removeprefix("OBJ_EVENT_GFX_").lower()
                self.assertEqual(r["graphics_id"], source["graphics_id"])
                self.assertEqual(r["source_asset_id"], source["source_asset_id"])
                self.assertEqual(source["status"], "oras_public_exact")
                self.assertEqual(r["presentation_id"], f"npc.{suffix}")
                self.assertEqual(r["model_id"], f"character.{suffix}.oras_overworld")
                self.assertEqual(r["native_skeleton_id"], f"skeleton.{suffix}.oras_overworld")
                self.assertEqual(
                    r["provenance_id"],
                    f"provenance.r6.npc.oras.models_resource.{r['source_asset_id']}",
                )
                self.assertEqual(r["source_family"], "oras")

    def test_manifest_consumes_mapping_without_schema_extension(self) -> None:
        fields = (
            "presentation_id", "source_family", "skeleton_family", "model_id",
            "animation_set_id", "fallback_id", "provenance_id", "source_sha256",
            "normalized_sha256",
        )
        for r in self.records:
            with self.subTest(name=r["graphics_name"]):
                entry = self.entries[r["graphics_name"]]
                override = self.overrides["overrides"][r["graphics_name"]]
                self.assertEqual(entry["presentation_kind"], "human")
                for field in fields:
                    self.assertEqual(entry[field], r[field])
                    self.assertEqual(override[field], r[field])
                self.assertEqual(entry["scale"], 1.0)
                self.assertEqual(entry["ground_offset_cm"], 0.0)
                self.assertEqual(entry["yaw_offset_deg"], 0.0)
                self.assertEqual(entry["material_ids"], [])

    def test_source_selection_does_not_claim_runtime_readiness(self) -> None:
        expected = {
            "mapping_status": "verified_single_model_named_source",
            "skeleton_family": "special_human",
            "skeleton_family_status": "unassigned_special_human_sentinel",
            "native_skeleton_status": "skin_controller_present_topology_unverified",
            "retarget_compatibility_status": "unverified",
            "family_compatible_reuse_allowed": False,
            "normalization_status": "pending",
            "normalized_sha256": [],
            "animation_clip_status": "missing",
            "animation_set_id": "fallback.human",
            "unreal_import_status": "untested",
            "runtime_status": "untested",
            "calibration_status": "placeholder_defaults_uncalibrated",
            "fallback_id": "fallback.human",
        }
        for r in self.records:
            for key, value in expected.items():
                with self.subTest(name=r["graphics_name"], field=key):
                    self.assertEqual(r[key], value)

    def test_source_gaps_and_ambiguous_packages_stay_on_placeholder(self) -> None:
        for name, finding in self.acquisition["identity_findings"].items():
            if finding["resolution"] == "source_gap" or "unresolved" in finding["resolution"]:
                with self.subTest(name=name):
                    self.assertNotIn(name, self.overrides["overrides"])
                    self.assertEqual(self.entries[name]["model_id"], "fallback.human")
                    self.assertEqual(self.entries[name]["source_family"], "project_placeholder")

    def test_order_uniqueness_and_full_identity_coverage_are_preserved(self) -> None:
        for field in ("graphics_id", "graphics_name", "presentation_id", "model_id", "provenance_id"):
            self.assertEqual(len({r[field] for r in self.records}), len(self.records))
        self.assertEqual(self.records, sorted(self.records, key=lambda r: (r["graphics_id"], r["graphics_name"])))
        self.assertEqual(self.manifest["identity_count"], 681)
        self.assertEqual(self.manifest["kind_counts"], {
            "human": 139, "pokemon_overworld": 447, "special_object": 95,
        })
        for entry in self.manifest["entries"]:
            if entry["presentation_kind"] != "human":
                self.assertEqual(entry["source_family"], "project_placeholder")

    def test_input_evidence_is_tied_to_current_source_records(self) -> None:
        evidence = self.mapping["source_evidence"]
        for field, filename in (("inventory_sha256", "npc_source_inventory.json"),
                                ("acquisition_sha256", "npc_asset_acquisition.json")):
            self.assertEqual(evidence[field], hashlib.sha256((DATA / filename).read_bytes()).hexdigest())
        for field in ("archives_rehashed", "archive_integrity_verified", "submodels_hashed", "private_provenance_records_verified"):
            self.assertEqual(evidence[field], 16)
        self.assertEqual(self.mapping["provenance_id_policy"],
                         "new_logical_alias_for_verified_existing_record_by_source_asset_id")

    def test_public_mapping_has_no_private_paths_or_payloads(self) -> None:
        serialized = json.dumps(self.mapping, sort_keys=True)
        for forbidden in ("pokemon-emerald-remastered-assets", "R6/NPCs/source/",
                          "Provenance/R6/NPC/", "/home/", "/Users/", "/tmp/",
                          "/mnt/", "/workspace/", "C:\\", "token=", "BEGIN "):
            self.assertNotIn(forbidden, serialized)
        for r in self.records:
            self.assertEqual(Path(r["source_submodel_file"]).name, r["source_submodel_file"])
            for field in ("presentation_id", "model_id", "native_skeleton_id", "provenance_id"):
                self.assertRegex(r[field], r"^[a-z0-9][a-z0-9_.-]*$")

    def test_unknown_identity_override_is_rejected(self) -> None:
        data = json.loads(OVERRIDES.read_text())
        data["overrides"]["OBJ_EVENT_GFX_UNDECLARED_NPC"] = {"model_id": "character.unknown"}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "overrides.json"
            path.write_text(json.dumps(data))
            with self.assertRaisesRegex(ManifestError, "unknown graphics identities"):
                build_manifest(VENDOR, path)

    def test_duplicate_named_presentation_is_rejected(self) -> None:
        data = json.loads(OVERRIDES.read_text())
        data["overrides"]["OBJ_EVENT_GFX_ROXANNE"]["presentation_id"] = "npc.prof_birch"
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "overrides.json"
            path.write_text(json.dumps(data))
            with self.assertRaisesRegex(ManifestError, "duplicate presentation_id"):
                build_manifest(VENDOR, path)


if __name__ == "__main__":
    unittest.main()
