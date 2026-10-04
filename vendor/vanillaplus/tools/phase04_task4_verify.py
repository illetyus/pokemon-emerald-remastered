#!/usr/bin/env python3
"""Focused RED/GREEN checks for Phase 4 direct field interactions."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def fail(message: str) -> None:
    errors.append(message)


def label_block(text: str, label: str, next_label: str) -> str:
    start = text.find(label + "::")
    if start < 0:
        return ""
    end = text.find(next_label + "::", start + len(label) + 2)
    if end < 0:
        end = len(text)
    return text[start:end]


def function_block(text: str, signature: str) -> str:
    search_from = 0
    while True:
        start = text.find(signature, search_from)
        if start < 0:
            return ""
        brace = text.find("{", start)
        semicolon = text.find(";", start)
        if brace >= 0 and (semicolon < 0 or brace < semicolon):
            break
        search_from = start + len(signature)
    depth = 0
    for pos in range(brace, len(text)):
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
            if depth == 0:
                return text[start:pos + 1]
    return ""


field_scripts = read("data/scripts/field_move_scripts.inc")
surf_script = read("data/scripts/surf.inc")
field_control = read("src/field_control_avatar.c")
specials = read("data/specials.inc")

for label, next_label, item, move in [
    ("EventScript_CutTree", "EventScript_UseCut", "ITEM_HM01_CUT", "MOVE_CUT"),
    ("EventScript_RockSmash", "EventScript_UseRockSmash", "ITEM_HM06_ROCK_SMASH", "MOVE_ROCK_SMASH"),
    ("EventScript_StrengthBoulder", "EventScript_UseStrength", "ITEM_HM04_STRENGTH", "MOVE_STRENGTH"),
]:
    block = label_block(field_scripts, label, next_label)
    if not block:
        fail(f"field_move_scripts: missing {label}")
        continue
    if f"checkitem {item}" not in block:
        fail(f"{label}: missing bag ownership check for {item}")
    if f"checkpartymove {move}" in block:
        fail(f"{label}: still requires learned move {move}")
    if "specialvar VAR_RESULT, GetVanillaPlusHmFieldActor" not in block:
        fail(f"{label}: does not select the shared safe HM field actor")
    if "goto_if_eq VAR_RESULT, PARTY_SIZE" not in block:
        fail(f"{label}: does not reject a missing visual actor")

# Preserve side effects that are easy to accidentally lose while changing auth.
for needle in [
    "setflag FLAG_SYS_USE_STRENGTH",
    "TryUpdateRusturfTunnelState",
    "RockSmashWildEncounter",
]:
    if needle not in field_scripts:
        fail(f"field_move_scripts: preserved side effect missing {needle}")

surf = label_block(surf_script, "EventScript_UseSurf", "EventScript_ReleaseUseSurf")
if "checkpartymove MOVE_SURF" in surf_script:
    fail("surf.inc: Surf still requires a learned move")
if "specialvar VAR_RESULT, GetVanillaPlusHmFieldActor" not in surf:
    fail("surf.inc: Surf does not select the shared safe HM field actor")
if "goto_if_eq VAR_RESULT, PARTY_SIZE" not in surf:
    fail("surf.inc: Surf does not reject a missing visual actor")
if "dofieldeffect FLDEFF_USE_SURF" not in surf_script:
    fail("surf.inc: existing Surf field effect flow was lost")

water = function_block(field_control, "static const u8 *GetInteractedWaterScript")
if not water:
    fail("field_control_avatar.c: GetInteractedWaterScript missing")
else:
    if "HasVanillaPlusHmAccess(ITEM_HM03_SURF)" not in water:
        fail("GetInteractedWaterScript: Surf must require shared HM03 item+badge access")
    if "PartyHasMonWithSurf()" in water:
        fail("GetInteractedWaterScript: Surf still depends on a learned move")
    if "IsPlayerFacingSurfableFishableWater()" not in water:
        fail("GetInteractedWaterScript: Surfable-water context check was lost")

if "def_special GetVanillaPlusHmFieldActor" not in specials:
    fail("data/specials.inc: safe HM actor is not exposed to field scripts")

if errors:
    print("Phase 4 Task 4 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Phase 4 Task 4 verification PASSED")
