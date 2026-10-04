#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 4 HM field tools."""
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path: str) -> str:
    p = ROOT / path
    if not p.exists():
        errors.append(f"missing required file: {path}")
        return ""
    return p.read_text(encoding="utf-8")


def fail(message: str) -> None:
    errors.append(message)


def item_block(text: str, item: str) -> str:
    match = re.search(rf"\[{re.escape(item)}\]\s*=\s*\{{(.*?)\n\s*\}},", text, re.S)
    return match.group(1) if match else ""


def function_block(text: str, signature: str) -> str:
    """Return a function definition, skipping matching forward declarations."""
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


header = read("include/vanillaplus_hm.h")
source = read("src/vanillaplus_hm.c")
items = read("src/data/items.h")
item_use_h = read("include/item_use.h")
item_use = read("src/item_use.c")
party = read("src/party_menu.c")

# Task 1: explicit HM policy and safe visual actor.
for api in [
    "IsVanillaPlusHmItem",
    "GetVanillaPlusHmMove",
    "GetVanillaPlusHmBadgeFlag",
    "HasVanillaPlusHmAccess",
    "GetVanillaPlusHmFieldActor",
]:
    if api not in header or api not in source:
        fail(f"phase 4 HM policy API missing {api}")

mapping_pairs = [
    ("ITEM_HM01_CUT", "MOVE_CUT", "FLAG_BADGE01_GET"),
    ("ITEM_HM05_FLASH", "MOVE_FLASH", "FLAG_BADGE02_GET"),
    ("ITEM_HM06_ROCK_SMASH", "MOVE_ROCK_SMASH", "FLAG_BADGE03_GET"),
    ("ITEM_HM04_STRENGTH", "MOVE_STRENGTH", "FLAG_BADGE04_GET"),
    ("ITEM_HM03_SURF", "MOVE_SURF", "FLAG_BADGE05_GET"),
    ("ITEM_HM02_FLY", "MOVE_FLY", "FLAG_BADGE06_GET"),
    ("ITEM_HM08_DIVE", "MOVE_DIVE", "FLAG_BADGE07_GET"),
    ("ITEM_HM07_WATERFALL", "MOVE_WATERFALL", "FLAG_BADGE08_GET"),
]
for item, move, badge in mapping_pairs:
    if item not in source or move not in source or badge not in source:
        fail(f"HM mapping missing {item} -> {move} -> {badge}")

if "CheckBagHasItem" not in source or "FlagGet" not in source:
    fail("HasVanillaPlusHmAccess must require both bag item and badge flag")

for needle in ["MON_DATA_SPECIES", "SPECIES_NONE", "MON_DATA_IS_EGG", "PARTY_SIZE"]:
    if needle not in source:
        fail(f"safe HM field actor contract missing {needle}")

if "return MOVE_NONE;" not in source:
    fail("GetVanillaPlusHmMove must reject non-HMs with MOVE_NONE")
if "return 0;" not in source:
    fail("GetVanillaPlusHmBadgeFlag must reject non-HMs with flag 0")

# Task 2: HM items are field tools, not move-teaching items.
hm_items = [
    "ITEM_HM01_CUT", "ITEM_HM02_FLY", "ITEM_HM03_SURF", "ITEM_HM04_STRENGTH",
    "ITEM_HM05_FLASH", "ITEM_HM06_ROCK_SMASH", "ITEM_HM07_WATERFALL", "ITEM_HM08_DIVE",
]
for hm in hm_items:
    block = item_block(items, hm)
    if not block:
        fail(f"src/data/items.h: missing item block {hm}")
        continue
    if ".type = ITEM_USE_FIELD" not in block:
        fail(f"src/data/items.h: {hm} must use ITEM_USE_FIELD")
    if ".fieldUseFunc = ItemUseOutOfBattle_HMField" not in block:
        fail(f"src/data/items.h: {hm} must use ItemUseOutOfBattle_HMField")
    if "ItemUseOutOfBattle_TMHM" in block:
        fail(f"src/data/items.h: {hm} still routes to TM/HM teaching")

for tm in ["ITEM_TM01_FOCUS_PUNCH", "ITEM_TM50_OVERHEAT"]:
    block = item_block(items, tm)
    if ".type = ITEM_USE_PARTY_MENU" not in block or ".fieldUseFunc = ItemUseOutOfBattle_TMHM" not in block:
        fail(f"src/data/items.h: {tm} must keep TM teaching behavior")

if "ItemUseOutOfBattle_HMField" not in item_use_h or "ItemUseOutOfBattle_HMField" not in item_use:
    fail("HM field-use entry point is missing")
if "InUnionRoom" not in item_use or "InMultiPartnerRoom" not in item_use:
    fail("HM field-use path must preserve Union/Multi Partner Room denial")
if "HasVanillaPlusHmAccess" not in item_use:
    fail("HM field-use entry point must use the shared HM item+badge policy")

# Task 3: learned HMs are no longer party-menu field actions, and HM effects
# no longer depend on the party-menu cursor for their visual actor.
selection = function_block(party, "static void SetPartyMonFieldSelectionActions")
if not selection:
    fail("src/party_menu.c: SetPartyMonFieldSelectionActions missing")
elif "IsMoveHm" not in selection:
    fail("src/party_menu.c: learned HM moves are still eligible for party field actions")

hm_effect_files = [
    "src/fldeff_cut.c",
    "src/fldeff_flash.c",
    "src/fldeff_rocksmash.c",
    "src/fldeff_strength.c",
]
for path in hm_effect_files:
    text = read(path)
    if "GetCursorSelectionMonId()" in text:
        fail(f"{path}: HM effect still reads the party-menu cursor")
    if "GetVanillaPlusHmFieldActor" not in text:
        fail(f"{path}: HM effect does not use the shared safe field actor")

for name in ["FieldCallback_Surf", "FieldCallback_Dive", "FieldCallback_Waterfall"]:
    block = function_block(party, f"static void {name}")
    if not block:
        fail(f"src/party_menu.c: {name} missing")
    else:
        if "GetCursorSelectionMonId()" in block:
            fail(f"src/party_menu.c: {name} still reads the party-menu cursor")
        if "GetVanillaPlusHmFieldActor" not in block:
            fail(f"src/party_menu.c: {name} does not use the shared safe field actor")

field_effect = read("src/field_effect.c")
if "GetCursorSelectionMonId()" in field_effect:
    fail("src/field_effect.c: Fly field effect still reads the party-menu cursor")
if "GetVanillaPlusHmFieldActor" not in field_effect:
    fail("src/field_effect.c: Fly field effect does not use the shared safe field actor")


# Exact build-marker ownership moved to Phase 8. Phase 4 remains version-agnostic
# but still requires the release/test/Continue markers to stay synchronized.
makefile = read("Makefile")
release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
strings = read("src/strings.c")
continue_match = re.search(
    r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)',
    strings,
)
if not release_match or not test_match or not continue_match:
    fail("Phase 4: synchronized Vanilla+ build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    fail("Phase 4: release/test/Continue build markers must stay synchronized")
workflow = read(".github/workflows/build.yml")
if "python3 tools/vanillaplus_phase04_verify.py" not in workflow:
    fail(".github/workflows/build.yml: Phase 4 verifier missing")

# Focused field-interaction verifiers are permanent subcontracts of Phase 4.
for helper in ["phase04_task4_verify.py", "phase04_task5_verify.py"]:
    result = subprocess.run(
        [sys.executable, str(ROOT / "tools" / helper)],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    if result.returncode != 0:
        fail(f"{helper} failed:\n{result.stdout}{result.stderr}")

if errors:
    print("Vanilla+ Phase 4 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Vanilla+ Phase 4 verification PASSED")
