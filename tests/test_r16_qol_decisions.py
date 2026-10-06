import json
import pathlib
import unittest
from collections import Counter

ROOT = pathlib.Path(__file__).resolve().parents[1]
DECISIONS_PATH = ROOT / "data" / "r16" / "qol_decisions.json"

EXPECTED_IDS = [f"QOL-{index:03d}" for index in range(1, 49)]
EXPECTED_COUNTS = {
    "ACCEPTED": 22,
    "ALREADY_COVERED": 18,
    "REJECTED": 8,
}
ALLOWED_STATUSES = set(EXPECTED_COUNTS)


class R16QolDecisionAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(DECISIONS_PATH.read_text(encoding="utf-8"))
        cls.decisions = cls.payload["decisions"]

    def test_manifest_identity_and_counts(self):
        self.assertEqual(self.payload["schema_version"], 1)
        self.assertEqual(self.payload["phase"], "R16")
        self.assertEqual(
            self.payload["source"],
            "vendor/vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c",
        )
        self.assertEqual(len(self.decisions), 48)
        self.assertEqual(
            self.payload["classification"]["expected_counts"], EXPECTED_COUNTS
        )
        self.assertEqual(
            set(self.payload["classification"]["allowed_statuses"]),
            ALLOWED_STATUSES,
        )

    def test_ids_are_complete_unique_and_ordered(self):
        ids = [entry["id"] for entry in self.decisions]
        self.assertEqual(ids, EXPECTED_IDS)
        self.assertEqual(len(ids), len(set(ids)))

    def test_only_three_statuses_and_exact_distribution(self):
        statuses = [entry["status"] for entry in self.decisions]
        self.assertTrue(set(statuses) <= ALLOWED_STATUSES)
        self.assertEqual(Counter(statuses), Counter(EXPECTED_COUNTS))

    def test_every_decision_is_explainable_and_regression_owned(self):
        required_text = (
            "feature",
            "stock_behavior",
            "vanillaplus_behavior",
            "owner",
        )
        for entry in self.decisions:
            with self.subTest(entry=entry["id"]):
                for key in required_text:
                    self.assertIsInstance(entry[key], str)
                    self.assertTrue(entry[key].strip(), f"{entry['id']} missing {key}")
                self.assertIsInstance(entry["source_evidence"], list)
                self.assertTrue(entry["source_evidence"])
                self.assertTrue(all(isinstance(x, str) and x for x in entry["source_evidence"]))
                self.assertIsInstance(entry["regression"], list)
                self.assertTrue(entry["regression"])
                self.assertTrue(all(isinstance(x, str) and x for x in entry["regression"]))

    def test_accepted_features_are_r16_owned(self):
        accepted = [e for e in self.decisions if e["status"] == "ACCEPTED"]
        self.assertEqual(len(accepted), 22)
        for entry in accepted:
            with self.subTest(entry=entry["id"]):
                self.assertEqual(entry["owner"], "R16")
                self.assertTrue(
                    any(test.startswith("r16_") for test in entry["regression"]),
                    f"{entry['id']} has no R16 regression owner",
                )

    def test_non_r16_features_are_not_silently_owned_by_r16(self):
        for entry in self.decisions:
            if entry["status"] != "ACCEPTED":
                with self.subTest(entry=entry["id"]):
                    self.assertNotEqual(entry["owner"], "R16")

    def test_fishing_contract_records_real_source_behavior(self):
        fishing = next(e for e in self.decisions if e["id"] == "QOL-013")
        behavior = fishing["vanillaplus_behavior"]
        self.assertIn("90-frame", behavior)
        self.assertIn("Old 1", behavior)
        self.assertIn("Good 1-3", behavior)
        self.assertIn("Super 1-6", behavior)
        self.assertIn("optional repeat chances are zero", behavior)


if __name__ == "__main__":
    unittest.main()
