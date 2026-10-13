import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
def module(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'tools'/('build_ap04_new_game_'+name+'.py'))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
init=module('init');oracle=module('oracle')

class PersistentInitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.files=init.load(ROOT)
        cls.layout=json.loads((ROOT/'data/ap04/new_game_layout.json').read_text())
        cls.blocks,cls.contract=init.derive(cls.files,cls.layout)
    def test_immutable_source_generates_exact_tracked_data(self):
        self.assertEqual(init.emit(self.blocks,self.contract,self.layout).encode(),(ROOT/'core/src/emerald_new_game_init_catalog.inc').read_bytes())
        self.assertEqual(self.contract,json.loads((ROOT/'data/ap04/new_game_init_source.json').read_text()))
        self.assertEqual(init.derive(self.files,self.layout),(self.blocks,self.contract))
    def test_all_source_hashes_and_block_hashes_bind_inputs(self):
        for name,data in self.files.items():self.assertEqual(hashlib.sha256(data).hexdigest(),self.contract['SourceHashes'][name])
        self.assertEqual([hashlib.sha256(b).hexdigest() for b in self.blocks],self.contract['BlockSHA256'])
    def test_production_provenance_and_sizes_cannot_be_host_guesses(self):
        for key,value in [('CompilerPin','wrong'),('SourceCommit','wrong'),('VendorTree','wrong'),('KnownProductionAnchorsVerified',False),('CompilerBuiltFromSnapshot',False)]:
            layout=copy.deepcopy(self.layout);layout[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):init.derive(self.files,layout)
        layout=copy.deepcopy(self.layout);layout['Fields']['sb1.bytes']+=4
        with self.assertRaises(ValueError):init.derive(self.files,layout)
    def test_all_production_bitfields_reject_drift(self):
        for name in self.layout['BitfieldDefaultBytes']:
            layout=copy.deepcopy(self.layout);raw=bytearray.fromhex(layout['BitfieldDefaultBytes'][name]);raw[0]^=1
            layout['BitfieldDefaultBytes'][name]=raw.hex()
            with self.subTest(name=name),self.assertRaises(ValueError):init.derive(self.files,layout)
    def test_out_of_bounds_layout_cannot_resize_block(self):
        for field in ['sb1.mail','sb1.pcItems','storage.boxNames','sb2.apprentices']:
            layout=copy.deepcopy(self.layout);layout['Fields'][field]=65535
            with self.subTest(field=field),self.assertRaises(ValueError):init.derive(self.files,layout)
    def test_restricted_constants_never_execute_python(self):
        constants=init.Constants(['#define X 2\n#define Y (X << 4)\n#define LOOP LOOP\n'])
        self.assertEqual(constants.value('Y | 3'),35)
        for value in ['__import__("os")','x.y','[1][0]','True','LOOP','UNKNOWN','(lambda: 1)()','2 ** 30']:
            with self.subTest(value=value),self.assertRaises(ValueError):constants.value(value)
    def test_pc_and_domain_dimension_changes_fail_closed(self):
        files=dict(self.files);files['src/player_pc.c']=files['src/player_pc.c'].replace(b'{ ITEM_POTION, 1 }',b'{ ITEM_POTION, 2 }')
        self.assertNotEqual(files,self.files)
        with self.assertRaises(ValueError):init.derive(files,self.layout)
        files=dict(self.files);name='include/constants/global.h'
        self.assertIn(name,files);files[name]=files[name].replace(b'QUIZ_QUESTION_LEN 9',b'QUIZ_QUESTION_LEN 8')
        self.assertNotEqual(files,self.files)
        with self.assertRaises(ValueError):init.derive(files,self.layout)
    def test_all_original_domain_functions_are_unchanged(self):
        generated,hashes=oracle.generate(self.files,self.layout)
        self.assertEqual(len(hashes),17)
        for file,names in oracle.FUNCTIONS.items():
            for name in names:
                body=init.prepare.function(self.files[file].decode(),name)
                self.assertIn(body,generated);self.assertEqual(hashlib.sha256(body.encode()).hexdigest(),hashes[name])
    def test_storage_cycle_hill_clock_and_berry_reset_source_constants(self):
        fields=self.layout['Fields'];sb2,sb1,storage=self.blocks
        self.assertEqual(storage[fields['storage.boxWallpapers']:fields['storage.boxWallpapers']+14],bytes(i%4 for i in range(14)))
        self.assertEqual(int.from_bytes(sb1[fields['sb1.trainerHillTimes']:fields['sb1.trainerHillTimes']+4],'little'),215999)
        self.assertGreater(self.contract['InitialBerryTrees'],40)
        self.assertGreater(len(self.contract['InitialFlags']),100)
        for i in range(4):self.assertEqual(sb2[fields['sb2.apprentices']+i*68],16)
    def test_source_bug_storyteller_and_explicit_ranking_policy_are_preserved(self):
        offset=self.layout['Fields']['sb1.oldMan.storyteller.trainerNames']-self.layout['Fields']['sb1.oldMan']
        row=self.contract['Lists']['oldMen'][3]
        self.assertEqual(row[offset:offset+4],[255]*4)
        self.assertEqual(row[offset+4:offset+11],[0]*7)
        self.assertEqual(self.contract['RankingIdPolicy'],'EXPLICIT_SOURCE_UBFIX_ZERO_ID')
    def test_persistent_projection_does_not_claim_full_source_or_unreal_execution(self):
        self.assertTrue(self.contract['PersistentStateProjected'])
        self.assertFalse(self.contract['FullOriginalNewGameInitDataExecuted'])
        self.assertFalse(self.contract['UnrealRuntimeVerified'])
        self.assertEqual(len(self.contract['PendingHostActions']),3)

if __name__=='__main__':unittest.main()
