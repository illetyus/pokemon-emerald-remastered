import hashlib
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    'ap04_new_game_prepare', ROOT / 'tools/build_ap04_new_game_prepare.py')
prepare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prepare)


class NewGameSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.files = prepare.source_files(ROOT)
        cls.catalog, cls.oracle, cls.contract = prepare.derive(cls.files)

    def test_tracked_catalog_and_contract_match_immutable_source(self):
        self.assertEqual(self.catalog.encode(),
            (ROOT / 'core/src/emerald_new_game_catalog.inc').read_bytes())
        self.assertEqual(self.contract,
            json.loads((ROOT / 'data/ap04/new_game_prepare_source.json').read_text()))
        self.assertEqual(prepare.derive(self.files),
            (self.catalog, self.oracle, self.contract))

    def test_original_semantic_functions_are_retained_exactly(self):
        for file, name in [('src/new_game.c', 'SetDefaultOptions'),
                ('src/new_game.c', 'SetTrainerId'), ('src/new_game.c', 'InitPlayerTrainerId'),
                ('src/random.c', 'Random'), ('src/main.c', 'GetGeneratedTrainerIdLower')]:
            body = prepare.function(self.files[file].decode(), name)
            self.assertIn(body, self.oracle)
            self.assertEqual(hashlib.sha256(body.encode()).hexdigest(),
                self.contract['OriginalFunctionHashes'][name])

    def test_preparation_does_not_claim_full_initialization_or_runtime(self):
        for claim in ['RTCInitialized', 'NewGameInitDataExecuted', 'UsableSaveInitialized',
                'SourceABIVerifiedBySemanticStub', 'UnrealRuntimeVerified']:
            self.assertIs(self.contract[claim], False)
        pending = self.contract['PendingSourceCalls']
        self.assertNotIn('InitPlayerTrainerId', pending)
        for name in ['RtcReset', 'ResetPokemonStorageSystem', 'NewGameInitPCItems',
                'InitDewfordTrend', 'InitLilycoveLady', 'RunScriptImmediately', 'WarpToTruck']:
            self.assertIn(name, pending)
        self.assertGreater(len(pending), 30)

    def test_truck_metadata_is_not_house_checkpoint(self):
        self.assertEqual(self.contract['Truck'], {'Group': 25, 'Map': 40, 'Layout': 237,
            'X': 2, 'Y': 2, 'WarpId': -1, 'WarpX': -1, 'WarpY': -1})

    def test_keyboard_drift_control_and_duplicate_bytes_fail_closed(self):
        for old, new in [(b'abc\xc3\xa7def ', b'abcdefgh '),
                (b"'A'         = BB", b"'A'         = FC"),
                (b"'A'         = BB", b"'A'         = BC")]:
            files = dict(self.files)
            field = 'src/naming_screen.c' if old.startswith(b'abc') else 'charmap.txt'
            self.assertIn(old, files[field])
            files[field] = files[field].replace(old, new)
            with self.subTest(new=new), self.assertRaises(ValueError):
                prepare.derive(files)

    def test_missing_or_ambiguous_truck_and_changed_layout_rejected(self):
        for case in ['missing', 'duplicate', 'layout']:
            files = dict(self.files)
            groups = json.loads(files['data/maps/map_groups.json'])
            if case == 'missing':
                groups['gMapGroup_IndoorDynamic'].remove('InsideOfTruck')
            elif case == 'duplicate':
                groups['gMapGroup_IndoorDynamic'].append('InsideOfTruck')
            else:
                files['include/constants/layouts.h'] = files['include/constants/layouts.h'].replace(
                    b'LAYOUT_INSIDE_OF_TRUCK 237', b'LAYOUT_INSIDE_OF_TRUCK 238')
            files['data/maps/map_groups.json'] = json.dumps(groups).encode()
            with self.subTest(case=case), self.assertRaises(ValueError):
                prepare.derive(files)

    def test_unknown_option_fields_rejected(self):
        files = dict(self.files)
        files['src/new_game.c'] = files['src/new_game.c'].replace(
            b'gSaveBlock2Ptr->regionMapZoom = FALSE;', b'gSaveBlock2Ptr->unknownOption = FALSE;')
        with self.assertRaises(ValueError):
            prepare.derive(files)

    def test_function_parser_ignores_comments_literals_and_rejects_ambiguity(self):
        text = '/* f() { } */\nvoid f(void) { const char *s="}"; /* { */ if(1) { } }\n'
        self.assertEqual(prepare.function(text, 'f'), text.split('\n')[1])
        for bad in ['void other(void) {}', text + '\nvoid f(void) {}', 'void f(void) {']:
            with self.subTest(text=bad), self.assertRaises(ValueError):
                prepare.function(bad, 'f')


if __name__ == '__main__':
    unittest.main()
