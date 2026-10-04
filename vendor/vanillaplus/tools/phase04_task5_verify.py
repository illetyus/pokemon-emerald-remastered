#!/usr/bin/env python3
"""Focused RED/GREEN checks for Phase 4 Dive/Waterfall/Fly/Flash paths."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def fail(message: str) -> None:
    errors.append(message)


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


def label_block(text: str, label: str, next_label: str) -> str:
    start = text.find(label + "::")
    if start < 0:
        return ""
    end = text.find(next_label + "::", start + len(label) + 2)
    if end < 0:
        end = len(text)
    return text[start:end]


field_control = read("src/field_control_avatar.c")
field_scripts = read("data/scripts/field_move_scripts.inc")
party = read("src/party_menu.c")
item_use = read("src/item_use.c")
flash = read("src/fldeff_flash.c")
rock = read("src/fldeff_rocksmash.c")
braille = read("src/braille_puzzles.c")
flash_script = read("data/scripts/flash.inc")

# Dive down and surface must require HM08 + Badge 7 through shared access while
# preserving the exact vanilla warp semantics.
for name, warp_result in [("TrySetupDiveDownScript", "TrySetDiveWarp() == 2"), ("TrySetupDiveEmergeScript", "TrySetDiveWarp() == 1")]:
    block = function_block(field_control, f"static bool32 {name}")
    if not block:
        fail(f"field_control_avatar.c: {name} missing")
        continue
    if "HasVanillaPlusHmAccess(ITEM_HM08_DIVE)" not in block:
        fail(f"{name}: Dive does not require shared HM08 item+badge access")
    if warp_result not in block:
        fail(f"{name}: vanilla Dive warp result semantics changed")
if "gMapHeader.mapType == MAP_TYPE_UNDERWATER" not in function_block(field_control, "static bool32 TrySetupDiveEmergeScript"):
    fail("TrySetupDiveEmergeScript: underwater map-type guard was lost")

# Waterfall interaction must require HM07 + Badge 8 and retain north-surfing +
# waterfall metatile guards.
water = function_block(field_control, "static const u8 *GetInteractedWaterScript")
for needle in [
    "MetatileBehavior_IsWaterfall(metatileBehavior)",
    "HasVanillaPlusHmAccess(ITEM_HM07_WATERFALL)",
    "IsPlayerSurfingNorth()",
]:
    if needle not in water:
        fail(f"GetInteractedWaterScript: Waterfall contract missing {needle}")

for label, next_label, move in [
    ("EventScript_UseWaterfall", "EventScript_CannotUseWaterfall", "MOVE_WATERFALL"),
    ("EventScript_UseDive", "EventScript_CantDive", "MOVE_DIVE"),
    ("EventScript_UseDiveUnderwater", "EventScript_CantSurface", "MOVE_DIVE"),
]:
    block = label_block(field_scripts, label, next_label)
    if f"checkpartymove {move}" in block:
        fail(f"{label}: still requires learned {move}")
    if "specialvar VAR_RESULT, GetVanillaPlusHmFieldActor" not in block:
        fail(f"{label}: safe HM actor is not selected")
    if "goto_if_eq VAR_RESULT, PARTY_SIZE" not in block:
        fail(f"{label}: missing safe-actor rejection")

# Fly remains badge/item-gated by the shared dispatcher, map-gated by vanilla,
# and still opens the existing region map.
bridge = function_block(party, "u8 VanillaPlus_TryUseHmFieldTool")
if "HasVanillaPlusHmAccess(itemId)" not in bridge:
    fail("HM dispatcher: shared item+badge access gate missing")
fly_setup = function_block(party, "static bool8 SetUpFieldMove_Fly")
if "Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType)" not in fly_setup:
    fail("SetUpFieldMove_Fly: vanilla map-type restriction was lost")
if "ITEM_HM02_FLY" not in item_use or "CB2_OpenFlyMap" not in item_use:
    fail("item_use.c: Fly HM does not route to the existing region map")

# Flash / Rock Smash puzzle routes must survive decoupling.
for needle in ["ShouldDoBrailleRegisteelEffect()", "SetUpPuzzleEffectRegisteel"]:
    if needle not in flash:
        fail(f"fldeff_flash.c: Registeel Flash path missing {needle}")
for needle in ["ShouldDoBrailleRegirockEffect()", "SetUpPuzzleEffectRegirock"]:
    if needle not in rock:
        fail(f"fldeff_rocksmash.c: Regirock Rock Smash path missing {needle}")
if "setflashlevel 0" not in flash_script:
    fail("flash.inc: Phase 3 full-visibility Flash behavior regressed")

if "GetCursorSelectionMonId()" in braille:
    fail("braille_puzzles.c: tomb puzzle still depends on party-menu cursor")
for name in ["SetUpPuzzleEffectRegisteel", "SetUpPuzzleEffectRegirock"]:
    block = function_block(braille, f"void {name}")
    if "GetVanillaPlusHmFieldActor()" not in block:
        fail(f"braille_puzzles.c: {name} does not use safe HM actor")

if errors:
    print("Phase 4 Task 5 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Phase 4 Task 5 verification PASSED")
