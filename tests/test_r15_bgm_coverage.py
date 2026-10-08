import copy
import hashlib
import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from build_r15_bgm_coverage import bind_inputs, validate_coverage
from build_r15_bgm_pilot import verify_blob


class CoverageIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.plan = json.loads((ROOT/'data/r15/bgm_source_plan.json').read_text())
        cls.pilot = json.loads((ROOT/'data/r15/bgm_pilot_evidence.json').read_text())

    def test_all_209_midi_inputs_bind_to_pinned_plan_and_bank(self):
        source = copy.deepcopy(self.pilot['source_inputs'])
        for job in self.plan['render_jobs']:
            path = job['source_file']['path']; data = (ROOT/'vendor/vanillaplus'/path).read_bytes()
            source['files'][path] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                git_blob=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest())
        bind_inputs(source, self.pilot['renderer_inputs'], self.plan, self.pilot)
        self.assertEqual(len(source['files']),589)
        for mutate in ['midi','bank','renderer','extra','missing','pin']:
            s=copy.deepcopy(source); r=copy.deepcopy(self.pilot['renderer_inputs'])
            if mutate=='midi': s['files'][self.plan['render_jobs'][0]['source_file']['path']]['sha256']='0'*64
            if mutate=='bank': s['files']['sound/voicegroups/voicegroup000.inc']['sha256']='0'*64
            if mutate=='renderer': r['files']['plugin/m4a_engine.c']['sha256']='0'*64
            if mutate=='extra': s['files']['unexpected']=dict(bytes=1,sha256='0'*64,git_blob='0'*40)
            if mutate=='missing': s['files'].pop(self.plan['render_jobs'][0]['source_file']['path'])
            if mutate=='pin': s['commit']='0'*40
            with self.subTest(mutation=mutate), self.assertRaises(ValueError):
                bind_inputs(s,r,self.plan,self.pilot)

    def report(self):
        return json.loads((ROOT/'data/r15/bgm_render_coverage.json').read_text())

    def test_complete_actual_render_receipts_and_pilot_replay(self):
        report=self.report(); validate_coverage(report,self.plan,self.pilot)
        self.assertEqual(len(report['renders']),209)
        self.assertEqual(sum(r['loop_evidence'] is not None for r in report['renders']),176)
        self.assertEqual(report['remaining_music_jingle_jobs'],0)

    def test_all_589_published_source_receipts_match_actual_pinned_bytes(self):
        for path,row in self.report()['source_inputs']['files'].items():
            with self.subTest(path=path):
                verify_blob((ROOT/'vendor/vanillaplus'/path).read_bytes(),row)

    def test_missing_duplicate_and_reserved_render_identity_rejected(self):
        for mutate in ['missing','duplicate','reserved']:
            r=self.report()
            if mutate=='missing':r['renders'].pop()
            if mutate=='duplicate':r['renders'][1]=copy.deepcopy(r['renders'][0])
            if mutate=='reserved':r['renders'][0]['source_id']=270
            with self.subTest(mutation=mutate),self.assertRaises(ValueError):
                validate_coverage(r,self.plan,self.pilot)

    def test_wrong_recipe_source_and_pilot_hash_rejected(self):
        for mutate in ['recipe','source','pilot','settings','units']:
            r=self.report(); row=next(x for x in r['renders'] if x['symbol']=='mus_littleroot')
            if mutate=='recipe':row['conversion']['master_volume']+=1
            if mutate=='source':row['source_midi_sha256']='0'*64
            if mutate=='pilot':row['sha256']='0'*64
            if mutate=='settings':row['settings'][row['settings'].index('--polyphony')+1]='8'
            if mutate=='units':row['native_midi']['division']*=2
            with self.subTest(mutation=mutate),self.assertRaises(ValueError):
                validate_coverage(r,self.plan,self.pilot)

    def test_coverage_cannot_promote_runtime_quality_or_hide_audio_failure(self):
        for mutate in ['quality','unreal','hardware','loop','silent','clipped','frames','engine']:
            r=self.report();row=r['renders'][0]
            if mutate=='quality':r['quality_approved']=True
            if mutate=='unreal':r['unreal_import_validated']=True
            if mutate=='hardware':r['hardware_audio_equivalence_verified']=True
            if mutate=='loop':row['loop_ready']=True
            if mutate=='silent':row['peak_sample']=0
            if mutate=='clipped':row['full_scale_samples']=1
            if mutate=='frames':row['frames']-=1
            if mutate=='engine':r['engine_tests_passed']=706
            with self.subTest(mutation=mutate),self.assertRaises(ValueError):
                validate_coverage(r,self.plan,self.pilot)


if __name__=='__main__':unittest.main()
