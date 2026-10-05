#!/usr/bin/env python3
"""R10 canonical quest catalog/source audit regression."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "data" / "r10" / "quest_catalog.json"
GENERATOR = ROOT / "tools" / "build_r10_quest_catalog.py"
GENERATED = ROOT / "core" / "src" / "emerald_quest_catalog.inc"
VENDOR = ROOT / "vendor" / "vanillaplus"


class R10QuestCatalogTest(unittest.TestCase):
    def test_catalog_is_complete_and_sequential(self) -> None:
        payload = json.loads(CATALOG.read_text(encoding="utf-8"))
        objectives = payload["objectives"]

        self.assertEqual(len(objectives), 32)
        self.assertEqual(
            [objective["id"] for objective in objectives],
            list(range(1, 33)),
        )
        self.assertEqual(
            len({objective["key"] for objective in objectives}),
            32,
        )

        object_targets = [
            objective
            for objective in objectives
            if objective["target"]["type"] == "object_event"
        ]
        self.assertEqual(len(object_targets), 17)

        for objective in object_targets:
            target = objective["target"]
            self.assertGreater(target["local_id"], 0)
            self.assertTrue(target["expected_script"])

    def test_catalog_is_derived_not_persisted(self) -> None:
        save_h = (ROOT / "core" / "include" / "remaster" / "emerald_save.h").read_text(
            encoding="utf-8"
        )
        state_h = (ROOT / "core" / "include" / "remaster" / "emerald_state.h").read_text(
            encoding="utf-8"
        )

        for forbidden in (
            "currentQuest",
            "questState",
            "questObjective",
            "activeQuest",
        ):
            self.assertNotIn(forbidden.lower(), save_h.lower())
            self.assertNotIn(forbidden.lower(), state_h.lower())

    def test_generated_catalog_matches_pinned_vanillaplus(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "emerald_quest_catalog.inc"
            subprocess.run(
                [
                    sys.executable,
                    str(GENERATOR),
                    str(VENDOR),
                    str(CATALOG),
                    str(output),
                ],
                cwd=ROOT,
                check=True,
            )

            self.assertEqual(
                output.read_bytes(),
                GENERATED.read_bytes(),
                "checked-in R10 catalog is stale relative to pinned Vanilla+",
            )


if __name__ == "__main__":
    unittest.main()
