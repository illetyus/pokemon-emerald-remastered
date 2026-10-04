#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 7 speed/flow QoL."""
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

text = read("gflib/text.c")
menu = read("src/menu.c")
field = read("src/field_effect.c")
main = read("src/main.c")
nurse = read("data/scripts/pkmn_center_nurse.inc")
makefile = read("Makefile")
strings = read("src/strings.c")
workflow = read(".github/workflows/build.yml")

# FAST remains a valid non-zero printer delay; acceleration is multi-glyph per frame.
require(menu, "[OPTIONS_TEXT_SPEED_FAST] = 1", "FAST delay")
require(text, "#define VANILLAPLUS_FAST_TEXT_GLYPHS_PER_FRAME 4", "FAST text acceleration")
require(text, "GetPlayerTextSpeed() == OPTIONS_TEXT_SPEED_FAST", "FAST text selection")
require(text, "while (glyphBudget-- != 0 && sTextPrinters[i].active)", "bounded text burst")
require(text, "case RENDER_UPDATE:", "text control barrier")
require(text, "glyphBudget = 0;", "text control/finish barrier")

# Held directional input should repeat sooner/faster in menus and lists.
require(main, "gKeyRepeatContinueDelay = 3;", "menu key repeat cadence")
require(main, "gKeyRepeatStartDelay = 24;", "menu key repeat start")

# Shared field-move presentation is shorter without changing HM checks or move logic.
require(field, "sprite->x -= 40", "field move mon slide")
require(field, "sprite->sOnscreenTimer = 4", "field move mon hold")
require(field, "task->tBgHoriz -= 32", "field move banner speed")
require(field, "task->tBgOffsetIdx += 32", "indoor field move background speed")
require(field, "task->tBgOffset += 4", "indoor field move tile speed")
require(field, "for (j = 0; j < 4; j++)", "indoor field move complete tile batches")

# Pokemon Center-only visual timings are accelerated; Hall of Fame keeps vanilla values.
require(field, "sprite->sTimer = sprite->sPlayHealSe ? 10 : 25", "center ball placement timing")
require(field, "sprite->sTimer = sprite->sPlayHealSe ? 12 : 32", "center placement tail timing")
require(field, "sprite->sTimer = sprite->sPlayHealSe ? 4 : 8", "center flash timing")
require(field, "sprite->sTimer = sprite->sPlayHealSe ? 12 : 30", "center flash tail timing")
require(field, "CreateGlowingPokeballsEffect(task->tNumMons, task->tFirstBallX, task->tFirstBallY, FALSE)", "Hall of Fame isolation")
require(nurse, "EventScript_FollowerIntoPokeballDelay::\n\tdelay 8", "center follower wait")
require(nurse, "Movement_PkmnCenterNurse_Bow:\n\tnurse_joy_bow\n\tdelay_2", "center nurse bow")

# Completed phases are version-agnostic but require synchronized build markers.
release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
continue_match = re.search(r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)', strings)
if not release_match or not test_match or not continue_match:
    errors.append("Phase 7 synchronized build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    errors.append("Phase 7 release/test/Continue build markers must stay synchronized")
require(workflow, "python3 tools/vanillaplus_phase07_verify.py", "permanent CI Phase 7 verifier")

if errors:
    print("Vanilla+ Phase 7 verification FAILED:")
    for error in errors:
        print(" - " + error)
    sys.exit(1)

print("Vanilla+ Phase 7 verification PASSED")
