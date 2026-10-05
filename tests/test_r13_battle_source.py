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
            "src/battle_ai_script_commands.c",
            "src/battle_ai_switch_items.c",
            "src/battle_setup.c",
            "src/data/trainers.h",
            "src/data/trainer_parties.h",
            "data/battle_ai_scripts.s",
            "src/pokemon.c",
            "include/data.h",
            "include/constants/battle.h",
            "include/constants/battle_ai.h",
            "include/constants/battle_move_effects.h",
            "include/constants/trainers.h",
        ):
            self.assertTrue((VENDOR / rel).is_file(), rel)

    def test_trainer_contract_sources_match_gen3_shapes(self):
        data_h = (VENDOR / "include/data.h").read_text(encoding="utf-8")
        ai_h = (VENDOR / "include/constants/battle_ai.h").read_text(
            encoding="utf-8"
        )
        trainers = (VENDOR / "src/data/trainers.h").read_text(encoding="utf-8")

        self.assertIn("struct TrainerMonNoItemDefaultMoves", data_h)
        self.assertIn("struct TrainerMonItemCustomMoves", data_h)
        self.assertIn("struct Trainer", data_h)
        self.assertIn("#define MAX_TRAINER_ITEMS 4", data_h)
        self.assertIn("#define AI_SCRIPT_CHECK_BAD_MOVE", ai_h)
        self.assertIn("#define AI_SCRIPT_TRY_TO_FAINT", ai_h)
        self.assertIn("#define AI_SCRIPT_CHECK_VIABILITY", ai_h)
        self.assertIn(".doubleBattle =", trainers)
        self.assertIn(".aiFlags =", trainers)
        self.assertIn(".party =", trainers)

    def test_trainer_party_construction_matches_vanillaplus(self):
        text = (VENDOR / "src/battle_main.c").read_text(encoding="utf-8")
        self.assertIn("static u8 CreateNPCTrainerParty", text)
        self.assertIn(
            "fixedIV = partyData[i].iv * MAX_PER_STAT_IVS / 255;",
            text,
        )
        self.assertIn("personalityValue = 0x80;", text)
        self.assertIn("personalityValue = 0x78;", text)
        self.assertIn("personalityValue = 0x88;", text)
        self.assertIn("personalityValue += nameHash << 8;", text)
        self.assertIn("OT_ID_RANDOM_NO_SHINY", text)
        self.assertIn(
            "gBattleTypeFlags |= gTrainers[trainerNum].doubleBattle;",
            text,
        )

    def test_trainer_reward_and_end_lifecycle_are_pinned(self):
        battle_main = (VENDOR / "src/battle_main.c").read_text(
            encoding="utf-8"
        )
        commands = (VENDOR / "src/battle_script_commands.c").read_text(
            encoding="utf-8"
        )
        setup = (VENDOR / "src/battle_setup.c").read_text(encoding="utf-8")

        self.assertIn("const struct TrainerMoney gTrainerMoneyTable[]", battle_main)
        self.assertIn(
            "4 * lastMonLevel * gBattleStruct->moneyMultiplier",
            commands,
        )
        self.assertIn("SetBattledTrainersFlags();", setup)
        self.assertIn("SetMainCallback2(CB2_WhiteOut);", setup)

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

    def test_all_used_move_effects_have_portable_handlers(self):
        effects_text = (
            VENDOR / "include/constants/battle_move_effects.h"
        ).read_text(encoding="utf-8")
        effect_names = {
            int(match.group(2)): match.group(1)
            for match in re.finditer(
                r"^#define\s+(EFFECT_[A-Z0-9_]+)\s+(\d+)",
                effects_text,
                re.MULTILINE,
            )
        }

        catalog = (
            ROOT / "core" / "src" / "emerald_domain_catalog.inc"
        ).read_text(encoding="utf-8")
        start = catalog.index(
            "static const RemasterEmeraldMoveInfo"
        )
        end = catalog.index("};", start)
        used_effects = {
            int(match.group(2))
            for match in re.finditer(
                r"\{\s*(\d+)\s*,\s*(\d+)\s*,",
                catalog[start:end],
            )
        }

        r13 = R13.read_text(encoding="utf-8")
        handled = set(
            re.findall(r"case\s+(EFFECT_[A-Z0-9_]+)", r13)
        )
        handled.update(
            re.findall(r"effect\s*==\s*(EFFECT_[A-Z0-9_]+)", r13)
        )

        missing = [
            effect_names[effect]
            for effect in sorted(used_effects)
            if effect_names.get(effect) not in handled
        ]
        self.assertEqual(
            missing,
            [],
            "portable battle core is missing used move effects: "
            + ", ".join(missing),
        )

    def test_unreal_does_not_own_battle_math(self):
        r13 = R13.read_text(encoding="utf-8")
        self.assertIn("remaster_emerald_battle_calculate_damage", r13)
        self.assertIn("remaster_emerald_battle_resolve_turn", r13)


if __name__ == "__main__":
    unittest.main()
