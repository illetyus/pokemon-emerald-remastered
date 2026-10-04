#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 8 RTC settings and safe time realignment."""
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


option_h = read("include/option_menu.h")
option_c = read("src/option_menu.c")
start_menu = read("src/start_menu.c")
main_menu = read("src/main_menu.c")
rtc_screen_h = read("include/reset_rtc_screen.h")
rtc_screen_c = read("src/reset_rtc_screen.c")
rtc_h = read("include/rtc.h")
rtc_c = read("src/rtc.c")
clock_h = read("include/clock.h")
clock_c = read("src/clock.c")
global_h = read("include/global.h")
event_data_h = read("include/event_data.h")
event_data_c = read("src/event_data.c")
title_screen_c = read("src/title_screen.c")
makefile = read("Makefile")
strings_h = read("include/strings.h")
strings = read("src/strings.c")
workflow = read(".github/workflows/build.yml")

required = [
    ("CB2_InitOptionMenuInGame", option_h + option_c, "in-game Options API"),
    ("CB2_InitRtcSettingsScreen", rtc_screen_h + rtc_screen_c, "player RTC settings API"),
    ("RtcIsUsableForSettings", rtc_h + rtc_c, "RTC usability policy"),
    ("RealignTimeBasedEventsAfterRtcCorrection", clock_h + clock_c, "time-event realignment API"),
]
for needle, text, label in required:
    if needle not in text:
        fail(f"missing Phase 8 {label}: {needle}")

# Task 2: explicit in-game Options entry and RTC row.
if "void CB2_InitOptionMenuInGame(void);" not in option_h:
    fail("include/option_menu.h: missing CB2_InitOptionMenuInGame declaration")
if "void CB2_InitRtcSettingsScreen(void);" not in rtc_screen_h:
    fail("include/reset_rtc_screen.h: missing CB2_InitRtcSettingsScreen declaration")
if "gText_RtcSettings" not in strings_h or 'gText_RtcSettings[] = _("RTC AYARLARI")' not in strings:
    fail("RTC AYARLARI string contract missing")
start_option = function_block(start_menu, "static bool8 StartMenuOptionCallback")
if not start_option:
    fail("src/start_menu.c: StartMenuOptionCallback missing")
else:
    if "CB2_InitOptionMenuInGame" not in start_option:
        fail("Start Menu must enter explicit in-game Options mode")
    if "SetMainCallback2(CB2_InitOptionMenu);" in start_option:
        fail("Start Menu still enters normal/title Options mode")
if "SetMainCallback2(CB2_InitOptionMenu);" not in main_menu:
    fail("title/main-menu Options path must remain normal CB2_InitOptionMenu")
if "MENUITEM_RTC_SETTINGS" not in option_c or "CB2_InitRtcSettingsScreen" not in option_c:
    fail("in-game Options RTC row/action missing")

# Task 3: RTC validity policy and player-facing editors.
if "bool8 RtcIsUsableForSettings(void);" not in rtc_h:
    fail("include/rtc.h: missing RtcIsUsableForSettings declaration")
rtc_policy = function_block(rtc_c, "bool8 RtcIsUsableForSettings(void)")
if not rtc_policy:
    fail("src/rtc.c: RtcIsUsableForSettings definition missing")
else:
    for needle in ["RTC_INIT_ERROR", "RTC_ERR_FLAG_MASK", "RtcGetErrorStatus"]:
        if needle not in rtc_policy:
            fail(f"RtcIsUsableForSettings missing {needle}")
    if "sRtcDummy" in rtc_policy or "RtcGetInfo" in rtc_policy:
        fail("RtcIsUsableForSettings must not use dummy RTC fallback")
for needle in [
    "RTC_MENU_SET_TIME",
    "RTC_MENU_ADVANCED",
    "RtcIsUsableForSettings()",
    "RTC_EDIT_MAX_DAYS 9999",
    "RTC_EDIT_MAX_HOURS 23",
    "RTC_EDIT_MAX_MINUTES 59",
    "RTC_SETTINGS_SECONDS 0",
]:
    if needle not in rtc_screen_c:
        fail(f"player RTC settings UI contract missing {needle}")
for text_name in [
    "gText_RtcSetTime",
    "gText_RtcAdvanced",
    "gText_RtcBack",
    "gText_RtcUnavailable",
    "gText_RtcConfirmCorrection",
]:
    if text_name not in strings_h or text_name not in strings:
        fail(f"RTC settings string missing {text_name}")

# RTC UI regression checks from device field test.
editor_block = function_block(rtc_screen_c, "static void DrawRtcSettingsEditor")
if not editor_block:
    fail("src/reset_rtc_screen.c: RTC settings editor renderer missing")
elif "gText_RtcEditHint" in editor_block:
    fail("RTC editor help must render only in WIN_MSG, not duplicate/clip inside WIN_TIME")
if "sText_RtcConfirmCorrectionCompact" not in rtc_screen_c:
    fail("RTC confirmation copy must use compact text that fits the vanilla message window")

# Task 4: safe local-time correction, event-reference realignment, and save result handling.
if "void RealignTimeBasedEventsAfterRtcCorrection(s32 days, s32 hours, s32 minutes);" not in clock_h:
    fail("include/clock.h: missing RTC realignment helper declaration")
realign = function_block(clock_c, "void RealignTimeBasedEventsAfterRtcCorrection")
if not realign:
    fail("src/clock.c: RTC realignment helper definition missing")
else:
    for needle in [
        "RtcCalcLocalTimeOffset",
        "RtcCalcLocalTime",
        "lastBerryTreeUpdate",
        "VarSet(VAR_DAYS",
    ]:
        if needle not in realign:
            fail(f"RTC realignment helper missing {needle}")
    for forbidden in [
        "RtcReset",
        "ClearDailyFlags",
        "BerryTreeTimeUpdate",
        "UpdatePerDay",
        "UpdatePerMinute",
        "DoTimeBasedEvents",
    ]:
        if forbidden in realign:
            fail(f"RTC realignment helper must not advance time events via {forbidden}")

rtc_settings_task = function_block(rtc_screen_c, "static void Task_RtcSettingsScreen")
if not rtc_settings_task:
    fail("src/reset_rtc_screen.c: player RTC settings task missing")
else:
    for needle in [
        "RealignTimeBasedEventsAfterRtcCorrection",
        "TrySavingData(SAVE_NORMAL)",
        "SAVE_STATUS_OK",
        "gText_SaveCompleted",
        "gText_SaveFailed",
    ]:
        if needle not in rtc_settings_task:
            fail(f"player RTC apply/save path missing {needle}")
    for forbidden in ["RtcReset()", "DisableResetRTC()"]:
        if forbidden in rtc_settings_task:
            fail(f"player RTC settings must not use legacy reset path: {forbidden}")
    if "Menu_ProcessInputNoWrapClearOnChoose()" not in rtc_settings_task:
        fail("RTC yes/no confirmation menu must clear its window after a choice")

legacy_reset_task = function_block(rtc_screen_c, "static void Task_ResetRtcScreen")
if not legacy_reset_task or "RtcReset()" not in legacy_reset_task:
    fail("legacy Task_ResetRtcScreen must retain RtcReset() recovery behavior")

# Task 5: save-layout compatibility and vanilla hidden RTC recovery are permanent contracts.
save_block1 = function_block(global_h, "struct SaveBlock1")
save_block2 = function_block(global_h, "struct SaveBlock2")
if not save_block1 or not save_block2:
    fail("include/global.h: SaveBlock1/SaveBlock2 definitions missing")
else:
    for block_name, block in [("SaveBlock1", save_block1), ("SaveBlock2", save_block2)]:
        rtc_field_lines = [
            line.strip()
            for line in block.splitlines()
            if ";" in line and re.search(r"\brtc\w*", line, re.IGNORECASE)
        ]
        if rtc_field_lines:
            fail(f"include/global.h: {block_name} gained RTC-specific persisted field(s): {rtc_field_lines}")
    if "struct Time localTimeOffset;" not in save_block2:
        fail("SaveBlock2 must retain existing localTimeOffset field")
    if "struct Time lastBerryTreeUpdate;" not in save_block2:
        fail("SaveBlock2 must retain existing lastBerryTreeUpdate field")

legacy_event_data = event_data_h + event_data_c
for needle in ["EnableResetRTC", "DisableResetRTC", "CanResetRTC"]:
    if needle not in legacy_event_data:
        fail(f"legacy RTC recovery contract missing {needle}")
if "RESET_RTC_BUTTON_COMBO" not in title_screen_c:
    fail("title screen hidden RTC reset button combo missing")
if "CB2_InitResetRtcScreen" not in title_screen_c or "CB2_InitResetRtcScreen" not in rtc_screen_c:
    fail("legacy hidden RTC reset screen entry missing")

# Exact build-marker ownership moved to Phase 5. Phase 8 remains version-agnostic
# but still requires release/test/Continue markers to stay synchronized.
release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
continue_match = re.search(r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)', strings)
if not release_match or not test_match or not continue_match:
    fail("Phase 8: synchronized Vanilla+ build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    fail("Phase 8: release/test/Continue build markers must stay synchronized")


if "GAME_CODE   := BPEE" not in makefile:
    fail("Makefile: GAME_CODE must remain BPEE")
for verifier in [
    "tools/phase_version_contract_test.py",
    "tools/vanillaplus_phase01_verify.py",
    "tools/vanillaplus_phase02_verify.py",
    "tools/vanillaplus_phase03_verify.py",
    "tools/vanillaplus_phase04_verify.py",
    "tools/vanillaplus_phase05_verify.py",
    "tools/vanillaplus_phase08_verify.py",
]:
    if f"python3 {verifier}" not in workflow:
        fail(f".github/workflows/build.yml: verifier missing {verifier}")

if errors:
    print("Vanilla+ Phase 8 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Vanilla+ Phase 8 verification PASSED")
