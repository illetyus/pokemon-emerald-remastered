import copy
import hashlib
import json
import re
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_r15_audio_catalog import build,dump,VENDOR
from validate_r15_audio_pack import validate,strict
from build_r15_audio_pilot import audition,stats,wav_bytes,pcm

class SourceCoverage(unittest.TestCase):
    @classmethod
    def setUpClass(cls): cls.catalog,cls.header=build()
    def test_generated_outputs_match_and_repeat(self):
        self.assertEqual(build(),(self.catalog,self.header))
        self.assertEqual((ROOT/'data/r15/source_audio_catalog.json').read_text(),dump(self.catalog))
        self.assertEqual((ROOT/'unreal/Source/PokemonEmeraldRemastered/RemasterAudioCatalog.inl').read_text(),self.header)
    def test_song_table_oracle(self):
        rows=re.findall(r'^\s*song\s+(\w+),\s*(\d+),\s*(\d+)',(VENDOR/'sound/song_table.inc').read_text(),re.M)
        self.assertEqual(len(rows),610)
        for i,(symbol,player,group) in enumerate(rows):
            actual=self.catalog['songs'][i];self.assertEqual((actual['source_id'],actual['source_symbol'],actual['source_player'],actual['source_group']),(i,symbol,int(player),int(group)))
        # Source constant MUS_DESERT is an alias whose physical song name differs.
        self.assertIn('MUS_DESERT',self.catalog['songs'][409]['constant_aliases'])
    def test_cry_index_oracle(self):
        table=re.findall(r'^\s*cry\s+(\w+)',(VENDOR/'sound/cry_tables.inc').read_text().split('gCryTable_Reverse::')[0],re.M)
        indices={n:int(i) for n,i in re.findall(r'\[(SPECIES_\w+)\s*-\s*277\]\s*=\s*(\d+)',(VENDOR/'src/data/pokemon/cry_ids.h').read_text())}
        species={s['core_species']:s for s in json.loads((ROOT/'data/r14/species_audit.json').read_text())['species']}
        for c in self.catalog['cries']:
            i=c['core_species']-1 if c['core_species']<=251 else indices[species[c['core_species']]['source_symbol']]
            self.assertEqual(c['cry_table_index'],i);self.assertEqual(c['cry_symbol'],table[i])
        self.assertEqual({c['national_dex'] for c in self.catalog['cries']},set(range(1,387)))
    def test_hashes_match_all_actual_source_bytes(self):
        for path,sha in self.catalog['source_hashes'].items(): self.assertEqual(hashlib.sha256((VENDOR/path).read_bytes()).hexdigest(),sha)
        for c in self.catalog['cries']: self.assertEqual(hashlib.sha256((VENDOR/c['source_pcm']['path']).read_bytes()).hexdigest(),c['source_pcm']['sha256'])
    def test_fanfare_source_waits_are_not_audio_duration(self):
        self.assertEqual(len(self.catalog['fanfares']),18)
        self.assertEqual(self.catalog['fanfares'][0]['source_wait_frames'],80)
        self.assertEqual(self.catalog['fanfares'][8]['source_wait_frames'],710)
        self.assertEqual({m['id'] for m in self.catalog['cry_modes']},set(range(13)))
    def test_missing_quality_and_imports_stay_explicit(self):
        self.assertEqual(self.catalog['counts']['approved_modern_cries'],0)
        self.assertEqual(self.catalog['counts']['verified_unreal_audio_imports'],0)
        for c in self.catalog['cries']: self.assertIn(c['modern'],['pilot_candidate_pending_listening','missing_pending_pilot','prepared_local_normal_only_not_imported','prepared_local_all_modes_candidates_fidelity_pending_not_imported'])
        for s in self.catalog['songs'][1:]: self.assertEqual(s['modern'],'missing_local_render')
    def test_authority_boundary(self):
        module=ROOT/'unreal/Source/PokemonEmeraldRemastered'
        text='\n'.join((module/n).read_text() for n in ['RemasterAudioRead.h','RemasterAudioSubsystem.h','RemasterAudioSubsystem.cpp'])
        for mutation in ['script_runtime_complete','battle_resolve_turn','battle_clear_events','rng_next','save_write']:
            self.assertNotRegex(text,rf'remaster_emerald_{mutation}\s*\(')
        self.assertIn('GGameUserSettingsIni',text)
        self.assertIn('RemoveTicker(TickHandle)',text)
        self.assertIn('ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle)',text)
        self.assertNotIn('BlueprintCallable',(module/'RemasterAudioSubsystem.h').read_text().split('bool PresentCoreRequest')[0].split('// Dispatch committed')[-1])

class PackProbe(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
        with wave.open(str(self.root/'tone.wav'),'wb') as w:
            w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes(struct.pack('<8h',0,1000,2000,1000,0,-1000,-2000,-1000))
        self.e={'identity':'cry.25.mode.0','status':'candidate','path':'tone.wav','sha256':hashlib.sha256((self.root/'tone.wav').read_bytes()).hexdigest(),'sample_rate':48000,'channels':1,'frames':8,'gain':1,'fade_seconds':0,'loop':None,
            'provenance':{'repository':'synthetic fixture','commit':'a'*40,'source_path':'fixture-generated','source_sha256':'b'*64,'rights':'original generated test waveform'}}
        self.m={'schema':'r15-local-audio-v1','profile':'modern','entries':[self.e]}
    def test_local_probe_never_claims_quality_or_unreal(self):
        r=validate(self.m,self.root);self.assertFalse(r['quality_approved']);self.assertFalse(r['unreal_import_validated']);self.assertEqual(r['entries'][0]['decoded_bytes'],16);self.assertEqual(r['required_semantics'],5627)
    def test_missing_needs_reason(self):
        self.m['entries']=[{'identity':'song.5','status':'missing','reason':'local render unavailable'}];self.assertEqual(validate(self.m,self.root)['entries'][0]['status'],'missing')
        self.m['entries'][0].pop('reason')
        with self.assertRaises(ValueError): validate(self.m,self.root)
    def test_unknown_id_and_duplicate_id(self):
        for entries in [[dict(self.e,identity='cry.999.mode.0')],[self.e,self.e]]:
            self.m['entries']=entries
            with self.assertRaises(ValueError): validate(self.m,self.root)
    def test_path_escape_absolute_and_symlink(self):
        (self.root/'escape.wav').symlink_to('/etc/hosts')
        for path in ['../outside.wav','/etc/hosts','escape.wav']:
            self.e['path']=path
            with self.assertRaises(ValueError): validate(self.m,self.root)
    def test_hash_mismatch(self):
        self.e['sha256']='0'*64
        with self.assertRaises(ValueError): validate(self.m,self.root)
    def test_bad_loop_bounds(self):
        for start,end in [(0,9),(8,8),(-1,8),(0,True),(0,1.5)]:
            self.e['loop']={'start_frame':start,'end_frame':end}
            with self.assertRaises(ValueError): validate(self.m,self.root)
    def test_valid_loop_frame_contract(self):
        self.e['loop']={'start_frame':2,'end_frame':8};self.assertFalse(validate(self.m,self.root)['unreal_import_validated'])
    def test_pcm_shape_metadata(self):
        for k,v in [('sample_rate',44100),('channels',2),('frames',False)]:
            e=copy.deepcopy(self.e);e[k]=v
            with self.assertRaises(ValueError): validate(dict(self.m,entries=[e]),self.root)
    def test_finite_bounded_gain_and_fade(self):
        for k,v in [('gain',float('nan')),('gain',-1),('fade_seconds',11),('gain',True)]:
            e=copy.deepcopy(self.e);e[k]=v
            with self.assertRaises(ValueError): validate(dict(self.m,entries=[e]),self.root)
    def test_provenance_required_and_hashed(self):
        for k,v in [('rights',''),('commit','main'),('source_sha256','unknown')]:
            e=copy.deepcopy(self.e);e['provenance'][k]=v
            with self.assertRaises(ValueError): validate(dict(self.m,entries=[e]),self.root)
    def test_status_cannot_promote_candidate(self):
        self.e['status']='approved'
        with self.assertRaises(ValueError): validate(self.m,self.root)
    def test_strict_json(self):
        for text in ['{"x":1,"x":2}','{"x":NaN}','{"x":Infinity}']:
            with self.assertRaises(ValueError): strict(text)
    def test_listening_gain_preserves_timing_and_peak(self):
        a=pcm(self.root/'tone.wav');b,gain=audition(a);self.assertEqual(len(a),len(b));self.assertLessEqual(gain,4);self.assertEqual(stats(b)['full_scale_samples'],0)
        (self.root/'copy.wav').write_bytes(wav_bytes(b));self.assertEqual(pcm(self.root/'copy.wav'),b)
