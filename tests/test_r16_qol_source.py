import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

class R16QolSourceAudit(unittest.TestCase):
    def test_pinned_vanillaplus_contracts_exist(self):
        phase03 = read("vendor/vanillaplus/tools/vanillaplus_phase03_verify.py")
        phase04 = read("vendor/vanillaplus/tools/vanillaplus_phase04_verify.py")
        phase05 = read("vendor/vanillaplus/tools/vanillaplus_phase05_verify.py")
        phase06 = read("vendor/vanillaplus/tools/vanillaplus_phase06_verify.py")
        phase08 = read("vendor/vanillaplus/tools/vanillaplus_phase08_verify.py")
        phase09 = read("vendor/vanillaplus/tools/vanillaplus_phase09_verify.py")
        phase10 = read("vendor/vanillaplus/tools/vanillaplus_phase10a_verify.py")
        pc = read("vendor/vanillaplus/tools/vanillaplus_pc_management_verify.py")

        for token in [
            "FLAG_SYS_B_DASH", "PlaySE(SE_LOW_HEALTH)", "hp > 1",
            "setflashlevel 0", "TrySwitchBikeType"
        ]:
            self.assertIn(token, phase03)

        for token in ["HasVanillaPlusHmAccess", "ITEM_HM01_CUT", "ITEM_HM08_DIVE"]:
            self.assertIn(token, phase04)

        for token in [
            "VANILLAPLUS_QUICK_ITEM_MAX", "unused_3598",
            "VanillaPlusSortBagPocket"
        ]:
            self.assertIn(token, phase05)

        for token in ["hidden power", "heart scale", "nickname"]:
            self.assertIn(token, phase06.lower())

        self.assertIn("RealignTimeBasedEventsAfterRtcCorrection", phase08)
        self.assertIn("386", phase09)
        self.assertIn("quest", phase10.lower())

        for token in [
            "SortCurrentBox(BOX_SORT_SPECIES)",
            "SortCurrentBox(BOX_SORT_LEVEL)",
            "SortCurrentBox(BOX_SORT_TYPE)",
            "CompactCurrentBox()",
            "INPUT_QUICK_DEPOSIT"
        ]:
            self.assertIn(token, pc)

    def test_already_owned_features_stay_in_existing_phases(self):
        self.assertIn("RTC correction updates", read("docs/R1_SAVE_RTC.md"))
        self.assertIn("32", read("docs/R10_QUEST_MAP.md"))
        r12 = read("docs/R12_ENCOUNTER_SYSTEM.md")
        self.assertIn("386-species", r12)
        self.assertIn("shuffled species-bag", r12)

    def test_portable_qol_does_not_expand_save_layout(self):
        header = read("core/include/remaster/emerald_qol.h")
        source = read("core/src/emerald_qol.c")
        save_header = read("core/include/remaster/emerald_save.h")

        self.assertIn("remaster_emerald_qol_hm_access", header)
        self.assertIn("REMASTER_EMERALD_QOL_TRANSFER_LAST_USABLE", header)
        self.assertIn("VANILLAPLUS_ITEM_META_MAGIC", source)
        self.assertIn("0x3598", source)
        self.assertIn("TYPE_MYSTERY", source)
        self.assertNotIn("qol_", save_header.lower())
        self.assertIn("REMASTER_EMERALD_SAVE_BLOCK1_BYTES = 0x3DC8", save_header)
        self.assertIn("REMASTER_EMERALD_STORAGE_BYTES = 0x83D0", save_header)

if __name__ == "__main__":
    unittest.main()
