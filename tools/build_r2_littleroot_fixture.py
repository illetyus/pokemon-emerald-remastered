#!/usr/bin/env python3
"""Build the pinned real-Vanilla+ Littleroot/Route101 R2 native fixture."""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_scripts import convert_script_closure
from generate_script_c import generate_c_fixture

ROOTS = [
    Path("data/maps/LittlerootTown/scripts.inc"),
    Path("data/maps/LittlerootTown_BrendansHouse_1F/scripts.inc"),
    Path("data/maps/LittlerootTown_BrendansHouse_2F/scripts.inc"),
    Path("data/maps/LittlerootTown_MaysHouse_1F/scripts.inc"),
    Path("data/maps/LittlerootTown_MaysHouse_2F/scripts.inc"),
    Path("data/maps/LittlerootTown_ProfessorBirchsLab/scripts.inc"),
    Path("data/maps/Route101/scripts.inc"),
]

ENTRY_LABELS = [
    "LittlerootTown_EventScript_StepOffTruckMale",
    "LittlerootTown_EventScript_StepOffTruckFemale",
    "LittlerootTown_BrendansHouse_1F_EventScript_EnterHouseMovingIn",
    "LittlerootTown_MaysHouse_1F_EventScript_EnterHouseMovingIn",
    "LittlerootTown_BrendansHouse_2F_EventScript_WallClock",
    "LittlerootTown_MaysHouse_2F_EventScript_WallClock",
    "PlayersHouse_1F_EventScript_PetalburgGymReportMale",
    "PlayersHouse_1F_EventScript_PetalburgGymReportFemale",
    "LittlerootTown_BrendansHouse_2F_EventScript_MeetBrendan",
    "LittlerootTown_MaysHouse_2F_EventScript_MeetMay",
    "Route101_EventScript_StartBirchRescue",
    "Route101_EventScript_BirchsBag",
    "LittlerootTown_ProfessorBirchsLab_EventScript_GiveStarterEvent",
]


def build_fixture(source_root: Path) -> str:
    ir = convert_script_closure(source_root, ROOTS, entry_labels=ENTRY_LABELS)
    return generate_c_fixture(ir, source_root, symbol_prefix="gR2Littleroot")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    rendered = build_fixture(args.source_root)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(rendered + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
