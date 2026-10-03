import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from convert_scripts import convert_script_closure  # noqa: E402
from generate_script_c import generate_c_fixture  # noqa: E402


SOURCE = ROOT / "tests/fixtures/r2/vanillaplus_source"
ROOTS = [
    Path("data/maps/LittlerootTown/scripts.inc"),
    Path("data/maps/LittlerootTown_BrendansHouse_1F/scripts.inc"),
    Path("data/maps/LittlerootTown_BrendansHouse_2F/scripts.inc"),
    Path("data/maps/LittlerootTown_MaysHouse_1F/scripts.inc"),
    Path("data/maps/LittlerootTown_MaysHouse_2F/scripts.inc"),
    Path("data/maps/LittlerootTown_ProfessorBirchsLab/scripts.inc"),
    Path("data/maps/Route101/scripts.inc"),
]
ENTRIES = [
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


class R2RealOpeningSourceTests(unittest.TestCase):
    def test_snapshot_is_pinned_to_acceptance_commit(self):
        self.assertEqual(
            (SOURCE / "VANILLAPLUS_COMMIT.txt").read_text(encoding="utf-8").strip(),
            "70db90c9077aed1272e746fc2537d9f12b95a91c",
        )

    def test_real_opening_closure_converts_and_lowers_to_native_c(self):
        ir = convert_script_closure(SOURCE, ROOTS, entry_labels=ENTRIES)
        script_ids = {item["script_id"] for item in ir["scripts"]}

        for required in [
            "LittlerootTown_EventScript_StepOffTruckMale",
            "LittlerootTown_EventScript_StepOffTruckFemale",
            "PlayersHouse_2F_EventScript_WallClock",
            "LittlerootTown_MaysHouse_2F_EventScript_MeetMay",
            "LittlerootTown_BrendansHouse_2F_EventScript_MeetBrendan",
            "Route101_EventScript_StartBirchRescue",
            "Route101_EventScript_BirchsBag",
            "LittlerootTown_ProfessorBirchsLab_EventScript_GiveStarterEvent",
        ]:
            self.assertIn(required, script_ids)

        specials = {item["special_id"] for item in ir["specials"]}
        self.assertTrue(
            {"ChooseStarter", "HealPlayerParty", "StartWallClock"}.issubset(specials)
        )

        generated = generate_c_fixture(
            ir,
            SOURCE,
            symbol_prefix="gR2Littleroot",
        )
        self.assertIn("gR2LittlerootRegistry", generated)
        self.assertIn('"Route101_EventScript_BirchsBag"', generated)
        self.assertIn('"LittlerootTown_ProfessorBirchsLab_EventScript_GiveStarterEvent"', generated)


if __name__ == "__main__":
    unittest.main()
