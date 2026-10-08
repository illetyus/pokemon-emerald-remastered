from pathlib import Path
import copy
import json
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from r8_input_fixture_matrix import replay,validate


class R8VersionedMatrix(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        cls.probe=Path(cls.temp.name)/'r8-matrix'
        compiler=shutil.which('g++')
        if not compiler:raise RuntimeError('R8 compiled fixture oracle requires g++')
        subprocess.run([compiler,'-std=c++17','-Wall','-Wextra','-Wpedantic','-Werror',
                        '-I',str(ROOT/'core/include'),'-I',str(ROOT/'unreal/Source/PokemonEmeraldRemastered'),
                        str(ROOT/'tests/r8_input_fixture_probe.cpp'),'-o',str(cls.probe)],
                       check=True,capture_output=True,text=True)
        cls.recipe=json.loads((ROOT/'tests/fixtures/r8/input_matrix.json').read_text())

    def test_actual_compiled_matrix(self):
        result=replay(self.probe,self.recipe)
        self.assertEqual(result['cases'],len(self.recipe['cases']))
        self.assertGreater(result['observations'],120)

    def test_first_divergent_case_step_is_reported(self):
        recipe=copy.deepcopy(self.recipe)
        recipe['cases'][0]['steps'][1]['expect']['target']='BattleInput'
        with self.assertRaisesRegex(AssertionError,r'keyboard/Up step 1.*target'):
            replay(self.probe,recipe)

    def test_malformed_fixture_action_rejected(self):
        recipe=copy.deepcopy(self.recipe)
        recipe['cases'][0]['steps'][1]['action']='WriteSave'
        with self.assertRaisesRegex(ValueError,'action'):
            validate(recipe)

    def test_duplicate_case_identity_rejected(self):
        recipe=copy.deepcopy(self.recipe)
        recipe['cases'].append(copy.deepcopy(recipe['cases'][0]))
        with self.assertRaisesRegex(ValueError,'duplicate'):
            validate(recipe)


if __name__=='__main__':unittest.main()
