import array
import hashlib
import json
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from build_r15_bgm_pilot import frame_at_tick, verify_blob, wave_receipt, loop_receipt, snapshot, ORACLE


class BinarySource(unittest.TestCase):
    def test_snapshot_pin_path_escape_and_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);b=b'input';p=root/'input';p.write_bytes(b)
            row=dict(bytes=5,git_blob=hashlib.sha1(b'blob 5\0'+b).hexdigest(),sha256=hashlib.sha256(b).hexdigest())
            receipt=root/'source_receipts.json'
            receipt.write_text(json.dumps(dict(commit='a'*40,files={'input':row})))
            self.assertEqual(snapshot(root,'a'*40,1)['files']['input'],row)
            with self.assertRaises(ValueError):snapshot(root,'b'*40,1)
            for name in ['../input','/etc/hosts','nested/../input','bad\\name']:
                receipt.write_text(json.dumps(dict(commit='a'*40,files={name:row})))
                with self.assertRaises(ValueError):snapshot(root,'a'*40,1)
            p.unlink();p.symlink_to('/etc/hosts')
            receipt.write_text(json.dumps(dict(commit='a'*40,files={'input':row})))
            with self.assertRaises(ValueError):snapshot(root,'a'*40,1)

    def test_pcm_transport_must_preserve_arbitrary_bytes(self):
        b = bytes(range(8)) + bytes(range(248,256))  # Original synthetic bytes.
        blob = hashlib.sha1(b'blob 16\0' + b).hexdigest()
        self.assertEqual(verify_blob(b, dict(bytes=16, git_blob=blob,
            sha256=hashlib.sha256(b).hexdigest()))['bytes'], 16)
        lossy = b.decode('utf-8', errors='replace').encode()
        with self.assertRaises(ValueError):
            verify_blob(lossy, dict(bytes=16, git_blob=blob, sha256=hashlib.sha256(b).hexdigest()))

    def test_same_length_wrong_content_and_false_receipts_fail(self):
        b = b'original'; r = dict(bytes=8, git_blob=hashlib.sha1(b'blob 8\0'+b).hexdigest(),
                               sha256=hashlib.sha256(b).hexdigest())
        for payload, receipt in [(b'altered!', r), (b, dict(r, git_blob='0'*40)),
                                 (b, dict(r, sha256='0'*64)), (b, dict(r, bytes=True))]:
            with self.assertRaises(ValueError): verify_blob(payload, receipt)


class RendererFrames(unittest.TestCase):
    def timeline(self, tempos=()):
        return dict(division_ticks_per_quarter=24, tempo_events=list(tempos))

    def test_default_tempo_and_half_up_rounding(self):
        self.assertEqual(frame_at_tick(self.timeline(), 1, 48000), 1000)
        self.assertEqual(frame_at_tick(self.timeline(), 1, 1), 0)
        self.assertEqual(frame_at_tick(dict(division_ticks_per_quarter=1, tempo_events=[]),1,1),1)

    def test_tempo_changes_apply_after_boundary_and_share_units(self):
        t = self.timeline([dict(tick=24, microseconds_per_quarter=1000000)])
        self.assertEqual(frame_at_tick(t,24,48000),24000)
        self.assertEqual(frame_at_tick(t,48,48000),72000)

    def test_invalid_timing_metadata_fails(self):
        bad = [dict(division_ticks_per_quarter=0, tempo_events=[]),
               self.timeline([dict(tick=0,microseconds_per_quarter=0)]),
               self.timeline([dict(tick=24,microseconds_per_quarter=500000),
                              dict(tick=12,microseconds_per_quarter=500000)])]
        for t in bad:
            with self.assertRaises(ValueError): frame_at_tick(t,48,48000)
        for tick in [-1, True, 1.5]:
            with self.assertRaises(ValueError): frame_at_tick(self.timeline(),tick,48000)

    def test_native_loop_bounds_and_repeat_frames_must_match(self):
        t = self.timeline(); t['markers']=[dict(tick=24,text='['),dict(tick=48,text=']')]
        native=dict(start_frame=24000,end_frame=48000,start_tick=24,end_tick=48)
        r=loop_receipt(t,native,96000)
        self.assertEqual((r['start_frame'],r['end_frame']),(24000,48000))
        self.assertFalse(r['runtime_loop_verified']);self.assertFalse(r['seam_listening_verified'])
        for altered, frames in [(dict(native,end_frame=48001),96000),(native,95999)]:
            with self.assertRaises(ValueError): loop_receipt(t,altered,frames)

    def test_ambiguous_markers_and_silent_loop_substitution_fail(self):
        t=self.timeline();t['markers']=[dict(tick=24,text='[')]
        with self.assertRaises(ValueError): loop_receipt(t,{},96000)
        t['markers']=[]
        native=dict(start_frame=2**64-1,end_frame=2**64-1,midi_end_frame=100)
        self.assertIsNone(loop_receipt(t,native,144100))
        with self.assertRaises(ValueError): loop_receipt(t,dict(native,start_frame=0),144100)


class DecodedPilot(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.path=Path(self.temp.name)/'fixture.wav'

    def wav(self, samples, rate=48000, width=2, channels=2):
        with wave.open(str(self.path),'wb') as w:
            w.setnchannels(channels);w.setsampwidth(width);w.setframerate(rate)
            w.writeframes(struct.pack('<'+'h'*len(samples),*samples))

    def test_pcm_frames_metrics_and_hash_are_not_quality_acceptance(self):
        self.wav([0,0,1000,-1000,0,0,-1000,1000])
        r=wave_receipt(self.path)
        self.assertEqual((r['frames'],r['channels'],r['sample_rate']),(4,2,48000))
        self.assertEqual(r['peak_sample'],1000);self.assertEqual(r['full_scale_samples'],0)
        self.assertEqual(r['sha256'],hashlib.sha256(self.path.read_bytes()).hexdigest())

    def test_empty_silent_clipped_wrong_shape_and_truncated_audio_fail(self):
        for samples in [[],[0,0,0,0],[-32768,32767]]:
            self.wav(samples)
            with self.assertRaises(ValueError):wave_receipt(self.path)
        for kwargs in [dict(rate=44100),dict(channels=1),dict(width=1)]:
            self.wav([1000,-1000],**kwargs)
            with self.assertRaises(ValueError):wave_receipt(self.path)
        self.wav([1000,-1000]);self.path.write_bytes(self.path.read_bytes()[:-1])
        with self.assertRaises(ValueError):wave_receipt(self.path)

    def test_published_pilot_evidence_never_promotes_quality_or_whole_phase(self):
        evidence=json.loads((ROOT/'data/r15/bgm_pilot_evidence.json').read_text())
        self.assertEqual(len(evidence['renders']),4)
        self.assertEqual(len(evidence['source_inputs']['files']),384)
        self.assertEqual(len(evidence['renderer_inputs']['files']),13)
        for key in ['quality_approved','unreal_import_validated','hardware_audio_equivalence_verified']:
            self.assertFalse(evidence[key])
        self.assertEqual(evidence['remaining_music_jingle_jobs'],205)
        jobs={j['source_id']:j for j in json.loads((ROOT/'data/r15/bgm_source_plan.json').read_text())['render_jobs']}
        self.assertEqual(evidence['renderer_pin'],'4000591de6c397b6c80adc07af17144e26b30dfd')
        self.assertEqual(evidence['oracle_source_sha256'],hashlib.sha256(ORACLE.encode()).hexdigest())
        for r in evidence['renders']:
            job=jobs[r['source_id']]
            self.assertEqual(r['source_midi_sha256'],job['source_file']['sha256'])
            self.assertEqual(r['conversion'],job['conversion'])
            self.assertFalse(r['loop_ready'])
            if r['loop_evidence']:
                self.assertEqual(r['frames'],r['loop_evidence']['start_frame']+3*(r['loop_evidence']['end_frame']-r['loop_evidence']['start_frame']))

    def test_all_384_input_receipts_match_actual_pinned_vendor_bytes(self):
        evidence=json.loads((ROOT/'data/r15/bgm_pilot_evidence.json').read_text())
        for name, row in evidence['source_inputs']['files'].items():
            with self.subTest(path=name):
                verify_blob((ROOT/'vendor/vanillaplus'/name).read_bytes(),row)


if __name__=='__main__':unittest.main()
