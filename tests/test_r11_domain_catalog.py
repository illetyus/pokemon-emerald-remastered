#!/usr/bin/env python3
"""R11 deterministic Pokémon-domain catalog regression."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor" / "vanillaplus"
GENERATOR = ROOT / "tools" / "build_r11_domain_catalog.py"


class R11DomainCatalogTest(unittest.TestCase):
    def test_catalog_counts_and_source_coverage(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            generated = Path(tmp) / "emerald_domain_catalog.inc"
            report_path = Path(tmp) / "domain_catalog_report.json"

            subprocess.run(
                [
                    sys.executable,
                    str(GENERATOR),
                    str(VENDOR),
                    str(generated),
                    str(report_path),
                ],
                cwd=ROOT,
                check=True,
            )

            report = json.loads(report_path.read_text(encoding="utf-8"))

            self.assertGreater(generated.stat().st_size, 10000)
            self.assertEqual(report["species_internal_count"], 412)
            self.assertEqual(report["move_count"], 355)
            self.assertEqual(report["item_count"], 377)
            self.assertEqual(report["evolution_slots_per_species"], 5)
            self.assertGreater(report["level_up_move_entry_count"], 1000)
            self.assertEqual(report["level_up_learnset_species_count"], 412)

            self.assertEqual(report["species_entries_missing"], [])
            self.assertEqual(report["move_entries_missing"], [])
            self.assertEqual(report["item_entries_missing"], [])

            self.assertEqual(report["box_pokemon_bytes"], 80)
            self.assertEqual(report["party_pokemon_bytes"], 100)
            self.assertEqual(report["party_size"], 6)
            self.assertEqual(report["storage_box_count"], 14)
            self.assertEqual(report["storage_box_capacity"], 30)
            self.assertEqual(report["storage_box_payload_offset"], 4)
            self.assertEqual(report["storage_box_names_offset"], 0x8344)

            self.assertEqual(
                report["bag_pocket_capacities"],
                {
                    "items": 30,
                    "key_items": 30,
                    "poke_balls": 16,
                    "tm_hm": 64,
                    "berries": 46,
                },
            )

    def test_generation_is_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            outputs = []
            reports = []
            for index in range(2):
                generated = Path(tmp) / f"catalog-{index}.inc"
                report = Path(tmp) / f"report-{index}.json"
                subprocess.run(
                    [
                        sys.executable,
                        str(GENERATOR),
                        str(VENDOR),
                        str(generated),
                        str(report),
                    ],
                    cwd=ROOT,
                    check=True,
                )
                outputs.append(generated.read_bytes())
                reports.append(report.read_bytes())

            self.assertEqual(outputs[0], outputs[1])
            self.assertEqual(reports[0], reports[1])


if __name__ == "__main__":
    unittest.main()
