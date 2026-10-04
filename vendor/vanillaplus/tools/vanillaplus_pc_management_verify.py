#!/usr/bin/env python3
"""Static source checks for Vanilla+ PC/Pokemon management QoL (VP019)."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []

def read(path):
    p = ROOT / path
    if not p.exists():
        errors.append(f"missing required file: {path}")
        return ""
    return p.read_text(encoding="utf-8")

def require(source, token, label):
    if token not in source:
        errors.append(f"{label}: missing {token}")

storage_c = read("src/pokemon_storage_system.c")
storage_h = read("include/pokemon_storage_system.h")
global_h = read("include/global.h")
makefile = read("Makefile")
strings = read("src/strings.c")
build_workflow = read(".github/workflows/build.yml")
branch_workflow = read(".github/workflows/pc-management-branch.yml")

# VP019 owns the exact build marker.
require(makefile, "ZUMRUT VP019", "release build marker")
require(makefile, "ZUMRUT T019", "test build marker")
require(strings, 'gText_ContinueMenuPlayer[] = _("OYUNCU V+019")', "Continue marker")

# Save compatibility guard: this phase must not persist new management state.
for forbidden in [
    "sortMode",
    "boxSortMode",
    "storageSortMode",
    "pcFilter",
    "boxFilter",
]:
    if forbidden.lower() in storage_h.lower() or forbidden.lower() in global_h.lower():
        errors.append(f"save-layout guard: unexpected persisted field {forbidden}")

# Current-box sorting / organization surface.
for token in [
    "MENU_SORT",
    "MENU_SORT_SPECIES",
    "MENU_SORT_LEVEL",
    "MENU_SORT_TYPE",
    "MENU_COMPACT",
    'sPCText_Sort[] = _("SIRALA")',
    'sPCText_SortSpecies[] = _("TÜR")',
    'sPCText_SortLevel[] = _("SEVİYE")',
    'sPCText_SortType[] = _("TİP")',
    'sPCText_Compact[] = _("TOPARLA")',
    "Task_HandleBoxSort",
    "SortCurrentBox(BOX_SORT_SPECIES)",
    "SortCurrentBox(BOX_SORT_LEVEL)",
    "SortCurrentBox(BOX_SORT_TYPE)",
    "CompactCurrentBox()",
]:
    require(storage_c, token, "current-box sort/organization UI")

# Menu-capacity guard: the largest Pokémon context remains exactly within 7 entries.
require(storage_c, "struct StorageMenu menuItems[7];", "storage menu capacity")

# Safety: no sorting while moving a mon or in held-item mode.
require(
    storage_c,
    "if (!IsMonBeingMoved() && sStorage->boxOption != OPTION_MOVE_ITEMS)",
    "sort safety gate",
)

# Data integrity: complete BoxPokemon records move intact.
for token in [
    "struct BoxPokemon key;",
    "key = boxMons[i];",
    "boxMons[j] = boxMons[j - 1];",
    "boxMons[j] = key;",
    "SpeciesToNationalPokedexNum",
    "GetLevelFromBoxMonExp",
    "gSpeciesInfo[speciesA].types[0]",
    "MON_DATA_IS_EGG",
]:
    require(storage_c, token, "sort data integrity")

# Visual layer is rebuilt, storage data is not recreated.
require(storage_c, "RebuildCurrentBoxMonSprites", "post-sort visual rebuild")
require(storage_c, "DestroyBoxMonIcon", "old icon cleanup")
require(storage_c, "InitBoxMonSprites(StorageGetCurrentBox())", "sorted icon rebuild")
require(storage_c, "ZeroBoxMonData(&boxMons[readPos])", "compact vacated-slot cleanup")

# Held-item QoL in normal PC modes.
for token in [
    'sPCText_ItemLabel[] = _("E:")',
    "RemoveDisplayedMonHeldItem",
    "if (sStorage->displayMonItemId == ITEM_NONE)",
    "SetMenuText(MENU_GIVE_2)",
    "SetMenuText(MENU_BAG)",
    "Task_GiveItemFromBag",
    "if (sStorage->boxOption == OPTION_MOVE_ITEMS)",
    "PrintMessage(MSG_PLACED_IN_BAG)",
]:
    require(storage_c, token, "held-item QoL")
# Normal-mode removal must not wait on Move Items sprite animation.
require(
    storage_c,
    "RemoveDisplayedMonHeldItem();\n                PrintMessage(MSG_PLACED_IN_BAG);\n                sStorage->state = 2;",
    "normal-mode held-item animation bypass",
)

# START quick Box <-> Party transfer.
for token in [
    "INPUT_QUICK_DEPOSIT",
    "Task_QuickDepositCurrentBox",
    "TryStorePartyMonInBox(StorageGetCurrentBox())",
    "IsRemovingLastPartyMon()",
    "ItemIsMail(sStorage->displayMonItemId)",
    "MSG_BOX_IS_FULL",
    "return INPUT_WITHDRAW;",
    "JOY_NEW(START_BUTTON)",
]:
    require(storage_c, token, "quick Box/Party transfer")

# CI ownership.
require(build_workflow, "python3 tools/vanillaplus_pc_management_verify.py", "permanent CI verifier")
require(branch_workflow, "python3 tools/vanillaplus_pc_management_verify.py", "branch CI verifier")
require(branch_workflow, "pokemon-emerald-vp019-release.gba", "VP019 release artifact")
require(branch_workflow, "pokemon-emerald-vp019-test.gba", "VP019 test artifact")

if errors:
    print("Vanilla+ PC management verification FAILED:")
    for error in errors:
        print(" - " + error)
    sys.exit(1)

print("Vanilla+ PC management verification PASSED")
print(" - VP019/T019/V+019 synchronized")
print(" - current-box Species/Level/Type sort and compact wired")
print(" - quick Box/Party transfer and held-item actions wired")
print(" - whole BoxPokemon records are moved intact")
print(" - save-layout persistence remains untouched")
