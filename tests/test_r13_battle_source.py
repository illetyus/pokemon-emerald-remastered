import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor" / "vanillaplus"
R13 = ROOT / "core" / "src" / "emerald_battle.c"


class R13BattleSourceContract(unittest.TestCase):
    def test_pinned_battle_sources_exist(self):
        for rel in (
            "src/battle_main.c",
            "src/battle_util.c",
            "src/battle_script_commands.c",
            "src/pokemon.c",
            "include/constants/battle.h",
            "include/constants/battle_move_effects.h",
        ):
            self.assertTrue((VENDOR / rel).is_file(), rel)

    def test_move_effect_count_is_gen3_contract(self):
        text = (VENDOR / "include/constants/battle_move_effects.h").read_text(
            encoding="utf-8"
        )
        self.assertIn("#define NUM_BATTLE_MOVE_EFFECTS 214", text)

    def test_ultra_ball_vanillaplus_bonus_is_four_x(self):
        text = (VENDOR / "src/battle_script_commands.c").read_text(
            encoding="utf-8"
        )
        self.assertRegex(
            text,
            r"\[ITEM_ULTRA_BALL\s*-\s*ITEM_ULTRA_BALL\]\s*=\s*40",
        )
        r13 = R13.read_text(encoding="utf-8")
        self.assertRegex(
            r13,
            r"case\s+ITEM_ULTRA_BALL:\s*\n\s*return\s+40;",
        )

    def test_portable_rng_uses_emerald_lcrng_constants(self):
        r13 = R13.read_text(encoding="utf-8")
        self.assertIn("1103515245", r13)
        self.assertIn("24691", r13)

    def test_unreal_does_not_own_battle_math(self):
        r13 = R13.read_text(encoding="utf-8")
        self.assertIn("remaster_emerald_battle_calculate_damage", r13)
        self.assertIn("remaster_emerald_battle_resolve_turn", r13)


if __name__ == "__main__":
    unittest.main()
