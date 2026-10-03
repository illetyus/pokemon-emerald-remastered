import tempfile
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from generate_script_c import (  # noqa: E402
    ScriptCGenerationError,
    build_numeric_resolver,
    generate_c_fixture,
)


class GenerateScriptCTests(unittest.TestCase):
    def make_source_tree(self, root: Path) -> None:
        (root / "include/constants").mkdir(parents=True)
        (root / "data/maps/TestTown").mkdir(parents=True)

        (root / "include/constants/vars.h").write_text(
            """
#define VAR_TEST 0x4020
#define VAR_0x8004 0x8004
#define VAR_0x8005 0x8005
#define VAR_RESULT 0x800D
""".strip()
            + "\n",
            encoding="utf-8",
        )
        (root / "include/constants/flags.h").write_text(
            """
#define SYSTEM_FLAGS 0x860
#define FLAG_TEST (SYSTEM_FLAGS + 0xF)
""".strip()
            + "\n",
            encoding="utf-8",
        )
        (root / "include/constants/map_groups.h").write_text(
            """
#define MAP_TEST_TOWN (3 | (2 << 8))
""".strip()
            + "\n",
            encoding="utf-8",
        )
        (root / "include/constants/global.h").write_text(
            """
#define FALSE 0
#define TRUE 1
#define MALE 0
#define FEMALE 1
""".strip()
            + "\n",
            encoding="utf-8",
        )
        (root / "data/maps/TestTown/scripts.inc").write_text(
            """
.set LOCALID_MOM, 5

Test_A::
    end

Test_B::
    return
""".strip()
            + "\n",
            encoding="utf-8",
        )

    def test_numeric_resolver_handles_defines_expressions_and_local_sets(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            resolver = build_numeric_resolver(root)

            self.assertEqual(resolver.resolve("VAR_TEST"), 0x4020)
            self.assertEqual(resolver.resolve("FLAG_TEST"), 0x86F)
            self.assertEqual(resolver.resolve("MAP_TEST_TOWN"), 0x0203)
            self.assertEqual(
                resolver.resolve(
                    "LOCALID_MOM",
                    Path("data/maps/TestTown/scripts.inc"),
                ),
                5,
            )
            self.assertEqual(resolver.resolve("FEMALE"), 1)
            self.assertEqual(resolver.resolve("-1"), -1)

    def test_fixture_generation_maps_semantic_ir_to_native_programs(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = {
                "schema_version": 1,
                "scripts": [
                    {
                        "script_id": "Test_A",
                        "source": {
                            "file": "data/maps/TestTown/scripts.inc",
                            "line": 3,
                        },
                        "instructions": [
                            {
                                "op": "SET_VAR",
                                "var": "VAR_TEST",
                                "value": "FEMALE",
                            },
                            {
                                "op": "SET_OBJECT_XY_PERM",
                                "local_id": "LOCALID_MOM",
                                "x": "3",
                                "y": "4",
                            },
                            {
                                "op": "OPEN_DOOR",
                                "x_operand": "VAR_0x8004",
                                "y_operand": "VAR_0x8005",
                            },
                            {
                                "op": "CALL",
                                "target_script_id": "Test_B",
                            },
                            {"op": "END"},
                        ],
                    },
                    {
                        "script_id": "Test_B",
                        "source": {
                            "file": "data/maps/TestTown/scripts.inc",
                            "line": 6,
                        },
                        "instructions": [
                            {"op": "CHECK_PLAYER_GENDER", "result_var": "VAR_RESULT"},
                            {"op": "RETURN"},
                        ],
                    },
                ],
                "map_scripts": [
                    {
                        "owner_id": "Test_MapScripts",
                        "kind": "MAP_SCRIPT_ON_TRANSITION",
                        "script_id": "Test_A",
                    },
                    {
                        "owner_id": "Test_MapScripts",
                        "kind": "MAP_SCRIPT_ON_FRAME_TABLE",
                        "script_id": "Test_OnFrame",
                    },
                ],
                "map_script_tables": [
                    {
                        "owner_id": "Test_OnFrame",
                        "var": "VAR_TEST",
                        "value": "FEMALE",
                        "script_id": "Test_B",
                    },
                ],
                "specials": [],
                "movements": [],
                "texts": [],
            }

            generated = generate_c_fixture(
                ir,
                root,
                symbol_prefix="gR2Test",
            )

            self.assertIn("REMASTER_EMERALD_SCRIPT_SET_VAR", generated)
            self.assertIn(".a = 0x4020u", generated)
            self.assertIn(".b = 0x0001u", generated)
            self.assertIn("REMASTER_EMERALD_SCRIPT_SET_OBJECT_XY_PERM", generated)
            self.assertIn(".a = 0x0005u", generated)
            self.assertIn(".x = 3", generated)
            self.assertIn(".y = 4", generated)
            self.assertIn("REMASTER_EMERALD_SCRIPT_OPEN_DOOR", generated)
            self.assertIn(".a = 0x8004u", generated)
            self.assertIn(".b = 0x8005u", generated)
            self.assertIn(".target_program = 1u", generated)
            self.assertIn(".target_program_valid = 1u", generated)
            self.assertIn("REMASTER_EMERALD_SCRIPT_CHECK_PLAYER_GENDER", generated)
            self.assertIn("REMASTER_EMERALD_MAP_SCRIPT_ON_TRANSITION", generated)
            self.assertIn("REMASTER_EMERALD_MAP_SCRIPT_ON_FRAME_TABLE", generated)
            self.assertIn(".lhs = 0x4020u", generated)
            self.assertIn(".rhs = 0x0001u", generated)
            self.assertIn(
                'const RemasterEmeraldScriptRegistry gR2TestRegistry',
                generated,
            )
            self.assertIn(
                'const RemasterEmeraldMapScriptEntry gR2TestMapScripts[]',
                generated,
            )

    def test_empty_map_script_set_emits_portable_zero_count_storage(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = {
                "schema_version": 1,
                "scripts": [
                    {
                        "script_id": "Test_A",
                        "source": {
                            "file": "data/maps/TestTown/scripts.inc",
                            "line": 3,
                        },
                        "instructions": [{"op": "END"}],
                    }
                ],
                "map_scripts": [],
                "map_script_tables": [],
                "specials": [],
                "movements": [],
                "texts": [],
            }

            generated = generate_c_fixture(
                ir,
                root,
                symbol_prefix="gR2Empty",
            )

            self.assertNotIn("gR2EmptyMapScripts[] = {\n};", generated)
            self.assertIn(
                "const RemasterEmeraldMapScriptEntry gR2EmptyMapScripts[1] = { { 0 } };",
                generated,
            )
            self.assertIn(
                "const size_t gR2EmptyMapScriptCount = 0u;",
                generated,
            )

    def test_unknown_ir_opcode_is_a_hard_generation_error(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.make_source_tree(root)
            ir = {
                "schema_version": 1,
                "scripts": [
                    {
                        "script_id": "Test_A",
                        "source": {
                            "file": "data/maps/TestTown/scripts.inc",
                            "line": 3,
                        },
                        "instructions": [{"op": "NOT_A_REAL_OPCODE"}],
                    }
                ],
                "map_scripts": [],
                "map_script_tables": [],
                "specials": [],
                "movements": [],
                "texts": [],
            }

            with self.assertRaises(ScriptCGenerationError) as raised:
                generate_c_fixture(ir, root, symbol_prefix="gR2Test")

            self.assertIn("Test_A", str(raised.exception))
            self.assertIn("NOT_A_REAL_OPCODE", str(raised.exception))

    def test_numeric_resolver_has_emerald_boolean_constants(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "include").mkdir(parents=True)
            resolver = build_numeric_resolver(root)
            self.assertEqual(resolver.resolve("TRUE"), 1)
            self.assertEqual(resolver.resolve("FALSE"), 0)

    def test_numeric_resolver_reads_assembler_assignments(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            asm = root / "asm/macros/event.inc"
            asm.parent.mkdir(parents=True)
            asm.write_text(
                "MSGBOX_DEFAULT = 4\nMSGBOX_YESNO = 5\nYES = 1\nNO = 0\n",
                encoding="utf-8",
            )
            resolver = build_numeric_resolver(root)
            self.assertEqual(resolver.resolve("MSGBOX_DEFAULT"), 4)
            self.assertEqual(resolver.resolve("MSGBOX_YESNO"), 5)
            self.assertEqual(resolver.resolve("YES"), 1)
            self.assertEqual(resolver.resolve("NO"), 0)

    def test_numeric_resolver_has_event_string_var_operands(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "include").mkdir(parents=True)
            resolver = build_numeric_resolver(root)
            self.assertEqual(resolver.resolve("STR_VAR_1"), 0)
            self.assertEqual(resolver.resolve("STR_VAR_2"), 1)
            self.assertEqual(resolver.resolve("STR_VAR_3"), 2)


if __name__ == "__main__":
    unittest.main()
