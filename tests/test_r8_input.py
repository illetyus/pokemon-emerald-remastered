import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class R8PhysicalContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("R8 compiled source oracle requires g++")
        cls.probe = Path(cls.temp.name) / "r8-physical"
        subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                        "-I", str(ROOT / "unreal/Source/PokemonEmeraldRemastered"),
                        str(ROOT / "tests/r8_physical_input_test.cpp"), "-o", str(cls.probe)],
                       check=True, capture_output=True, text=True)
        cls.rows = json.loads(subprocess.run([str(cls.probe), "--bindings"],
                                            check=True, capture_output=True, text=True).stdout)
        cls.contract = json.loads((ROOT / "data/r8/input_contract.json").read_text())

    def test_compiled_bindings_match_frozen_semantic_schema(self):
        expected = {(source, key, action)
                    for source in ("keyboard", "gamepad")
                    for action, keys in self.contract["mapping"][source].items()
                    for key in keys}
        actual = {(row["source"], row["key"], row["action"]) for row in self.rows}
        self.assertEqual(actual, expected)
        self.assertEqual(len(self.rows), len(actual))

    def test_actual_engine_key_names_cover_all_gamepad_semantics(self):
        names = {"DPadUp": "Gamepad_DPad_Up", "DPadDown": "Gamepad_DPad_Down",
                 "DPadLeft": "Gamepad_DPad_Left", "DPadRight": "Gamepad_DPad_Right",
                 "FaceBottom": "Gamepad_FaceButton_Bottom", "FaceRight": "Gamepad_FaceButton_Right",
                 "FaceTop": "Gamepad_FaceButton_Top", "SpecialRight": "Gamepad_Special_Right",
                 "LeftShoulder": "Gamepad_LeftShoulder", "RightShoulder": "Gamepad_RightShoulder"}
        gamepad = [row for row in self.rows if row["source"] == "gamepad"]
        self.assertEqual({row["action"] for row in gamepad}, set(self.contract["actions"]))
        for row in gamepad:
            self.assertEqual(row["engine_key"], names[row["key"]])

    def test_compiled_edge_epoch_and_axis_regressions(self):
        result = subprocess.run([str(self.probe)], check=True, capture_output=True, text=True)
        self.assertRegex(result.stdout, r"\d+ R8 physical input checks passed")

