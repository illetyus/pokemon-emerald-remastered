import json
import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from world_fingerprint import (  # noqa: E402
    canonical_json_bytes,
    fingerprint_json,
    fingerprint_world_package,
)


class WorldFingerprintTests(unittest.TestCase):
    def test_canonical_json_is_key_order_independent(self):
        a = {"b": 2, "a": {"y": 2, "x": 1}}
        b = {"a": {"x": 1, "y": 2}, "b": 2}

        self.assertEqual(canonical_json_bytes(a), canonical_json_bytes(b))
        self.assertEqual(fingerprint_json(a), fingerprint_json(b))

    def test_content_change_changes_fingerprint(self):
        self.assertNotEqual(
            fingerprint_json({"value": 1}),
            fingerprint_json({"value": 2}),
        )

    def test_world_package_ignores_provenance_timestamp_but_hashes_content(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "maps").mkdir()
            (root / "manifest.json").write_text(
                json.dumps({"schema_version": 1, "maps": [{"file": "maps/A.json"}]}),
                encoding="utf-8",
            )
            (root / "maps/A.json").write_text(
                json.dumps({"schema_version": 1, "map": {"id": "MAP_A"}}),
                encoding="utf-8",
            )
            (root / "provenance.json").write_text(
                json.dumps(
                    {
                        "source_commit": "abc",
                        "generated_at": "2026-10-03T00:00:00Z",
                    }
                ),
                encoding="utf-8",
            )

            first = fingerprint_world_package(root)
            provenance = json.loads(
                (root / "provenance.json").read_text(encoding="utf-8")
            )
            provenance["generated_at"] = "2030-01-01T00:00:00Z"
            (root / "provenance.json").write_text(
                json.dumps(provenance),
                encoding="utf-8",
            )
            second = fingerprint_world_package(root)

            self.assertEqual(first["package_sha256"], second["package_sha256"])

            map_doc = json.loads(
                (root / "maps/A.json").read_text(encoding="utf-8")
            )
            map_doc["map"]["id"] = "MAP_B"
            (root / "maps/A.json").write_text(
                json.dumps(map_doc),
                encoding="utf-8",
            )
            third = fingerprint_world_package(root)
            self.assertNotEqual(first["package_sha256"], third["package_sha256"])

    def test_package_fingerprint_is_relative_path_stable(self):
        with tempfile.TemporaryDirectory() as first_temp, tempfile.TemporaryDirectory() as second_temp:
            first = Path(first_temp)
            second = Path(second_temp)
            for root in (first, second):
                (root / "maps").mkdir()
                (root / "manifest.json").write_text(
                    json.dumps({"schema_version": 1}),
                    encoding="utf-8",
                )
                (root / "maps/A.json").write_text(
                    json.dumps({"a": 1}),
                    encoding="utf-8",
                )

            self.assertEqual(
                fingerprint_world_package(first)["package_sha256"],
                fingerprint_world_package(second)["package_sha256"],
            )


if __name__ == "__main__":
    unittest.main()
