#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 2."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def fail(message: str) -> None:
    errors.append(message)

party = read("src/party_menu.c")
learned = re.search(
    r"(?ms)^static void Task_LearnedMove\(u8 taskId\)\s*\{(.*?)^\}",
    party,
)
if not learned:
    fail("src/party_menu.c: Task_LearnedMove not found")
else:
    body = learned.group(1)
    if "RemoveBagItem(item, 1)" in body:
        fail("src/party_menu.c: TMs must not be removed in Task_LearnedMove")
    if "AdjustFriendship(mon, FRIENDSHIP_EVENT_LEARN_TMHM);" not in body:
        fail("src/party_menu.c: TM/HM friendship behavior must be preserved")

tutors = read("data/scripts/move_tutors.inc")
for forbidden in [
    "goto_if_set FLAG_MOVE_TUTOR_TAUGHT_",
    "setflag FLAG_MOVE_TUTOR_TAUGHT_",
    "call MoveTutor_EventScript_CanOnlyBeLearnedOnce",
]:
    if forbidden in tutors:
        fail(f"data/scripts/move_tutors.inc: one-time tutor behavior remains: {forbidden}")

# Keep the party menu and tutor move IDs intact: all 10 Emerald overworld tutors
# must still route through the standard tutor party menu.
if tutors.count("call MoveTutor_EventScript_OpenPartyMenu") != 10:
    fail("data/scripts/move_tutors.inc: expected 10 overworld tutor party-menu calls")

summary = read("src/pokemon_summary_screen.c")
can_replace = re.search(
    r"(?ms)^static bool8 CanReplaceMove\(void\)\s*\{(.*?)^\}",
    summary,
)
if not can_replace:
    fail("src/pokemon_summary_screen.c: CanReplaceMove not found")
else:
    body = can_replace.group(1)
    if "IsMoveHm" in body:
        fail("src/pokemon_summary_screen.c: CanReplaceMove must not reject HM moves")
    if not re.search(r"\breturn\s+TRUE\s*;", body):
        fail("src/pokemon_summary_screen.c: CanReplaceMove must allow replacement")

# Exact build marker ownership belongs to the current phase verifier. Older
# phase verifiers remain version-agnostic so later phases can advance safely.
workflow = read(".github/workflows/build.yml")
if "python3 tools/vanillaplus_phase01_verify.py" not in workflow:
    fail(".github/workflows/build.yml: Phase 0-1 verifier missing")
if "python3 tools/vanillaplus_phase02_verify.py" not in workflow:
    fail(".github/workflows/build.yml: Phase 2 verifier missing")

if errors:
    print("Vanilla+ Phase 2 verification FAILED:")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Vanilla+ Phase 2 verification PASSED")
