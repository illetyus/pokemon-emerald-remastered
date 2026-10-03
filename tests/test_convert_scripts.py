import json
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
    def make_source_tree(self, root: Path) -> None:
        (root / "data").mkdir(parents=True)
        (root / "asm/macros").mkdir(parents=True)
        (root / "data/maps/TestTown").mkdir(parents=True)
        (root / "data/scripts").mkdir(parents=True)

        handlers = [f"ScrCmd_cmd_{i:02x}" for i in range(0xE8)]
        handlers[0x00] = "ScrCmd_nop"
        handlers[0x02] = "ScrCmd_end"
        handlers[0x03] = "ScrCmd_return"
        handlers[0x04] = "ScrCmd_call"
        handlers[0x05] = "ScrCmd_goto"
        handlers[0x06] = "ScrCmd_goto_if"
        handlers[0x07] = "ScrCmd_call_if"
        handlers[0x16] = "ScrCmd_setvar"
        handlers[0x25] = "ScrCmd_special"
        handlers[0x27] = "ScrCmd_waitstate"
        handlers[0x29] = "ScrCmd_setflag"
        handlers[0x2A] = "ScrCmd_clearflag"
        handlers[0x4F] = "ScrCmd_applymovement"
        handlers[0x51] = "ScrCmd_waitmovement"
        handlers[0xA0] = "ScrCmd_checkplayergender"

        lines = ["\t.align 2", "gScriptCmdTable::"]
        for opcode, handler in enumerate(handlers):
            lines.append(f"\t.4byte {handler:<40} @ 0x{opcode:02x}")
        lines.extend(["gScriptCmdTableEnd::", "\t.4byte ScrCmd_nop"])
        (root / "data/script_cmd_table.inc").write_text(
            "\n".join(lines) + "\n",
            encoding="utf-8",
        )

        (root / "asm/macros/event.inc").write_text(
            """
.macro call destination:req
.endm
.macro goto destination:req
.endm
.macro goto_if condition:req, destination:req
.endm
.macro call_if condition:req, destination:req
.endm
.macro setvar destination:req, value:req
.endm
.macro special function:req
.endm
.macro waitstate
.endm
.macro setflag flag:req
.endm
.macro clearflag flag:req
.endm
.macro applymovement localId:req, movements:req, map
.endm
.macro waitmovement localId:req, map
.endm
.macro checkplayergender
.endm
.macro map_script kind:req, script:req
.endm
""".strip()
            + "\n",
            encoding="utf-8",
        )

        (root / "data/specials.inc").write_text(
            """
.set __special__, 0
gSpecials::
    def_special HealPlayerParty
    def_special SetCableClubWarp
    def_special ChooseStarter
""".strip()
            + "\n",
            encoding="utf-8",
        )

        (root / "data/maps/TestTown/scripts.inc").write_text(
            """
TestTown_MapScripts::
    map_script MAP_SCRIPT_ON_TRANSITION, TestTown_OnTransition
    .byte 0

TestTown_OnTransition:
    call Shared_EventScript_Helper
    setflag FLAG_TEST
    end
""".strip()
            + "\n",
            encoding="utf-8",
        )

        (root / "data/scripts/shared.inc").write_text(
            """
Shared_EventScript_Helper::
    setvar VAR_TEST, 1
    special HealPlayerParty
    return
""".strip()
            + "\n",
            encoding="utf-8",
        )

    def test_opcode_table_is_complete_and_pins_known_vanilla_slots(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)

            inventory = build_command_inventory(root)

            self.assertEqual(len({spec.opcode for spec in inventory.values()}), 0xE8)
            self.assertEqual(inventory["setvar"].opcode, 0x16)
            self.assertEqual(inventory["special"].opcode, 0x25)
            self.assertEqual(inventory["applymovement"].opcode, 0x4F)
            self.assertEqual(inventory["waitmovement"].opcode, 0x51)
            self.assertEqual(inventory["checkplayergender"].opcode, 0xA0)

    def test_special_inventory_preserves_source_order(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)

            specials = build_special_inventory(root)

            self.assertEqual(specials["HealPlayerParty"].special_id, 0)
            self.assertEqual(specials["SetCableClubWarp"].special_id, 1)
            self.assertEqual(specials["ChooseStarter"].special_id, 2)

    def test_dependency_closure_follows_cross_file_script_calls(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)

            closure = collect_script_dependency_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )

            self.assertIn("TestTown_OnTransition", closure.labels)
            self.assertIn("Shared_EventScript_Helper", closure.labels)
            self.assertIn(
                "data/scripts/shared.inc",
                {str(path).replace("\\", "/") for path in closure.source_files},
            )
            self.assertEqual(closure.commands["setflag"], "CORE")
            self.assertEqual(closure.commands["special"], "SPECIAL_ADAPTER")

    def test_reachable_unclassified_command_is_a_source_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            script_path = root / "data/maps/TestTown/scripts.inc"
            script_path.write_text(
                script_path.read_text(encoding="utf-8")
                + """
TestTown_BadScript::
    mysterycommand 7
    end
""",
                encoding="utf-8",
            )

            with self.assertRaises(ScriptConversionError) as raised:
                collect_script_dependency_closure(
                    root,
                    [Path("data/maps/TestTown/scripts.inc")],
                    entry_labels=["TestTown_BadScript"],
                )

            message = str(raised.exception)
            self.assertIn("data/maps/TestTown/scripts.inc", message)
            self.assertIn("TestTown_BadScript", message)
            self.assertIn("mysterycommand", message)
            self.assertRegex(message, r":\d+:")


if __name__ == "__main__":
    unittest.main()
