#!/usr/bin/env python3
"""Apply Vanilla+ Phase 2 source changes deterministically and idempotently."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    (ROOT / path).write_text(text, encoding="utf-8")

# 1) Reusable TMs. HMs were already reusable; preserve friendship behavior.
path = "src/party_menu.c"
party = read(path)
old = """        AdjustFriendship(mon, FRIENDSHIP_EVENT_LEARN_TMHM);\n        if (item < ITEM_HM01_CUT)\n            RemoveBagItem(item, 1);\n"""
new = """        AdjustFriendship(mon, FRIENDSHIP_EVENT_LEARN_TMHM);\n        // Vanilla+ Phase 2: TMs, like HMs, are reusable.\n"""
if old in party:
    party = party.replace(old, new, 1)
elif "RemoveBagItem(item, 1);" in re.search(r"(?ms)^static void Task_LearnedMove\(u8 taskId\)\s*\{(.*?)^\}", party).group(1):
    raise SystemExit("Unexpected Task_LearnedMove layout; refusing broad edit")
write(path, party)

# 2) Make all one-time overworld tutors repeatable, including on old saves whose
# one-time tutor flags are already set. Keep the normal yes/no and party flow.
path = "data/scripts/move_tutors.inc"
tutors = read(path)
tutors = re.sub(
    r"\tcall MoveTutor_EventScript_CanOnlyBeLearnedOnce\n"
    r"\tgoto_if_eq VAR_RESULT, NO, [A-Za-z0-9_]+\n",
    "",
    tutors,
)
tutors = re.sub(r"^\tgoto_if_set FLAG_MOVE_TUTOR_TAUGHT_[A-Z0-9_]+, [A-Za-z0-9_]+\n", "", tutors, flags=re.M)
tutors = re.sub(r"^\tsetflag FLAG_MOVE_TUTOR_TAUGHT_[A-Z0-9_]+\n", "", tutors, flags=re.M)
write(path, tutors)

# 3) Allow HM moves to be replaced in the ordinary learn/replace flow.
path = "src/pokemon_summary_screen.c"
summary = read(path)
pattern = re.compile(r"(?ms)^static bool8 CanReplaceMove\(void\)\s*\{.*?^\}")
match = pattern.search(summary)
if not match:
    raise SystemExit("CanReplaceMove not found")
replacement = """static bool8 CanReplaceMove(void)\n{\n    // Vanilla+ Phase 2: HM moves may be forgotten/replaced normally.\n    return TRUE;\n}"""
summary = summary[:match.start()] + replacement + summary[match.end():]
write(path, summary)

# 4) Bump visible build markers while keeping the final file name and BPEE code.
path = "Makefile"
makefile = read(path)
makefile = makefile.replace("TITLE       := ZUMRUT T002", "TITLE       := ZUMRUT T003")
makefile = makefile.replace("TITLE       := ZUMRUT VP002", "TITLE       := ZUMRUT VP003")
write(path, makefile)

path = "src/strings.c"
strings = read(path)
strings = strings.replace('gText_ContinueMenuPlayer[] = _("OYUNCU V+002")', 'gText_ContinueMenuPlayer[] = _("OYUNCU V+003")')
write(path, strings)

print("Vanilla+ Phase 2 source changes applied")
