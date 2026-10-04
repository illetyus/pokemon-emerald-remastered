#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 3 daily/field QoL."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def fail(message: str) -> None:
    errors.append(message)

# Running: B should work without requiring the Running Shoes story flag.
player = read("src/field_player_avatar.c")
if "&& FlagGet(FLAG_SYS_B_DASH)" in player:
    fail("src/field_player_avatar.c: running is still gated by FLAG_SYS_B_DASH")

# Repel: preserve the existing quick re-use prompt/cycle behavior.
repel = read("data/scripts/repel.inc")
for needle in [
    "callnative Cycle_Through_Repels",
    "Text_RepelWoreOff_UseAnother",
    "removeitem VAR_REPEL_LAST_USED, 1",
]:
    if needle not in repel:
        fail(f"data/scripts/repel.inc: repel quick re-use behavior missing {needle!r}")

# Fishing: one reaction round, with a much wider response window.
for needle in [
    "[OLD_ROD]   = 90",
    "[GOOD_ROD]  = 90",
    "[SUPER_ROD] = 90",
    "[OLD_ROD]   = {0, 0}",
    "[GOOD_ROD]  = {0, 0}",
    "[SUPER_ROD] = {0, 0}",
]:
    if needle not in player:
        fail(f"src/field_player_avatar.c: easier fishing contract missing {needle!r}")

# Low-HP warning sound should never be started.
battle_sfx = read("src/battle_gfx_sfx_util.c")
if "PlaySE(SE_LOW_HEALTH);" in battle_sfx:
    fail("src/battle_gfx_sfx_util.c: low-HP warning beep is still started")

# Field poison may damage but must stop at 1 HP.
poison = read("src/field_poison.c")
if "if (hp > 1)" not in poison or "hp--;" not in poison:
    fail("src/field_poison.c: poison must decrement only above 1 HP")
if "--hp == 0" in poison:
    fail("src/field_poison.c: field poison can still reduce a Pokemon to 0 HP")

# Flash should fully illuminate dark maps after use.
flash = read("data/scripts/flash.inc")
if "setflashlevel 0" not in flash:
    fail("data/scripts/flash.inc: Flash must set full visibility (level 0)")

# Bike: while stationary on either bike, R toggles Mach <-> Acro directly.
bike = read("src/bike.c")
for needle in [
    "TrySwitchBikeType",
    "newKeys & R_BUTTON",
    "PLAYER_AVATAR_FLAG_MACH_BIKE",
    "PLAYER_AVATAR_FLAG_ACRO_BIKE",
]:
    if needle not in bike:
        fail(f"src/bike.c: fast bike switching contract missing {needle!r}")
if bike.count("if (TrySwitchBikeType(newKeys))") < 2:
    fail("src/bike.c: both Mach and Acro movement paths must allow fast switching")

# Keep the field-move Pokemon animation, with Phase 3 establishing an upper bound of 8 frames.
# Later speed-focused phases may shorten it further without invalidating Phase 3.
field_effect = read("src/field_effect.c")
hold_match = re.search(r"sprite->sOnscreenTimer\s*=\s*(\d+);", field_effect)
if not hold_match or int(hold_match.group(1)) > 8:
    fail("src/field_effect.c: field-move Pokemon hold time must remain 8 frames or shorter")

# Exact build-marker ownership moved to Phase 4. Phase 3 remains
# version-agnostic so later synchronized marker bumps do not invalidate it.

workflow = read(".github/workflows/build.yml")
for verifier in [
    "tools/phase_version_contract_test.py",
    "tools/vanillaplus_phase01_verify.py",
    "tools/vanillaplus_phase02_verify.py",
    "tools/vanillaplus_phase03_verify.py",
]:
    if f"python3 {verifier}" not in workflow:
        fail(f".github/workflows/build.yml: verifier missing {verifier}")

if errors:
    print("Vanilla+ Phase 3 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Vanilla+ Phase 3 verification PASSED")
