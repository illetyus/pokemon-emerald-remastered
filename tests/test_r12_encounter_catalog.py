#!/usr/bin/env python3
"""R12 pinned Vanilla+/Phase9 encounter catalog audit."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor" / "vanillaplus"
GENERATOR = ROOT / "tools" / "build_r12_encounter_catalog.py"


class R12EncounterCatalogTest(unittest.TestCase):
    def _generate(self, directory: Path) -> tuple[Path, Path]:
        catalog = directory / "emerald_encounter_catalog.inc"
        report = directory / "encounter_catalog_report.json"
        subprocess.run(
            [
                sys.executable,
                str(GENERATOR),
                str(VENDOR),
                str(catalog),
                str(report),
            ],
            cwd=ROOT,
            check=True,
        )
        return catalog, report

    def test_generator_is_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as a_tmp, tempfile.TemporaryDirectory() as b_tmp:
            a_catalog, a_report = self._generate(Path(a_tmp))
            b_catalog, b_report = self._generate(Path(b_tmp))
            self.assertEqual(a_catalog.read_bytes(), b_catalog.read_bytes())
            self.assertEqual(a_report.read_bytes(), b_report.read_bytes())

    def test_source_coverage_and_phase9_contract(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            _, report_path = self._generate(Path(tmp))
            report = json.loads(report_path.read_text(encoding="utf-8"))

        self.assertEqual(report["map_encounter_count"], 124)
        self.assertEqual(report["land_map_count"], 95)
        self.assertEqual(report["water_map_count"], 55)
        self.assertEqual(report["rock_smash_map_count"], 6)
        self.assertEqual(report["fishing_map_count"], 53)

        self.assertEqual(report["battle_pyramid_table_count"], 7)
        self.assertEqual(report["battle_pike_table_count"], 4)

        self.assertEqual(report["national_dex_habitat_count"], 386)
        self.assertEqual(report["phase9_level_delta"], 15)
        self.assertEqual(report["phase9_level_min"], 2)
        self.assertEqual(report["phase9_level_max"], 100)
        self.assertEqual(report["phase9_max_local_pool"], 64)
        self.assertTrue(report["new_mauville_mr_mime_guarantee"])

        self.assertEqual(report["metatile_behavior_count"], 0xF0)
        self.assertGreaterEqual(report["learnset_species_count"], 386)
        self.assertEqual(report["missing_map_constants"], [])
        self.assertEqual(report["missing_region_sections"], [])
        self.assertEqual(report["missing_anchor_species"], [])

    def test_phase9_intentionally_owns_species_and_level_selection(self) -> None:
        source = (VENDOR / "src" / "wild_encounter.c").read_text(encoding="utf-8")
        start = source.index("static bool8 TryGeneratePhase9WildMon")
        end = source.index("static u16 GeneratePhase9FishingWildMon", start)
        block = source[start:end]

        self.assertIn("Phase9ChooseWildSpecies", block)
        self.assertIn("Phase9ChooseWildLevel", block)
        self.assertNotIn("TryGetAbilityInfluencedWildMonIndex", block)
        self.assertNotIn("ChooseWildMonLevel", block)

    def test_current_phase9_fishing_does_not_reactivate_legacy_feebas_spots(self) -> None:
        source = (VENDOR / "src" / "wild_encounter.c").read_text(encoding="utf-8")
        self.assertEqual(
            source.count("CheckFeebas("),
            1,
            "current pinned Vanilla+ should contain only the unused CheckFeebas definition",
        )
        self.assertIn(
            "GeneratePhase9FishingWildMon",
            source,
        )
        self.assertIn(
            "Phase9ChooseWildSpecies",
            (VENDOR / "src" / "phase9_wild_ecosystem.c").read_text(encoding="utf-8"),
        )


if __name__ == "__main__":
    unittest.main()
