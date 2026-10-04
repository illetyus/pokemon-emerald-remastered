#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 6 Pokémon info/management QoL."""
from pathlib import Path
import sys
import re

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

global_h = read("include/global.h")
pokemon_h = read("include/pokemon.h")
summary = read("src/pokemon_summary_screen.c")
party = read("src/party_menu.c")
party_data = read("src/data/party_menu.h")
relearner = read("src/move_relearner.c")
relearner_h = read("include/move_relearner.h")
makefile = read("Makefile")
strings = read("src/strings.c")
workflow = read(".github/workflows/build.yml")

# Save/Pokémon persistence layouts must remain anchored.
for token in [
    "/*0x3598*/ u8 unused_3598[0x180];",
    "// sizeof: 0x3D88",
]:
    require(global_h, token, "save-layout guard")
for forbidden in ["phase6", "ivs[NUM_STATS]", "evs[NUM_STATS]", "hiddenPowerType"]:
    save1 = global_h[global_h.find("struct SaveBlock1"):global_h.find("extern struct SaveBlock1")]
    if forbidden in save1:
        errors.append(f"SaveBlock1 gained Phase 6 persistent field: {forbidden}")

# Summary advanced info: IV/EV are read from existing mon data.
for token in [
    "MON_DATA_HP_IV",
    "MON_DATA_HP_EV",
    "showAdvancedStats",
    "PrintVanillaPlusAdvancedStats",
    "GetVanillaPlusHiddenPowerType",
    "IsVanillaPlusAdvancedStatsTogglePressed",
    "JOY_NEW(A_BUTTON) || JOY_NEW(SELECT_BUTTON)",
    "sTextVpIvEvPrompt",
    "PrintVanillaPlusAButtonPrompt(PSS_LABEL_WINDOW_PROMPT_INFO, sTextVpIvEvPrompt)",
]:
    require(summary, token, "summary advanced view")
if summary.count("if (sMonSummaryScreen->showAdvancedStats)") < 4:
    errors.append("summary advanced view: page/mon navigation does not consistently close the IV/EV panel")
for type_name in [
    "TYPE_FIGHTING", "TYPE_FLYING", "TYPE_POISON", "TYPE_GROUND",
    "TYPE_ROCK", "TYPE_BUG", "TYPE_GHOST", "TYPE_STEEL",
    "TYPE_FIRE", "TYPE_WATER", "TYPE_GRASS", "TYPE_ELECTRIC",
    "TYPE_PSYCHIC", "TYPE_ICE", "TYPE_DRAGON", "TYPE_DARK",
]:
    require(summary, type_name, "Hidden Power mapping")

# Existing Nature and move-details behavior must remain.
for token in ["gNatureStatTable", "sTextNatureUp", "sTextNatureDown"]:
    require(summary, token, "nature color regression")
for token in ["PrintMovePowerAndAccuracy", "SetMoveTypeIcons", "gMoveDescriptionPointers", "CalculatePPWithBonus"]:
    require(summary, token, "move-detail regression")

# Party field menu shortcuts.
for token in ["MENU_MANAGE", "MENU_RELEARN", "MENU_RENAME", "CursorCb_Manage", "CursorCb_Relearn", "CursorCb_Rename"]:
    if token not in party and token not in party_data:
        errors.append(f"party shortcut missing: {token}")
require(party_data, "sPartyMenuAction_SummarySwitchCancel", "field party action list")
require(party_data, "MENU_RELEARN", "field party action list")
require(party_data, "MENU_RENAME", "field party action list")
require(party, "AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_MANAGE)", "field party manage entry")
require(party_data, "sPartyMenuAction_ManageCancel", "manage submenu")
require(summary, "PrintVanillaPlusStatLine", "IV/EV aligned table")
require(summary, "sVanillaPlusIvFields", "explicit IV field mapping")
require(summary, "sVanillaPlusEvFields", "explicit EV field mapping")
require(summary, "GetVanillaPlusSourceMonData", "direct source-mon IV/EV reads")
require(summary, "GetBoxMonData(&sMonSummaryScreen->monList.boxMons[sMonSummaryScreen->curMonIndex]", "box source reads")
require(summary, "GetMonData(&sMonSummaryScreen->monList.mons[sMonSummaryScreen->curMonIndex]", "party source reads")
require(summary, "GetStringRightAlignXOffset(FONT_NORMAL, number, ivRight)", "right-aligned IV values")
require(summary, "GetStringRightAlignXOffset(FONT_NORMAL, number, evRight)", "right-aligned EV values")
require(summary, "sLeftStats[i], 6, 47, 77, 16 * i", "three-row left stat geometry")
require(summary, "sRightStats[i], 86, 127, 158, 16 * i", "three-row right stat geometry")
require(summary, "sTextVpHiddenPower, 6, 56", "Hidden Power lower-zone geometry")
require(summary, "sTextVpTotalEv, 6, 72", "total EV lower-zone geometry")
require(party, "Task_VanillaPlusWaitForMessageDismiss", "persistent Phase 6 warning message")
require(party, "{PAUSE_UNTIL_PRESS}", "A/B warning dismissal")

# Direct Move Relearner must preserve vanilla economy and return path.
for token in [
    "VanillaPlusStartMoveRelearner",
    "CheckBagHasItem(ITEM_HEART_SCALE",
    "RemoveBagItem(ITEM_HEART_SCALE",
    "CB2_ReturnToFieldWithOpenMenu",
]:
    if token not in relearner and token not in relearner_h:
        errors.append(f"direct Move Relearner contract missing: {token}")

# Nickname shortcut must preserve Name Rater ownership rules.
for token in [
    "NAMING_SCREEN_NICKNAME",
    "MON_DATA_OT_ID",
    "MON_DATA_OT_NAME",
    "MON_DATA_IS_EGG",
    "DoNamingScreen",
]:
    require(party, token, "nickname shortcut")

# Completed phases only require synchronized markers; the current phase owns the exact number.
release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
continue_match = re.search(r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)', strings)
if not release_match or not test_match or not continue_match:
    errors.append("Phase 6 synchronized build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    errors.append("Phase 6 release/test/Continue build markers must stay synchronized")
require(workflow, "python3 tools/vanillaplus_phase06_verify.py", "permanent CI Phase 6 verifier")

if errors:
    print("Vanilla+ Phase 6 verification FAILED:")
    for error in errors:
        print(" - " + error)
    sys.exit(1)

print("Vanilla+ Phase 6 verification PASSED")

# Phase 6 must not change the original PokeSummary runtime layout with IV/EV cache arrays.
summary_struct = summary[summary.find("struct PokeSummary"):summary.find("} summary;")]
if "u8 ivs[NUM_STATS]" in summary_struct or "u8 evs[NUM_STATS]" in summary_struct:
    errors.append("PokeSummary layout changed by IV/EV cache arrays")
if "sum->ivs[" in summary or "sum->evs[" in summary or "summary.ivs[" in summary or "summary.evs[" in summary:
    errors.append("IV/EV display still depends on summary cache instead of source mon")

# Turkish UI: use the readable full Pokemon word in unconstrained message boxes.
strings_src = read("src/strings.c")
for needle in [
    'gText_DoWhatWithPokemon[] = _("POKéMON ile ne yapılsın?")',
    'gText_CantSelectSamePkmn[] = _("Aynı POKéMON seçilemez.")',
    'gText_SamePkmnInPartyAlready[] = _("Aynı POKéMON takımda.")',
]:
    if needle not in strings_src:
        errors.append(f"missing readable Pokemon UI text: {needle}")
# Width-constrained menus may deliberately retain the original two-glyph {PKMN} token.
