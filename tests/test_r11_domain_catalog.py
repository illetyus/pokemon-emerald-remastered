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
GENERATED = ROOT / "core" / "src" / "emerald_domain_catalog.inc"
REPORT = ROOT / "data" / "r11" / "domain_catalog_report.json"


class R11DomainCatalogTest(unittest.TestCase):
    def test_generated_catalog_matches_pinned_source(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            generated = Path(tmp) / "emerald_domain_catalog.inc"
            report = Path(tmp) / "domain_catalog_report.json"
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
            self.assertEqual(
                generated.read_bytes(),
                GENERATED.read_bytes(),
                "checked-in R11 domain catalog is stale",
            )
            self.assertEqual(
                report.read_bytes(),
                REPORT.read_bytes(),
                "checked-in R11 audit report is stale",
            )

    def test_catalog_counts_and_source_coverage(self) -> None:
        report = json.loads(REPORT.read_text(encoding="utf-8"))

        self.assertEqual(report["species_internal_count"], 412)
        self.assertEqual(report["move_count"], 355)
        self.assertEqual(report["item_count"], 377)
        self.assertEqual(report["evolution_slots_per_species"], 5)

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


if __name__ == "__main__":
    unittest.main()
