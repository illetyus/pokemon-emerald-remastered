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


if __name__ == "__main__":
    unittest.main()
