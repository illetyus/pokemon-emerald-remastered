from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from tools.build_r6_character_manifest import (
    ManifestError,
    build_manifest,
    resolve_entry,
)

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor" / "vanillaplus"
OVERRIDES = ROOT / "data" / "r6" / "character_presentation_overrides.json"


class R6CharacterManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.manifest = build_manifest(VENDOR, OVERRIDES)
        cls.by_name = {
            entry["graphics_name"]: entry
            for entry in cls.manifest["entries"]
        }

    def test_pinned_vanillaplus_identity_set_is_complete(self) -> None:
        self.assertEqual(self.manifest["schema_version"], 1)
        self.assertFalse(
            self.manifest["source_configuration"]["pokemon_expansion"]
        )
        self.assertEqual(self.manifest["identity_count"], 681)
        self.assertEqual(self.manifest["min_graphics_id"], 0)
        self.assertEqual(self.manifest["max_graphics_id"], 680)
        self.assertEqual(
            [entry["graphics_id"] for entry in self.manifest["entries"]],
            list(range(681)),
        )

    def test_known_human_pokemon_and_object_identities_classify(self) -> None:
        expected = {
            "OBJ_EVENT_GFX_BRENDAN_NORMAL": (0, "human"),
            "OBJ_EVENT_GFX_PROF_BIRCH": (64, "human"),
            "OBJ_EVENT_GFX_MAY_NORMAL": (89, "human"),
            "OBJ_EVENT_GFX_RAYQUAZA_STILL": (41, "pokemon_overworld"),
            "OBJ_EVENT_GFX_BULBASAUR": (239, "pokemon_overworld"),
            "OBJ_EVENT_GFX_TREECKO": (515, "pokemon_overworld"),
            "OBJ_EVENT_GFX_ITEM_BALL": (59, "special_object"),
            "OBJ_EVENT_GFX_GREAT_BALL": (490, "special_object"),
            "OBJ_EVENT_GFX_EXPANDING_SPARKLE": (680, "special_object"),
        }
        for name, (graphics_id, kind) in expected.items():
            with self.subTest(name=name):
                entry = self.by_name[name]
                self.assertEqual(entry["graphics_id"], graphics_id)
                self.assertEqual(entry["presentation_kind"], kind)

    def test_every_identity_has_explicit_fallback(self) -> None:
        for entry in self.manifest["entries"]:
            self.assertTrue(entry["presentation_id"])
            self.assertTrue(entry["fallback_id"])
            self.assertTrue(entry["model_id"])
            self.assertEqual(entry["schema_version"], 1)
            self.assertNotIn("/", entry["model_id"])
            self.assertNotIn("\\", entry["model_id"])

    def test_resolver_uses_authoritative_numeric_identity(self) -> None:
        self.assertEqual(
            resolve_entry(self.manifest, 64)["graphics_name"],
            "OBJ_EVENT_GFX_PROF_BIRCH",
        )
        self.assertIsNone(resolve_entry(self.manifest, 9999))

    def test_manifest_is_deterministic(self) -> None:
        first = json.dumps(
            self.manifest,
            sort_keys=True,
            separators=(",", ":"),
        )
        second = json.dumps(
            build_manifest(VENDOR, OVERRIDES),
            sort_keys=True,
            separators=(",", ":"),
        )
        self.assertEqual(first, second)
        self.assertEqual(len(self.manifest["content_sha256"]), 64)
        int(self.manifest["content_sha256"], 16)

    def test_public_overrides_can_bind_logical_asset_ids(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "overrides.json"
            path.write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "overrides": {
                            "OBJ_EVENT_GFX_BRENDAN_NORMAL": {
                                "source_family": "oras",
                                "source_sha256": ["a" * 64],
                                "provenance_id": "provenance.test.fixture",
                                "skeleton_family": "player_male",
                                "model_id": "character.brendan",
                                "animation_set_id": "human.player_male",
                            }
                        },
                    }
                ),
                encoding="utf-8",
            )
            manifest = build_manifest(VENDOR, path)
            entry = resolve_entry(manifest, 0)
            self.assertEqual(entry["source_family"], "oras")
            self.assertEqual(entry["model_id"], "character.brendan")
            self.assertEqual(entry["fallback_id"], "fallback.human")
            serialized = json.dumps(manifest, sort_keys=True)
            self.assertNotIn(str(Path(tmp)), serialized)
            self.assertEqual(
                manifest["source_files"][2]["path"],
                "data/r6/character_presentation_overrides.json",
            )

    def test_machine_paths_are_rejected_as_logical_ids(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "overrides.json"
            path.write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "overrides": {
                            "OBJ_EVENT_GFX_BRENDAN_NORMAL": {
                                "model_id": "/tmp/brendan.fbx"
                            }
                        },
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaises(ManifestError):
                build_manifest(VENDOR, path)


    def test_player_state_overrides_bind_i2c_contract(self) -> None:
        states_path = ROOT / "data" / "r6" / "player_presentation_states.json"
        state_data = json.loads(states_path.read_text(encoding="utf-8"))
        override_data = json.loads(OVERRIDES.read_text(encoding="utf-8"))["overrides"]

        self.assertEqual(state_data["state_count"], 18)
        for state in state_data["records"]:
            with self.subTest(name=state["graphics_name"]):
                name = state["graphics_name"]
                override = override_data[name]
                entry = self.by_name[name]

                self.assertEqual(override["presentation_id"], state["presentation_id"])
                self.assertEqual(override["source_family"], state["source_family"])
                self.assertEqual(override["skeleton_family"], state["skeleton_family"])
                self.assertEqual(override["model_id"], state["base_model_id"])
                self.assertEqual(
                    override["animation_set_id"],
                    state["animation_semantic"],
                )
                self.assertEqual(
                    override["source_sha256"],
                    [state["source_archive_sha256"]],
                )
                self.assertEqual(override["normalized_sha256"], [])

                for field in (
                    "presentation_id",
                    "source_family",
                    "skeleton_family",
                    "model_id",
                    "animation_set_id",
                    "provenance_id",
                ):
                    self.assertEqual(entry[field], override[field])
                self.assertEqual(entry["source_sha256"], override["source_sha256"])
                self.assertEqual(entry["normalized_sha256"], [])
                self.assertEqual(entry["fallback_id"], "fallback.human")



if __name__ == "__main__":
    unittest.main()
