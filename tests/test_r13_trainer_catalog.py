import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor" / "vanillaplus"
GENERATOR = ROOT / "tools" / "build_r13_trainer_catalog.py"


class R13TrainerCatalogTest(unittest.TestCase):
    def test_catalog_covers_pinned_trainer_data(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            catalog = directory / "emerald_trainer_catalog.inc"
            report_path = directory / "trainer_catalog_report.json"
            subprocess.run(
                [
                    sys.executable,
                    str(GENERATOR),
                    str(VENDOR),
                    str(catalog),
                    str(report_path),
                ],
                check=True,
            )

            report = json.loads(report_path.read_text(encoding="utf-8"))
            self.assertEqual(report["trainer_entry_count"], 857)
            self.assertEqual(report["max_trainer_id"], 856)
            self.assertEqual(report["party_array_count"], 856)
            self.assertEqual(
                report["party_variant_counts"],
                {
                    "TrainerMonNoItemDefaultMoves": 672,
                    "TrainerMonNoItemCustomMoves": 87,
                    "TrainerMonItemDefaultMoves": 31,
                    "TrainerMonItemCustomMoves": 66,
                },
            )

            text = catalog.read_text(encoding="utf-8")
            self.assertIn("/* TRAINER_SAWYER_1 */", text)
            self.assertIn("/* TRAINER_FELIX */", text)
            self.assertIn("/* TRAINER_GABBY_AND_TY_1 */", text)
            self.assertIn('"SAWYER"', text)
            self.assertIn('"GABBY & TY"', text)


if __name__ == "__main__":
    unittest.main()
