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
    convert_script_closure,
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
        handlers[0x21] = "ScrCmd_compare_var_to_value"
        handlers[0x25] = "ScrCmd_special"
        handlers[0x27] = "ScrCmd_waitstate"
        handlers[0x29] = "ScrCmd_setflag"
        handlers[0x2A] = "ScrCmd_clearflag"
        handlers[0x2B] = "ScrCmd_checkflag"
        handlers[0x4F] = "ScrCmd_applymovement"
        handlers[0x51] = "ScrCmd_waitmovement"
        handlers[0x66] = "ScrCmd_waitmessage"
        handlers[0x67] = "ScrCmd_message"
        handlers[0x68] = "ScrCmd_closemessage"
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
.macro checkflag flag:req
.endm
.macro applymovement localId:req, movements:req, map
.endm
.macro waitmovement localId:req, map
.endm
.macro checkplayergender
.endm
.macro opendoor x:req, y:req
.endm
.macro setobjectxyperm localId:req, x:req, y:req
.endm
.macro lockall
.endm
.macro releaseall
.endm
.macro setmetatile x:req, y:req, metatileId:req, impassable:req
.endm
.macro savebgm song:req
.endm
.macro fadedefaultbgm
.endm
.macro incrementgamestat stat:req
.endm
.macro bufferleadmonspeciesname stringVarId:req
.endm
.macro setfollower localId:req, flags:req
.endm
.macro map_script kind:req, script:req
.endm
.macro map_script_2 var:req, value:req, script:req
.endm
.macro goto_if_eq a:req, b, c
.endm
.macro call_if_unset flag:req, dest:req
.endm
.macro call_if_lt a:req, b, c
.endm
.macro msgbox text:req, type=MSGBOX_DEFAULT
.endm
.macro followerintopokeball
.endm
.macro updatefollowerpokemongraphic
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
    map_script MAP_SCRIPT_ON_FRAME_TABLE, TestTown_OnFrame
    .byte 0

TestTown_OnFrame:
    map_script_2 VAR_TEST, 1, TestTown_Final
    .2byte 0

TestTown_OnTransition:
    call Shared_EventScript_Helper
    goto_if_eq VAR_TEST, 1, TestTown_Final
    call_if_unset FLAG_OTHER, TestTown_Final
    checkplayergender
    setobjectxyperm VAR_0x8004, 3, 4
    opendoor VAR_0x8004, VAR_0x8005
    lockall
    setmetatile VAR_0x8004, VAR_0x8005, METATILE_TEST, TRUE
    savebgm MUS_TEST
    fadedefaultbgm
    incrementgamestat GAME_STAT_TEST
    bufferleadmonspeciesname STR_VAR_1
    setfollower VAR_0x8004, 0x7E
    followerintopokeball
    updatefollowerpokemongraphic
    releaseall
    applymovement 1, TestTown_Movement_Walk
    msgbox TestTown_Text_Hello, MSGBOX_DEFAULT
    setflag FLAG_TEST
    followerintopokeball
    updatefollowerpokemongraphic
    end

TestTown_Final:
    end

TestTown_Movement_Walk:
    walk_down
    step_end

TestTown_Text_Hello:
    .string "Hello!$"
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
            self.assertEqual(closure.commands["followerintopokeball"], "WORLD")
            self.assertEqual(
                closure.commands["updatefollowerpokemongraphic"],
                "WORLD",
            )

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


class ConvertScriptsIrTests(ConvertScriptsInventoryTests):
    def test_cross_script_call_and_special_ids_are_stable_labels(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )

            scripts = {item["script_id"]: item for item in ir["scripts"]}
            transition = scripts["TestTown_OnTransition"]
            call = next(ins for ins in transition["instructions"] if ins["op"] == "CALL")
            self.assertEqual(call["target_script_id"], "Shared_EventScript_Helper")

            shared = scripts["Shared_EventScript_Helper"]
            special = next(ins for ins in shared["instructions"] if ins["op"] == "SPECIAL")
            self.assertEqual(special["special_id"], "HealPlayerParty")
            self.assertEqual(special["special_index"], 0)

    def test_convenience_macros_expand_to_compare_check_and_conditional_flow(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )
            scripts = {item["script_id"]: item for item in ir["scripts"]}
            ops = scripts["TestTown_OnTransition"]["instructions"]

            eq_index = next(
                i for i, ins in enumerate(ops)
                if ins["op"] == "COMPARE_VAR_VALUE" and ins["var"] == "VAR_TEST"
            )
            self.assertEqual(ops[eq_index]["value"], "1")
            self.assertEqual(ops[eq_index + 1]["op"], "GOTO_IF")
            self.assertEqual(ops[eq_index + 1]["condition"], "EQUAL")
            self.assertEqual(ops[eq_index + 1]["target_script_id"], "TestTown_Final")

            flag_index = next(
                i for i, ins in enumerate(ops)
                if ins["op"] == "CHECK_FLAG" and ins["flag"] == "FLAG_OTHER"
            )
            self.assertEqual(ops[flag_index + 1]["op"], "CALL_IF")
            self.assertEqual(ops[flag_index + 1]["condition"], "FALSE")
            self.assertEqual(ops[flag_index + 1]["target_script_id"], "TestTown_Final")

    def test_opening_operands_have_typed_var_aware_ir(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )
            scripts = {item["script_id"]: item for item in ir["scripts"]}
            ops = scripts["TestTown_OnTransition"]["instructions"]

            self.assertIn(
                {"op": "CHECK_PLAYER_GENDER", "result_var": "VAR_RESULT"},
                ops,
            )
            self.assertIn(
                {
                    "op": "SET_OBJECT_XY_PERM",
                    "local_id": "VAR_0x8004",
                    "x": "3",
                    "y": "4",
                },
                ops,
            )
            self.assertIn(
                {
                    "op": "OPEN_DOOR",
                    "x_operand": "VAR_0x8004",
                    "y_operand": "VAR_0x8005",
                },
                ops,
            )

            self.assertIn({"op": "LOCK_ALL"}, ops)
            self.assertIn(
                {
                    "op": "SET_METATILE",
                    "x_operand": "VAR_0x8004",
                    "y_operand": "VAR_0x8005",
                    "metatile": "METATILE_TEST",
                    "impassable": "TRUE",
                },
                ops,
            )
            self.assertIn(
                {"op": "SAVE_BGM", "song": "MUS_TEST"},
                ops,
            )
            self.assertIn({"op": "FADE_DEFAULT_BGM"}, ops)
            self.assertIn(
                {"op": "INCREMENT_GAME_STAT", "stat": "GAME_STAT_TEST"},
                ops,
            )
            self.assertIn(
                {
                    "op": "BUFFER_LEAD_MON_SPECIES_NAME",
                    "string_var": "STR_VAR_1",
                },
                ops,
            )
            self.assertIn(
                {
                    "op": "SET_FOLLOWER",
                    "local_id": "VAR_0x8004",
                    "flags": "0x7E",
                },
                ops,
            )
            self.assertIn({"op": "FOLLOWER_INTO_POKEBALL"}, ops)
            self.assertIn(
                {"op": "UPDATE_FOLLOWER_POKEMON_GRAPHIC"},
                ops,
            )
            self.assertIn({"op": "RELEASE_ALL"}, ops)

    def test_map_tables_movements_and_text_have_separate_stable_identity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )

            self.assertIn(
                {
                    "owner_id": "TestTown_MapScripts",
                    "kind": "MAP_SCRIPT_ON_TRANSITION",
                    "script_id": "TestTown_OnTransition",
                },
                ir["map_scripts"],
            )
            self.assertIn(
                {
                    "owner_id": "TestTown_OnFrame",
                    "var": "VAR_TEST",
                    "value": "1",
                    "script_id": "TestTown_Final",
                },
                ir["map_script_tables"],
            )

            movements = {item["movement_id"]: item for item in ir["movements"]}
            self.assertEqual(
                [step["op"] for step in movements["TestTown_Movement_Walk"]["steps"]],
                ["walk_down", "step_end"],
            )
            texts = {item["text_id"]: item for item in ir["texts"]}
            self.assertEqual(texts["TestTown_Text_Hello"]["strings"], ["Hello!$"])


    def test_explicit_entry_labels_limit_acceptance_closure(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            script_path = root / "data/maps/TestTown/scripts.inc"
            script_path.write_text(
                script_path.read_text(encoding="utf-8")
                + """
TestTown_PostgameOnly::
    special ChooseStarter
    end
""",
                encoding="utf-8",
            )

            ir = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
                entry_labels=["TestTown_OnTransition"],
            )
            script_ids = {item["script_id"] for item in ir["scripts"]}
            self.assertIn("TestTown_OnTransition", script_ids)
            self.assertIn("Shared_EventScript_Helper", script_ids)
            self.assertNotIn("TestTown_PostgameOnly", script_ids)


    def test_external_text_sources_resolve_without_becoming_script_entries(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            (root / "data/text").mkdir(parents=True)
            (root / "data/text/common.inc").write_text(
                'External_Text_FromInclude::\n    .string "From include$"\n',
                encoding="utf-8",
            )
            (root / "data/event_scripts.s").write_text(
                'External_Text_FromEventScripts::\n    .string "From event scripts$"\n',
                encoding="utf-8",
            )
            script_path = root / "data/maps/TestTown/scripts.inc"
            script_path.write_text(
                script_path.read_text(encoding="utf-8")
                + """
TestTown_ExternalText::
    msgbox External_Text_FromInclude, MSGBOX_DEFAULT
    msgbox External_Text_FromEventScripts, MSGBOX_DEFAULT
    end
""",
                encoding="utf-8",
            )

            ir = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
                entry_labels=["TestTown_ExternalText"],
            )
            texts = {item["text_id"]: item for item in ir["texts"]}
            self.assertEqual(
                texts["External_Text_FromInclude"]["strings"],
                ["From include$"],
            )
            self.assertEqual(
                texts["External_Text_FromEventScripts"]["strings"],
                ["From event scripts$"],
            )
            script_ids = {item["script_id"] for item in ir["scripts"]}
            self.assertNotIn("External_Text_FromInclude", script_ids)
            self.assertNotIn("External_Text_FromEventScripts", script_ids)

    def test_ir_emits_ordered_special_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )

            self.assertEqual(
                ir["specials"],
                [
                    {"special_id": "HealPlayerParty", "special_index": 0},
                    {"special_id": "SetCableClubWarp", "special_index": 1},
                    {"special_id": "ChooseStarter", "special_index": 2},
                ],
            )

    def test_ir_output_is_deterministic(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            first = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )
            second = convert_script_closure(
                root,
                [Path("data/maps/TestTown/scripts.inc")],
            )
            self.assertEqual(
                json.dumps(first, sort_keys=True, ensure_ascii=False),
                json.dumps(second, sort_keys=True, ensure_ascii=False),
            )

    def test_unresolved_script_target_is_a_conversion_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            script_path = root / "data/maps/TestTown/scripts.inc"
            script_path.write_text(
                script_path.read_text(encoding="utf-8").replace(
                    "call Shared_EventScript_Helper",
                    "call Missing_EventScript",
                ),
                encoding="utf-8",
            )

            with self.assertRaises(ScriptConversionError) as raised:
                convert_script_closure(
                    root,
                    [Path("data/maps/TestTown/scripts.inc")],
                )
            self.assertIn("Missing_EventScript", str(raised.exception))


if __name__ == "__main__":
    unittest.main()
