#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 0-1.

This intentionally checks source invariants rather than emulating gameplay. It is
fast enough to run before every ROM build and catches incomplete multi-stage Mart
updates and accidental broadening of reusable evolution items.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

MART_FILES = [
    "FallarborTown_Mart",
    "FortreeCity_Mart",
    "LavaridgeTown_Mart",
    "MauvilleCity_Mart",
    "MossdeepCity_Mart",
    "OldaleTown_Mart",
    "PetalburgCity_Mart",
    "RustboroCity_Mart",
    "SlateportCity_Mart",
    "SootopolisCity_Mart",
    "VerdanturfTown_Mart",
]

REQUIRED_MART_ITEMS = [
    "ITEM_ULTRA_BALL",
    "ITEM_MAX_REVIVE",
    "ITEM_FIRE_STONE",
    "ITEM_WATER_STONE",
    "ITEM_THUNDER_STONE",
    "ITEM_LEAF_STONE",
    "ITEM_MOON_STONE",
    "ITEM_SUN_STONE",
    "ITEM_PP_MAX",
]

REUSABLE_STONES = [
    "ITEM_FIRE_STONE",
    "ITEM_WATER_STONE",
    "ITEM_THUNDER_STONE",
    "ITEM_LEAF_STONE",
    "ITEM_MOON_STONE",
    "ITEM_SUN_STONE",
]

DIRECT_TRADE_ITEMS = [
    "ITEM_KINGS_ROCK",
    "ITEM_METAL_COAT",
    "ITEM_DRAGON_SCALE",
    "ITEM_UP_GRADE",
    "ITEM_DEEP_SEA_TOOTH",
    "ITEM_DEEP_SEA_SCALE",
]

errors = []


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def fail(message: str) -> None:
    errors.append(message)


# Every inventory branch in every normal Mart must expose the same core QoL stock.
for mart in MART_FILES:
    rel = f"data/maps/{mart}/scripts.inc"
    text = read(rel)
    blocks = re.findall(
        r"(?ms)^([A-Za-z0-9_]+_Pokemart(?:_[A-Za-z0-9_]+)?):\s*\n"
        r"((?:\s*\.2byte\s+ITEM_[A-Z0-9_]+\s*\n)+?\s*\.2byte\s+ITEM_NONE\s*)",
        text,
    )
    if not blocks:
        fail(f"{rel}: no Pokemart inventory blocks found")
        continue
    for label, body in blocks:
        for item in REQUIRED_MART_ITEMS:
            if re.search(rf"\b{re.escape(item)}\b", body) is None:
                fail(f"{rel}:{label}: missing {item}")
        if re.search(r"\bITEM_RARE_CANDY\b", body):
            fail(f"{rel}:{label}: Rare Candy must not be sold")

# Trade+item evolutions were changed to EVO_ITEM, so their items must actually be
# usable from the party menu. They are deliberately NOT part of REUSABLE_STONES.
items_text = read("src/data/items.h")
for item in DIRECT_TRADE_ITEMS:
    match = re.search(
        rf"(?ms)^\s*\[{re.escape(item)}\]\s*=\s*\{{(.*?)^\s*\}},",
        items_text,
    )
    if not match:
        fail(f"src/data/items.h: item block not found for {item}")
        continue
    body = match.group(1)
    if ".type = ITEM_USE_PARTY_MENU" not in body:
        fail(f"src/data/items.h:{item}: must use ITEM_USE_PARTY_MENU")
    if ".fieldUseFunc = ItemUseOutOfBattle_EvolutionStone" not in body:
        fail(f"src/data/items.h:{item}: must use ItemUseOutOfBattle_EvolutionStone")

# Only the six standard stones skip bag consumption after a successful evolution.
party = read("src/party_menu.c")
helper = re.search(
    r"(?ms)static\s+bool8\s+IsReusableEvolutionStone\s*\(\s*u16\s+itemId\s*\)\s*\{(.*?)^\}",
    party,
)
if not helper:
    fail("src/party_menu.c: IsReusableEvolutionStone(u16 itemId) missing")
else:
    helper_body = helper.group(1)
    for item in REUSABLE_STONES:
        if item not in helper_body:
            fail(f"src/party_menu.c:IsReusableEvolutionStone: missing {item}")
    for item in DIRECT_TRADE_ITEMS + ["ITEM_PP_MAX", "ITEM_RARE_CANDY"]:
        if item in helper_body:
            fail(f"src/party_menu.c:IsReusableEvolutionStone: must not include {item}")

if not re.search(
    r"if\s*\(\s*!IsReusableEvolutionStone\s*\(\s*gSpecialVar_ItemId\s*\)\s*\)\s*"
    r"RemoveBagItem\s*\(\s*gSpecialVar_ItemId\s*,\s*1\s*\)\s*;",
    party,
    re.S,
):
    fail("src/party_menu.c: successful evolution must consume only non-reusable items")

# Phase build identity / test-build contract. Phase 0-1 owns the structure, not
# the current phase number. Later phase verifiers pin their own exact version.
makefile = read("Makefile")
for needle in [
    "VANILLAPLUS_TEST ?= 0",
    "-DVANILLAPLUS_TEST=$(VANILLAPLUS_TEST)",
    "GAME_CODE   := BPEE",
]:
    if needle not in makefile:
        fail(f"Makefile: missing {needle!r}")

release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
strings = read("src/strings.c")
continue_match = re.search(
    r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)',
    strings,
)
if not release_match:
    fail("Makefile: release title must match ZUMRUT VP###")
if not test_match:
    fail("Makefile: test title must match ZUMRUT T###")
if not continue_match:
    fail("src/strings.c: Continue screen build marker must match OYUNCU V+###")
if release_match and test_match and continue_match:
    versions = {release_match.group(1), test_match.group(1), continue_match.group(1)}
    if len(versions) != 1:
        fail("Build markers must use the same three-digit version")

workflow = read(".github/workflows/build.yml")
for needle in [
    "python3 tools/vanillaplus_phase01_verify.py",
    "pokemon-emerald_v1.gba",
    "pokemon-emerald_v1-test.gba",
    "VANILLAPLUS_TEST=1",
]:
    if needle not in workflow:
        fail(f".github/workflows/build.yml: missing {needle!r}")

if errors:
    print("Vanilla+ Phase 0-1 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Vanilla+ Phase 0-1 verification PASSED")
