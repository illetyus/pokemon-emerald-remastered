from __future__ import annotations

import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAPS = ROOT / "vendor/vanillaplus/data/maps"
RUNTIME_OBJECT_CAPACITY = 16


class R4RuntimeObjectCapacityTests(unittest.TestCase):
    def test_all_vanillaplus_maps_fit_runtime_object_capacity(self):
        offenders: list[tuple[str, int]] = []
        maximum = 0
        maximum_maps: list[str] = []

        for path in sorted(MAPS.glob("*/map.json")):
            document = json.loads(path.read_text(encoding="utf-8"))
            count = len(document.get("object_events", []))

            if count > maximum:
                maximum = count
                maximum_maps = [document.get("name", path.parent.name)]
            elif count == maximum:
                maximum_maps.append(document.get("name", path.parent.name))

            if count > RUNTIME_OBJECT_CAPACITY:
                offenders.append(
                    (document.get("name", path.parent.name), count)
                )

        self.assertEqual(
            offenders,
            [],
            f"maps exceed {RUNTIME_OBJECT_CAPACITY} runtime object slots: "
            f"{offenders}",
        )
        self.assertGreater(maximum, 0)

        print(
            "R4 runtime object capacity: "
            f"max={maximum} maps={','.join(maximum_maps[:10])}"
        )


if __name__ == "__main__":
    unittest.main()
