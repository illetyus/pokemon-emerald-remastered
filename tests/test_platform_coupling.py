import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from scan_platform_coupling import scan  # noqa: E402

class PlatformCouplingTests(unittest.TestCase):
    def test_detects_gba_accesses(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "src").mkdir()
            (root / "src/test.c").write_text(
                "void f(void) { REG_DISPCNT = 0; DmaCopy16(3, VRAM, PLTT, 4); }"
            )
            report = scan(root)
            self.assertEqual(report["files_with_platform_coupling"], 1)
            hits = report["files"][0]["hits"]
            self.assertGreater(hits["hardware_register"], 0)
            self.assertGreater(hits["dma"], 0)
            self.assertGreater(hits["vram"], 0)
            self.assertGreater(hits["palette"], 0)

if __name__ == "__main__":
    unittest.main()
