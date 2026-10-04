#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 5 item-management QoL."""
from pathlib import Path
import re
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


global_h = read("include/global.h")
item_h = read("include/item.h")
vp_h = read("include/vanillaplus_items.h")
vp_c = read("src/vanillaplus_items.c")
item_menu = read("src/item_menu.c")
item_menu_h = read("include/item_menu.h")
player_pc = read("src/player_pc.c")
party = read("src/party_menu.c")
party_data = read("src/data/party_menu.h")
strings = read("src/strings.c")
strings_h = read("include/strings.h")
makefile = read("Makefile")
workflow = read(".github/workflows/build.yml")

# Save-layout compatibility: Phase 5 must consume existing reserve bytes through
# accessors instead of inserting fields into the save structures.
for needle in [
    "/*0x496*/ u16 registeredItem;",
    "/*0x498*/ struct ItemSlot pcItems[PC_ITEMS_COUNT];",
    "/*0x560*/ struct ItemSlot bagPocket_Items[BAG_ITEMS_COUNT];",
    "/*0x3598*/ u8 unused_3598[0x180];",
    "// sizeof: 0x3D88",
]:
    if needle not in global_h:
        fail(f"save-layout anchor changed or missing: {needle}")

for forbidden in ["phase5Metadata", "quickItems[", "bagSortMode[", "autoSortMask"]:
    save1 = global_h[global_h.find("struct SaveBlock1"):global_h.find("extern struct SaveBlock1")]
    if forbidden in save1:
        fail(f"SaveBlock1 gained persistent Phase 5 field: {forbidden}")

for api in [
    "VANILLAPLUS_QUICK_ITEM_MAX",
    "enum VanillaPlusItemSortMode",
    "VanillaPlusSortBagPocket",
    "VanillaPlusSortPCItems",
    "VanillaPlusMaybeAutoSortBagPocket",
    "VanillaPlusGetBagSortMode",
    "VanillaPlusSetBagSortMode",
    "VanillaPlusIsAutoSortEnabled",
    "VanillaPlusSetAutoSortEnabled",
    "VanillaPlusRegisterQuickItem",
    "VanillaPlusUnregisterQuickItem",
    "VanillaPlusGetQuickItem",
    "VanillaPlusGetQuickItemCount",
    "VanillaPlusPruneQuickItems",
]:
    if api not in vp_h or api not in vp_c:
        fail(f"Phase 5 shared item API missing: {api}")

for mode in ["VP_ITEM_SORT_NAME", "VP_ITEM_SORT_TYPE", "VP_ITEM_SORT_QUANTITY", "VP_ITEM_SORT_VALUE"]:
    if mode not in vp_h or mode not in vp_c:
        fail(f"sort mode missing: {mode}")

for token in ["unused_3598", "VANILLAPLUS_ITEM_META_MAGIC", "VANILLAPLUS_ITEM_META_VERSION"]:
    if token not in vp_c:
        fail(f"reserved-region metadata contract missing: {token}")

for token in ["CHAR_C_CEDILLA", "CHAR_G_BREVE", "CHAR_I_DOTTED", "CHAR_O_DIAERESIS", "CHAR_S_CEDILLA", "CHAR_U_DIAERESIS"]:
    if token not in vp_c:
        fail(f"Turkish collation mapping missing: {token}")

if "StartItemSwap(taskId)" not in item_menu or "JOY_NEW(SELECT_BUTTON)" not in item_menu:
    fail("vanilla SELECT manual Bag move must remain")
for token in ["JOY_NEW(START_BUTTON)", "OpenVanillaPlusBagSortMenu", "VanillaPlusMaybeAutoSortBagPocket"]:
    if token not in item_menu:
        fail(f"Bag sorting integration missing: {token}")

if "ItemStorage_StartItemSwap" not in player_pc or "JOY_NEW(SELECT_BUTTON)" not in player_pc:
    fail("vanilla SELECT PC item move must remain")
for token in ["JOY_NEW(START_BUTTON)", "VanillaPlusSortPCItems"]:
    if token not in player_pc:
        fail(f"PC item sorting integration missing: {token}")

for token in ["MENU_MOVE_ITEM", "CursorCb_MoveItem", "gText_MoveItemWhere"]:
    if token not in party and token not in party_data and token not in strings and token not in strings_h:
        fail(f"held-item MOVE UI missing: {token}")
if "MON_DATA_HELD_ITEM" not in party or "SetMonData" not in party:
    fail("held-item move/swap must update Pokémon held-item data")

for token in ["VanillaPlusRegisterQuickItem", "VanillaPlusGetQuickItemCount", "VanillaPlusPruneQuickItems"]:
    if token not in item_menu:
        fail(f"quick-item Bag/field integration missing: {token}")
# Slot 0 compatibility is intentionally centralized in vanillaplus_items.c rather
# than duplicating raw registeredItem writes throughout the Bag UI.
if "gSaveBlock1Ptr->registeredItem" not in vp_c or "VanillaPlusSyncPrimaryRegisteredItem" not in vp_c:
    fail("primary vanilla registeredItem compatibility path missing")

for token in ["gText_SortItems", "gText_SortByName", "gText_SortByType", "gText_SortByQuantity", "gText_SortByValue", "gText_AutoSort", "gText_MoveItemWhere"]:
    if token not in strings or token not in strings_h:
        fail(f"Phase 5 UI string missing: {token}")

release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
continue_match = re.search(r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)', strings)
if not release_match or not test_match or not continue_match:
    fail("Phase 5 synchronized build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    fail("Phase 5 release/test/Continue build markers must stay synchronized")
if "python3 tools/vanillaplus_phase05_verify.py" not in workflow:
    fail("permanent build workflow does not run Phase 5 verifier")

if errors:
    print("Vanilla+ Phase 5 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Vanilla+ Phase 5 verification PASSED")
