import array
import copy
import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from build_r15_audio_catalog import VENDOR, build as catalog
from build_r15_cry_modes import build, function, numeric_table, frequency, RATE, CLOCK, FRAME_CYCLES
from build_r15_special_cries import render, release_curve, assemble_entries, transformed
from build_r15_audio_pilot import wav_bytes, pcm
from validate_r15_audio_pack import validate

class PinnedSourceOracle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Compile the actual pinned switch, setters, track pitch and frequency
        # calculation, with only the device/music allocation boundary stubbed.
        # Thus the oracle is independent of Python's interpretation of the switch.
        compiler = shutil.which('cc')
        if not compiler:
            raise RuntimeError('source oracle requires a C compiler')
        source = (VENDOR/'src/m4a.c').read_text()
        tables = (VENDOR/'src/m4a_tables.c').read_text()
        prefix = r'''
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16;
typedef int16_t s16; typedef uint32_t u32; typedef int32_t s32; typedef int bool32;
#define TRUE 1
#define FALSE 0
#define C_V 64
#define MPT_FLG_VOLSET 1
#define MPT_FLG_PITSET 4
#include "constants/sound.h"
struct ToneData { int dummy; };
struct MusicPlayerInfo { int dummy; };
struct WaveData { u32 freq; };
struct MusicPlayerTrack {
    u8 flags, pitM; s8 keyM, tune;
    int bend,bendRange,keyShift,keyShiftX,pitX,modT,modM;
    int vol,volX,pan,panX,volMR,volML;
};
struct Song { u8 tuneValue,tuneValue2,tieKeyValue,trackCount;
    u8 volumeValue,panValue,releaseValue,priority; u16 unkCmd0CParam;u32 unkCmd0DParam;
} gPokemonCrySong;
struct ToneData gCryTable[512],gCryTable_Reverse[512];
struct MusicPlayerInfo *gMPlay_PokemonCry;
static int reversed;
static u32 umul3232H32(u32 a,u32 b) { return (u32)(((uint64_t)a*b)>>32); }
static u16 SpeciesToCryId(u16 species) { return species; }
static struct MusicPlayerInfo *SetPokemonCryTone(struct ToneData *tone) {
    reversed = tone == &gCryTable_Reverse[0]; return 0;
}
'''
        definitions = []
        for name in ['SetPokemonCryVolume','SetPokemonCryPanpot','SetPokemonCryPitch',
                     'SetPokemonCryLength','SetPokemonCryProgress','SetPokemonCryRelease',
                     'SetPokemonCryChorus','SetPokemonCryPriority','TrkVolPitSet']:
            definitions.append('void ' + function(source,name))
        definitions.append('void ' + function((VENDOR/'src/sound.c').read_text(),'PlayCryInternal'))
        for name, typ in [('gScaleTable','u8'),('gFreqTable','u32')]:
            numbers = numeric_table(tables,name)
            prefix += 'const '+typ+' '+name+'[] = {'+','.join(str(n)+'u' for n in numbers)+'};\n'
        definitions.append('u32 ' + function(source,'MidiKeyToFreq'))
        main = r'''
int main(void) {
    struct WaveData wave={0xffffffffu};
    for(int mode=0; mode<13; ++mode) {
        gPokemonCrySong.tuneValue=64;gPokemonCrySong.tuneValue2=80;
        PlayCryInternal(1,0,CRY_VOLUME,CRY_PRIORITY_NORMAL,(u8)mode);
        printf("%d %u %u %u %d %u",mode,gPokemonCrySong.volumeValue,
            gPokemonCrySong.unkCmd0CParam,gPokemonCrySong.releaseValue,reversed,gPokemonCrySong.trackCount);
        for(int t=0;t<gPokemonCrySong.trackCount;++t) {
            struct MusicPlayerTrack track={0};track.flags=MPT_FLG_PITSET;
            track.tune=(s8)((t?gPokemonCrySong.tuneValue2:gPokemonCrySong.tuneValue)-64);
            TrkVolPitSet(0,&track);
            int key=gPokemonCrySong.tieKeyValue+track.keyM;
            printf(" %d %u %u",key,track.pitM,MidiKeyToFreq(&wave,(u8)key,track.pitM));
        }
        puts("");
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'oracle.c';exe=Path(tmp)/'oracle'
            path.write_text(prefix+'\n'.join(definitions)+main)
            subprocess.run([compiler,'-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
                            '-I',str(VENDOR/'include'),str(path),'-o',str(exe)],check=True,capture_output=True)
            cls.oracle = [list(map(int,row.split())) for row in subprocess.check_output([str(exe)],text=True).splitlines()]

    def test_all_modes_match_compiled_source_parameters_and_frequency(self):
        recipes=build();text=(VENDOR/'src/m4a_tables.c').read_text()
        scale=numeric_table(text,'gScaleTable');freq=numeric_table(text,'gFreqTable')
        for r, actual in zip(recipes['modes'],self.oracle):
            with self.subTest(mode=r['id']):
                p=r['source_parameters']
                self.assertEqual(actual[:6],[r['id'],p['volume'],p['length'],p['release'],int(p['reverse']),len(r['tracks'])])
                expected=[]
                for t in r['tracks']:
                    factor=frequency(t['midi_key'],t['fine_256'],scale,freq)
                    expected.extend([t['midi_key'],t['fine_256'],factor*0xffffffff>>32])
                self.assertEqual(actual[6:],expected)

    def test_weak_doubles_fallthrough_and_signed_chorus_wrap(self):
        r=build()['modes']
        self.assertEqual(r[12]['tracks'],r[11]['tracks'])
        self.assertEqual(r[12]['source_parameters']['length'],20)
        # Source 192 is not a delay in milliseconds. It wraps fine tuning.
        self.assertEqual([(t['midi_key'],t['fine_256']) for t in r[4]['tracks']],[(60,224),(61,224)])
        self.assertLess(r[5]['tracks'][0]['relative_rate'],r[11]['tracks'][0]['relative_rate'])

class CryRendering(unittest.TestCase):
    def test_generated_recipe_and_fidelity_gates(self):
        value=build();stored=json.loads((ROOT/'data/r15/cry_mode_recipes.json').read_text())
        self.assertEqual(value,stored)
        self.assertFalse(value['hardware_audio_equivalence_verified'])
        self.assertFalse(value['individual_listening_verified'])
        self.assertFalse(value['unreal_import_validated'])

    def test_release_uses_integer_decay_and_rejects_bad_byte(self):
        self.assertEqual(release_curve(0),[255,0])
        self.assertEqual(release_curve(100)[:4],[255,99,38,14])
        curve=release_curve(225)
        self.assertEqual(curve[-1],0)
        self.assertTrue(all(b<a for a,b in zip(curve,curve[1:])))
        for bad in [-1,256,True,1.5]:
            with self.assertRaises(ValueError):release_curve(bad)

    def test_shortened_cry_gate_and_tail_are_audio_only(self):
        r=build()['modes'][7];a=array.array('h',[1000]*(RATE*3))
        rendered,guard=render([a],r)
        gate=r['gate_seconds']*RATE;frame=RATE*FRAME_CYCLES/CLOCK
        self.assertEqual(len(rendered),math.ceil(gate+(len(release_curve(100))-1)*frame))
        self.assertEqual(rendered[0],2000)
        self.assertEqual(guard,1)
        self.assertLess(rendered[-1],rendered[0])

    def test_source_volume_not_erased_by_per_mode_normalization(self):
        r=build()['modes'][2];a=array.array('h',[1000]*4800)
        rendered,guard=render([a,a],r)
        self.assertEqual(rendered[0],3000) # restore 0.5 headroom, sum two, volume 90/120
        self.assertEqual(guard,1)
        strong=array.array('h',[20000]*4800)
        rendered,guard=render([strong,strong],r)
        self.assertLess(guard,1)
        self.assertLessEqual(max(rendered),math.ceil(32768*10**(-3/20)))

    def test_complete_cry_coverage_keeps_other_audio_missing(self):
        c,_=catalog();entries={f'cry.{i}.mode.{m}':{'identity':f'cry.{i}.mode.{m}','status':'candidate'}
                               for i in range(1,387) for m in range(13)}
        combined=assemble_entries(c,entries)
        self.assertEqual(len(combined),5627)
        self.assertEqual(sum(e['status']=='candidate' for e in combined),5018)
        self.assertEqual(sum(e['status']=='missing' for e in combined),609)
        del entries['cry.386.mode.12']
        with self.assertRaises(ValueError):assemble_entries(c,entries)

    def fixture_manifest(self, root):
        payload=wav_bytes(array.array('h',[0,1000,-1000,0]));(root/'fixture.wav').write_bytes(payload)
        entry={'identity':'cry.25.mode.0','status':'candidate','path':'fixture.wav',
               'sha256':hashlib.sha256(payload).hexdigest(),'sample_rate':48000,'channels':1,'frames':4,
               'gain':1,'fade_seconds':0,'loop':None,
               'provenance':{'repository':'original synthetic fixture','commit':'a'*40,
                   'source_path':'generated','source_sha256':'b'*64,'rights':'original test waveform'}}
        special=dict(entry,identity='cry.25.mode.5',processing={'kind':'authored_modern_special_candidate',
            'mode':5,'recipe_sha256':hashlib.sha256((ROOT/'data/r15/cry_mode_recipes.json').read_bytes()).hexdigest(),
            'source_parameter_pin':build()['source_pin'],'base_normal_sha256':entry['sha256'],
            'peak_guard_gain':1,'hardware_audio_equivalence_verified':False})
        return {'schema':'r15-local-audio-v1','profile':'modern','entries':[entry,special]}

    def test_special_probe_rejects_missing_or_mismatched_recipe_and_base(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);manifest=self.fixture_manifest(root)
            self.assertFalse(validate(manifest,root)['unreal_import_validated'])
            for field,bad in [('mode',0),('mode',True),('recipe_sha256','0'*64),
                              ('base_normal_sha256','0'*64),('source_parameter_pin','a'*40),
                              ('peak_guard_gain',float('inf')),('hardware_audio_equivalence_verified',True)]:
                changed=copy.deepcopy(manifest);changed['entries'][1]['processing'][field]=bad
                with self.subTest(field=field),self.assertRaises(ValueError):validate(changed,root)
            changed=copy.deepcopy(manifest);changed['entries'][1].pop('processing')
            with self.assertRaises(ValueError):validate(changed,root)

    def test_special_probe_cannot_lose_base_normal_or_invent_loop(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);manifest=self.fixture_manifest(root)
            changed=copy.deepcopy(manifest);changed['entries'].pop(0)
            with self.assertRaises(ValueError):validate(changed,root)
            changed=copy.deepcopy(manifest);changed['entries'][1]['loop']={'start_frame':0,'end_frame':4}
            with self.assertRaises(ValueError):validate(changed,root)

    def test_pitch_changes_duration_and_reverse_order(self):
        if not shutil.which('ffmpeg'):
            if os.environ.get('REMASTER_R15_REQUIRE_FFMPEG') == '1':
                self.fail('FFmpeg is required when REMASTER_R15_REQUIRE_FFMPEG=1')
            self.skipTest('FFmpeg required for audio transform fixture')
        # Asymmetric ramp tests reverse playback rather than a generic echo.
        with tempfile.TemporaryDirectory() as tmp:
            work=Path(tmp);input_path=work/'input.wav'
            source=array.array('h',[int(6000*i/47999) for i in range(48000)])
            input_path.write_bytes(wav_bytes(source));recipes=build()['modes']
            result=transformed(input_path,[recipes[5]],work)
            rate=recipes[5]['tracks'][0]['render_input_rate'];changed=result[rate]
            self.assertAlmostEqual(len(changed)/RATE,48000/rate,delta=.002)
            self.assertLess(changed[100],changed[-100])
            reverse=work/'reverse';reverse.mkdir();input_path=reverse/'source.wav'
            input_path.write_bytes(wav_bytes(reversed(source)))
            changed=transformed(input_path,[recipes[9]],reverse)[recipes[9]['tracks'][0]['render_input_rate']]
            self.assertGreater(changed[100],changed[-100])

if __name__=='__main__':unittest.main()
