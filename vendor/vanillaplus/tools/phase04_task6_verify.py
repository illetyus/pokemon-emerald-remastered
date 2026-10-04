#!/usr/bin/env python3
"""Focused RED/GREEN checks for Phase 4 release/CI ownership."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def fail(message: str) -> None:
    errors.append(message)


makefile = read("Makefile")
strings = read("src/strings.c")
workflow = read(".github/workflows/build.yml")
contract = read("tools/phase_version_contract_test.py")
phase03 = read("tools/vanillaplus_phase03_verify.py")
phase04 = read("tools/vanillaplus_phase04_verify.py")

for needle in ["ZUMRUT VP005", "ZUMRUT T005"]:
    if needle not in makefile:
        fail(f"Makefile: Phase 4 build marker missing {needle}")
if 'gText_ContinueMenuPlayer[] = _("OYUNCU V+005")' not in strings:
    fail("src/strings.c: Continue marker must be OYUNCU V+005")

if "python3 tools/vanillaplus_phase04_verify.py" not in workflow:
    fail("build.yml: Phase 4 verifier is not wired into permanent CI")

# Phase 4 must permanently include the focused Task 4/5 source contracts.
for helper in ["phase04_task4_verify.py", "phase04_task5_verify.py"]:
    if helper not in phase04:
        fail(f"vanillaplus_phase04_verify.py: focused contract not chained: {helper}")

# Exact version ownership moves from Phase 3 to Phase 4.
for forbidden in ["ZUMRUT VP004", "ZUMRUT T004", "OYUNCU V+004"]:
    if forbidden in phase03:
        fail(f"Phase 3 verifier still owns exact old marker {forbidden}")
for required in ["ZUMRUT VP005", "ZUMRUT T005", "OYUNCU V+005"]:
    if required not in phase04:
        fail(f"Phase 4 verifier does not own exact marker {required}")

if 'phase04 = (ROOT / "tools/vanillaplus_phase04_verify.py")' not in contract:
    fail("phase_version_contract_test.py: Phase 4 verifier is not included")
for required in ["ZUMRUT VP005", "ZUMRUT T005", "OYUNCU V+005"]:
    if required not in contract:
        fail(f"phase_version_contract_test.py: Phase 4 ownership assertion missing {required}")

if errors:
    print("Phase 4 Task 6 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Phase 4 Task 6 verification PASSED")
