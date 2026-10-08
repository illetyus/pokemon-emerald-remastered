import hashlib
import json
import sys
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_r15_audio_catalog import build
from build_r15_audio_pilot import MODERN_PIN
from fetch_r15_modern_cries import verify
from build_r15_modern_pack import assemble_entries

class ModernSelection(unittest.TestCase):
    def test_user_selection_is_style_approval_not_engine_import(self):
        selection=json.loads((ROOT/'data/r15/modern_selection.json').read_text())
        self.assertTrue(selection['pilot_style_approved']);self.assertEqual(selection['selected_profile'],'modern')
        self.assertFalse(selection['individual_full_pack_listening_verified']);self.assertFalse(selection['unreal_import_validated'])
    def test_new_default_keeps_saved_user_choice(self):
        module=ROOT/'unreal/Source/PokemonEmeraldRemastered'
        self.assertIn('SelectedPack=ERemasterAudioPack::Modern',(module/'RemasterAudioSubsystem.h').read_text())
        cpp=(module/'RemasterAudioSubsystem.cpp').read_text()
        self.assertIn('int32 Pack=1;GConfig->GetInt',cpp)
        self.assertIn('SelectedPack=Pack==1?ERemasterAudioPack::Modern:ERemasterAudioPack::Original',cpp)
    def test_pinned_source_byte_receipt(self):
        data=b'original synthetic fixture';sha=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
        receipt={'commit':MODERN_PIN,'sha':sha,'size':len(data)};verify(data,receipt)
        for changed in [dict(receipt,sha='0'*40),dict(receipt,size=True),dict(receipt,commit='main'),dict(receipt,size=len(data)-1)]:
            with self.assertRaises(ValueError):verify(data,changed)
        with self.assertRaises(ValueError):verify(data+b'corrupted',receipt)
    def test_other_modes_are_explicitly_missing(self):
        catalog,_=build();normal={i:{'entry':{'identity':f'cry.{i}.mode.0','status':'candidate'}} for i in range(1,387)}
        entries=assemble_entries(catalog,normal);keys=[e['identity'] for e in entries]
        self.assertEqual(len(keys),len(set(keys)));self.assertEqual(len(keys),5627)
        ready=[e for e in entries if e['status']=='candidate'];missing=[e for e in entries if e['status']=='missing']
        self.assertEqual(len(ready),386);self.assertEqual(len(missing),5241)
        self.assertTrue(all(e['identity'].endswith('.mode.0') for e in ready))
        self.assertTrue(all(any(e['identity']==f'cry.{i}.mode.{m}' and e['status']=='missing' for e in entries) for i in [1,25,201,351,386] for m in range(1,13)))
    def test_incomplete_modern_pack_rejected(self):
        catalog,_=build()
        with self.assertRaises(ValueError):assemble_entries(catalog,{25:{'entry':{}}})
