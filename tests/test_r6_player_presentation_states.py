from __future__ import annotations

import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data" / "r6" / "player_presentation_states.json"
EVENT_OBJECTS = ROOT / "vendor" / "vanillaplus" / "include" / "constants" / "event_objects.h"
PLAYER_AVATAR = ROOT / "vendor" / "vanillaplus" / "src" / "field_player_avatar.c"

PLAYER_NAME_RE = re.compile(
    r"^#define\s+(OBJ_EVENT_GFX_(?:BRENDAN|MAY)_[A-Z0-9_]+)\s+(\d+)\s*$",
    re.MULTILINE,
)


class R6PlayerPresentationStateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data = json.loads(DATA.read_text(encoding="utf-8"))
        cls.records = cls.data["records"]
        cls.by_name = {item["graphics_name"]: item for item in cls.records}

    def test_all_player_graphics_identities_are_classified(self) -> None:
        source = EVENT_OBJECTS.read_text(encoding="utf-8")
        source_pairs = {
            name: int(value)
            for name, value in PLAYER_NAME_RE.findall(source)
        }
        self.assertEqual(len(source_pairs), 18)
        self.assertEqual(
            source_pairs,
            {item["graphics_name"]: item["graphics_id"] for item in self.records},
        )

    def test_source_avatar_table_has_eight_states_per_gender(self) -> None:
        source = PLAYER_AVATAR.read_text(encoding="utf-8")
        expected_states = {
            "NORMAL",
            "MACH_BIKE",
            "ACRO_BIKE",
            "SURFING",
            "UNDERWATER",
            "FIELD_MOVE",
            "FISHING",
            "WATERING",
        }
        for state in expected_states:
            self.assertIn(
                f"[PLAYER_AVATAR_STATE_{state}]",
                source,
            )
        self.assertNotIn("PLAYER_AVATAR_STATE_DECORATING", source)

        for character in ("BRENDAN", "MAY"):
            for state in expected_states:
                self.assertIn(
                    f"OBJ_EVENT_GFX_{character}_{state}",
                    source,
                )

    def test_decorating_is_not_promoted_to_gameplay_state(self) -> None:
        for name in (
            "OBJ_EVENT_GFX_BRENDAN_DECORATING",
            "OBJ_EVENT_GFX_MAY_DECORATING",
        ):
            self.assertEqual(
                self.by_name[name]["activation_class"],
                "legacy_special_identity",
            )

    def test_base_models_and_skeleton_families_are_stable(self) -> None:
        for record in self.records:
            if record["character"] == "brendan":
                self.assertEqual(
                    record["base_model_id"],
                    "character.brendan.oras_overworld",
                )
                self.assertEqual(record["skeleton_family"], "player_male")
            else:
                self.assertEqual(
                    record["base_model_id"],
                    "character.may.oras_overworld",
                )
                self.assertEqual(record["skeleton_family"], "player_female")

    def test_no_state_claims_animation_clip_is_ready(self) -> None:
        self.assertTrue(
            all(
                item["animation_clip_status"] == "missing"
                for item in self.records
            )
        )
        self.assertTrue(
            all(
                item["runtime_selection_status"]
                == "declarative_only_until_authoritative_avatar_state_is_exposed"
                for item in self.records
            )
        )

    def test_logical_ids_do_not_contain_paths(self) -> None:
        for item in self.records:
            for field in (
                "presentation_id",
                "base_model_id",
                "skeleton_family",
                "animation_semantic",
            ):
                self.assertNotIn("/", item[field])
                self.assertNotIn("\\", item[field])


if __name__ == "__main__":
    unittest.main()
