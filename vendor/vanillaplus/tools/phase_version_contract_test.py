#!/usr/bin/env python3
"""Regression test for Vanilla+ verifier version ownership.

Older phase verifiers must accept later synchronized build markers; only the
current phase owns the exact marker. This prevents version bumps from breaking
already-completed phase verification.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
phase01 = (ROOT / "tools/vanillaplus_phase01_verify.py").read_text(encoding="utf-8")
phase02 = (ROOT / "tools/vanillaplus_phase02_verify.py").read_text(encoding="utf-8")
phase03 = (ROOT / "tools/vanillaplus_phase03_verify.py").read_text(encoding="utf-8")
phase04 = (ROOT / "tools/vanillaplus_phase04_verify.py").read_text(encoding="utf-8")
phase05 = (ROOT / "tools/vanillaplus_phase05_verify.py").read_text(encoding="utf-8")
phase06 = (ROOT / "tools/vanillaplus_phase06_verify.py").read_text(encoding="utf-8")
phase07 = (ROOT / "tools/vanillaplus_phase07_verify.py").read_text(encoding="utf-8")
phase08 = (ROOT / "tools/vanillaplus_phase08_verify.py").read_text(encoding="utf-8")
phase09 = (ROOT / "tools/vanillaplus_phase09_verify.py").read_text(encoding="utf-8")
phase10a = (ROOT / "tools/vanillaplus_phase10a_verify.py").read_text(encoding="utf-8")
pc_management = (ROOT / "tools/vanillaplus_pc_management_verify.py").read_text(encoding="utf-8")

assert '"ZUMRUT VP002"' not in phase01
assert '"ZUMRUT T002"' not in phase01
assert 'OYUNCU V+002")\'' not in phase01
assert "release_match" in phase01
assert "test_match" in phase01
assert "continue_match" in phase01
assert "len(versions) != 1" in phase01

assert '"ZUMRUT VP003"' not in phase02
assert '"ZUMRUT T003"' not in phase02
assert 'OYUNCU V+003' not in phase02

assert '"ZUMRUT VP004"' not in phase03
assert '"ZUMRUT T004"' not in phase03
assert 'OYUNCU V+004' not in phase03

assert '"ZUMRUT VP005"' not in phase04
assert '"ZUMRUT T005"' not in phase04
assert 'OYUNCU V+005' not in phase04

assert '"ZUMRUT VP006"' not in phase08
assert '"ZUMRUT T006"' not in phase08
assert 'OYUNCU V+006' not in phase08
assert "release_match" in phase08
assert "test_match" in phase08
assert "continue_match" in phase08

assert '"ZUMRUT VP007"' not in phase05
assert '"ZUMRUT T007"' not in phase05
assert 'OYUNCU V+007' not in phase05
assert "release_match" in phase05
assert "test_match" in phase05
assert "continue_match" in phase05

assert '"ZUMRUT VP015"' not in phase06
assert '"ZUMRUT T015"' not in phase06
assert 'OYUNCU V+015")' not in phase06
assert "release_match" in phase06
assert "test_match" in phase06
assert "continue_match" in phase06

assert '"ZUMRUT VP016"' not in phase07
assert '"ZUMRUT T016"' not in phase07
assert 'OYUNCU V+016' not in phase07
assert "release_match" in phase07
assert "test_match" in phase07
assert "continue_match" in phase07

assert '"ZUMRUT VP017"' not in phase09
assert '"ZUMRUT T017"' not in phase09
assert 'OYUNCU V+017' not in phase09
assert "release_match" in phase09
assert "test_match" in phase09
assert "continue_match" in phase09

assert '"ZUMRUT VP018"' not in phase10a
assert '"ZUMRUT T018"' not in phase10a
assert 'OYUNCU V+018' not in phase10a
assert "release_match" in phase10a
assert "test_match" in phase10a
assert "continue_match" in phase10a

assert 'ZUMRUT VP019' in pc_management
assert 'ZUMRUT T019' in pc_management
assert 'OYUNCU V+019' in pc_management

print("Phase version verifier ownership contract PASSED")
