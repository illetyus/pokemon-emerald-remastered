import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from convert_scripts import (  # noqa: E402
    ScriptConversionError,
    build_command_inventory,
    build_special_inventory,
    collect_script_dependency_closure,
    convert_script_closure,
)


class ConvertScriptsInventoryTests(unittest.TestCase):
    def make_source(self):
        temp = tempfile.TemporaryDirectory()
        root = Path(temp.name)
        (root / "data/maps/LittlerootTown").mkdir(parents=True)
        (root / "data/maps/Route101").mkdir(parents=True)
        (root / "data/scripts").mkdir(parents=True)
        (root / "data").mkdir(exist_ok=True)
        (root / "asm/macros").mkdir(parents=True)

        table = []
        names = {i: "ScrCmd_nop1" for i in range(0xE8)}
        names.update({
            0x00: "ScrCmd_nop",
            0x16: "ScrCmd_setvar",
            0x25: "ScrCmd_special",
            0x4F: "ScrCmd_applymovement",
            0x51: "ScrCmd_waitmovement",
            0xA0: "ScrCmd_checkplayergender",
            0xE7: "ScrCmd_isfollowervisible",
        })
        table.append("gScriptCmdTable::")
        for opcode in range(0xE8):
            table.append(f"    .4byte {names[opcode]} @ 0x{opcode:02x}")
        table.append("gScriptCmdTableEnd::")
        table.append("    .4byte ScrCmd_nop")
        (root / "data/script_cmd_table.inc").write_text("\n".join(table), encoding="utf-8")

        (root / "asm/macros/event.inc").write_text(
            """
.macro setvar destination:req, value:req
.macro special function:req
.macro applymovement localId:req, movements:req, map
.macro waitmovement localId:req, map
.macro checkplayergender
.macro call destination:req
.macro end
.macro return
.macro call_if_eq a:req, b, c
.macro call_if_unset flag:req, dest:req
.macro goto_if_eq a:req, b, c
.macro call_if_lt a:req, b, c
.macro map_script kind:req, script:req
.macro map_script_2 var:req, value:req, script:req
.macro followerintopokeball
.macro updatefollowerpokemongraphic
""",
            encoding="utf-8",
        )
        (root / "data/specials.inc").write_text(
            """
.set __special__, 0
 gSpecials::
 def_special HealPlayerParty
 def_special ChooseStarter
 def_special GetPlayerBigGuyGirlString
""",
            encoding="utf-8",
        )
        (root / "data/maps/LittlerootTown/scripts.inc").write_text(
            """
LittlerootTown_OnTransition:
    setvar VAR_TEST, 1
    call Common_EventScript_SetupRivalGfxId
    call_if_unset FLAG_TEST, Common_EventScript_Helper
    special GetPlayerBigGuyGirlString
    followerintopokeball
    updatefollowerpokemongraphic
    end

LittlerootTown_Movement_Player:
    walk_down
    step_end
""",
            encoding="utf-8",
        )
        (root / "data/maps/Route101/scripts.inc").write_text(
            """
Route101_Start:
    special ChooseStarter
    applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_Walk
    waitmovement 0
    end
""",
            encoding="utf-8",
        )
        (root / "data/scripts/common.inc").write_text(
            """
Common_EventScript_SetupRivalGfxId:
    checkplayergender
    return
Common_EventScript_Helper:
    setvar VAR_HELPER, 2
    return
Common_Movement_Walk:
    walk_up
    step_end
""",
            encoding="utf-8",
        )
        return temp, root

    def test_build_command_inventory_pins_authoritative_opcode_slots(self):
        temp, root = self.make_source()
        self.addCleanup(temp.cleanup)
        inventory = build_command_inventory(root)
        self.assertEqual(len(inventory), 0xE8)
        self.assertEqual(inventory["setvar"].opcode, 0x16)
        self.assertEqual(inventory["special"].opcode, 0x25)
        self.assertEqual(inventory["applymovement"].opcode, 0x4F)
        self.assertEqual(inventory["waitmovement"].opcode, 0x51)
        self.assertEqual(inventory["checkplayergender"].opcode, 0xA0)
        self.assertEqual(max(spec.opcode for spec in inventory.values()), 0xE7)

    def test_build_special_inventory_preserves_source_order(self):
        temp, root = self.make_source()
        self.addCleanup(temp.cleanup)
        specials = build_special_inventory(root)
        self.assertEqual(specials["HealPlayerParty"].index, 0)
        self.assertEqual(specials["ChooseStarter"].index, 1)
        self.assertEqual(specials["GetPlayerBigGuyGirlString"].index, 2)

    def test_collect_dependency_closure_follows_shared_script_labels(self):
        temp, root = self.make_source()
        self.addCleanup(temp.cleanup)
        closure = collect_script_dependency_closure(
            root,
            [
                root / "data/maps/LittlerootTown/scripts.inc",
                root / "data/maps/Route101/scripts.inc",
            ],
        )
        rel = {path.relative_to(root).as_posix() for path in closure.files}
        self.assertIn("data/maps/LittlerootTown/scripts.inc", rel)
        self.assertIn("data/maps/Route101/scripts.inc", rel)
        self.assertIn("data/scripts/common.inc", rel)
        self.assertIn("Common_EventScript_SetupRivalGfxId", closure.labels)
        self.assertIn("Common_Movement_Walk", closure.labels)
        self.assertIn("followerintopokeball", closure.commands)
        self.assertIn("updatefollowerpokemongraphic", closure.commands)

    def test_reachable_event_macro_without_classification_is_explicit_error(self):
        temp, root = self.make_source()
        self.addCleanup(temp.cleanup)
        with (root / "asm/macros/event.inc").open("a", encoding="utf-8") as handle:
            handle.write("\n.macro brandnewcmd value:req\n")
        with (root / "data/maps/LittlerootTown/scripts.inc").open("a", encoding="utf-8") as handle:
            handle.write("\nBroken_EventScript:\n    brandnewcmd 7\n    end\n")

        with self.assertRaises(ScriptConversionError) as caught:
            collect_script_dependency_closure(
                root,
                [root / "data/maps/LittlerootTown/scripts.inc"],
            )
        message = str(caught.exception)
        self.assertIn("data/maps/LittlerootTown/scripts.inc", message)
        self.assertIn("Broken_EventScript", message)
        self.assertIn("brandnewcmd", message)
        self.assertIn("line", message)


class ConvertScriptsIrTests(ConvertScriptsInventoryTests):
    def test_convert_script_closure_resolves_cross_script_targets_and_stable_ids(self):
        temp, root = self.make_source()
        self.addCleanup(temp.cleanup)
        with (root / "data/maps/LittlerootTown/scripts.inc").open("w", encoding="utf-8") as handle:
            handle.write(
                """
LittlerootTown_MapScripts::
    map_script MAP_SCRIPT_ON_TRANSITION, LittlerootTown_OnTransition
    map_script MAP_SCRIPT_ON_FRAME_TABLE, LittlerootTown_OnFrame
    .byte 0

LittlerootTown_OnFrame:
    map_script_2 VAR_TEST, 1, LittlerootTown_FrameScript
    .2byte 0

LittlerootTown_OnTransition:
    setvar VAR_TEST, 1
    call Common_EventScript_Helper
    goto_if_eq VAR_TEST, 1, LittlerootTown_FrameScript
    end

LittlerootTown_FrameScript:
    call_if_unset FLAG_TEST, Common_EventScript_Helper
    call_if_lt VAR_TEST, 3, Common_EventScript_Helper
    special GetPlayerBigGuyGirlString
    end

LittlerootTown_Movement_Test:
    walk_up
    step_end

LittlerootTown_Text_Test:
    .string "Hello$"
"""
            )

        ir = convert_script_closure(
            root, [root / "data/maps/LittlerootTown/scripts.inc"]
        )
        scripts = {item["script_id"]: item for item in ir["scripts"]}
        self.assertIn("LittlerootTown_OnTransition", scripts)
        self.assertIn("Common_EventScript_Helper", scripts)
        self.assertIn("LittlerootTown_FrameScript", scripts)

        transition = scripts["LittlerootTown_OnTransition"]["instructions"]
        self.assertEqual(transition[1]["op"], "CALL")
        self.assertEqual(transition[1]["target_script_id"], "Common_EventScript_Helper")
        self.assertEqual(transition[2]["op"], "COMPARE_VAR_VALUE")
        self.assertEqual(transition[3]["op"], "GOTO_IF")
        self.assertEqual(transition[3]["condition"], "EQUAL")
        self.assertEqual(transition[3]["target_script_id"], "LittlerootTown_FrameScript")

        frame = scripts["LittlerootTown_FrameScript"]["instructions"]
        self.assertEqual(frame[0]["op"], "CHECK_FLAG")
        self.assertEqual(frame[1]["op"], "CALL_IF")
        self.assertEqual(frame[1]["condition"], "UNSET")
        self.assertEqual(frame[2]["op"], "COMPARE_VAR_VALUE")
        self.assertEqual(frame[3]["condition"], "LESS")

        self.assertEqual(ir["movements"][0]["movement_id"], "LittlerootTown_Movement_Test")
        self.assertEqual(ir["movements"][0]["steps"], ["walk_up", "step_end"])
        self.assertEqual(ir["texts"][0]["text_id"], "LittlerootTown_Text_Test")

    def test_convert_script_closure_emits_map_script_tables(self):
        temp, root = self.make_source()
        self.addCleanup(temp.cleanup)
        with (root / "data/maps/LittlerootTown/scripts.inc").open("w", encoding="utf-8") as handle:
            handle.write(
                """
LittlerootTown_MapScripts::
    map_script MAP_SCRIPT_ON_TRANSITION, LittlerootTown_OnTransition
    map_script MAP_SCRIPT_ON_FRAME_TABLE, LittlerootTown_OnFrame
    .byte 0
LittlerootTown_OnFrame:
    map_script_2 VAR_TEST, 1, LittlerootTown_FrameScript
    .2byte 0
LittlerootTown_OnTransition:
    end
LittlerootTown_FrameScript:
    end
"""
            )
        ir = convert_script_closure(root, [root / "data/maps/LittlerootTown/scripts.inc"])
        hooks = ir["map_scripts"]
        self.assertIn(
            {"kind": "MAP_SCRIPT_ON_TRANSITION", "script_id": "LittlerootTown_OnTransition"},
            hooks,
        )
        tables = ir["map_script_tables"]
        self.assertEqual(tables[0]["kind"], "MAP_SCRIPT_ON_FRAME_TABLE")
        self.assertEqual(tables[0]["entries"][0]["var"], "VAR_TEST")
        self.assertEqual(tables[0]["entries"][0]["value"], "1")
        self.assertEqual(tables[0]["entries"][0]["script_id"], "LittlerootTown_FrameScript")

    def test_convert_script_closure_is_deterministic_and_rejects_unresolved_target(self):
        temp, root = self.make_source()
        self.addCleanup(temp.cleanup)
        first = convert_script_closure(root, [root / "data/maps/LittlerootTown/scripts.inc"])
        second = convert_script_closure(root, [root / "data/maps/LittlerootTown/scripts.inc"])
        self.assertEqual(first, second)

        path = root / "data/maps/LittlerootTown/scripts.inc"
        path.write_text("Bad::\n    call Missing_Label\n    end\n", encoding="utf-8")
        with self.assertRaises(ScriptConversionError) as caught:
            convert_script_closure(root, [path])
        self.assertIn("Missing_Label", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
