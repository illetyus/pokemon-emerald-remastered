import hashlib
import json
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from build_r15_bgm_plan import build, dump, midi_timeline, recipes


def midi(*tracks, division=96, format=1):
    return (b'MThd' + struct.pack('>IHHH', 6, format, len(tracks), division)
            + b''.join(b'MTrk' + struct.pack('>I', len(t)) + t for t in tracks))


EOT = b'\x00\xff\x2f\x00'


class MidiTimeline(unittest.TestCase):
    def test_intro_loop_ticks_and_tempo_stay_source_units(self):
        track = (b'\x00\xff\x51\x03\x07\xa1\x20'
                 b'\x60\xff\x06\x01['
                 b'\x60\xff\x06\x01]' + EOT)
        t = midi_timeline(midi(track))
        self.assertEqual(t['markers'], [dict(tick=96, text='[', meta_type=6),
                                       dict(tick=192, text=']', meta_type=6)])
        self.assertEqual(t['tempo_events'], [dict(tick=0, microseconds_per_quarter=500000)])
        self.assertEqual(t['division_ticks_per_quarter'], 96)
        self.assertNotIn('start_frame', t)

    def test_first_track_is_converter_sequence_authority(self):
        t = midi_timeline(midi(EOT, b'\x00\xff\x06\x01[' + EOT))
        self.assertEqual(t['markers'], [])
        self.assertEqual(t['ignored_other_track_sequence_events'], 1)

    def test_all_converter_text_types_and_multiple_blocks_preserved(self):
        track = b''.join(bytes([0, 255, kind, 1]) + b'[' for kind in range(1, 8))
        track += b'\x01\xff\x01\x02][' + b'\x01\xff\x01\x01:' + EOT
        t = midi_timeline(midi(track))
        self.assertEqual([r['meta_type'] for r in t['markers'][:7]], list(range(1, 8)))
        self.assertEqual([r['text'] for r in t['markers'][-2:]], ['][', ':'])

    def test_running_status_and_sysex(self):
        track = b'\x00\x90\x3c\x40\x60\x3c\x00\x00\xf0\x02\x01\xf7' + EOT
        self.assertEqual(midi_timeline(midi(track))['track_end_ticks'], [96])

    def test_duplicate_end_event_ignored_with_receipt_like_pinned_converter(self):
        t = midi_timeline(midi(EOT + EOT))
        self.assertEqual(t['track_end_ticks'], [0])
        self.assertEqual(t['ignored_post_eot'], [dict(bytes=4, sha256=hashlib.sha256(EOT).hexdigest())])

    def test_source_eight_bit_controller_value_is_recorded_not_clamped(self):
        t = midi_timeline(midi(b'\x00\xb0\x07\x80' + EOT))
        self.assertEqual(t['nonstandard_controller_values'],
                         [dict(track=0, tick=0, controller=7, value=128)])
        with self.assertRaises(ValueError):
            midi_timeline(midi(b'\x00\x90\x3c\x80' + EOT))

    def test_malformed_truncated_or_unterminated_tracks_fail(self):
        for blob in [b'', midi(EOT)[:-1], midi(b'\x00\x90\x3c\x40'),
                     midi(b'\x00\xff\x2f\x01\x00'), midi(EOT) + b'extra',
                     midi(b'\x00\x3c\x40' + EOT),
                     midi(b'\x00\xff\x51\x02\x07\xa1' + EOT),
                     midi(b'\x00\xff\x51\x03\x00\x00\x00' + EOT),
                     midi(b'\x81\x80\x80\x80\x00\xff\x2f\x00')]:
            with self.subTest(blob=blob), self.assertRaises(ValueError):
                midi_timeline(blob)

    def test_unsupported_format_and_time_division_fail(self):
        for kwargs in [dict(format=2), dict(division=0), dict(division=0xe728), dict(format=0)]:
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                midi_timeline(midi(EOT, EOT, **kwargs))


class SourcePlan(unittest.TestCase):
    def test_all_209_recipes_against_compiled_pinned_mid2agb(self):
        compiler = shutil.which('g++')
        self.assertIsNotNone(compiler, 'g++ required for pinned converter oracle')
        vendor = ROOT / 'vendor/vanillaplus'
        tool = vendor / 'tools/mid2agb'
        with tempfile.TemporaryDirectory() as temp:
            executable = Path(temp) / 'mid2agb'
            sources = [tool / (name + '.cpp') for name in ['main', 'agb', 'midi', 'tables', 'error']]
            subprocess.run([compiler, '-std=c++17', '-O2', *map(str, sources), '-o', str(executable)],
                           check=True, capture_output=True, timeout=60)
            for job in build()['render_jobs']:
                with self.subTest(identity=job['identity']):
                    c = job['conversion']; target = Path(temp) / (job['source_symbol'] + '.s')
                    args = ['-G' + str(c['voicegroup']), '-V' + str(c['master_volume']),
                            '-P' + str(c['priority'])]
                    if c['reverb'] >= 0: args.append('-R' + str(c['reverb']))
                    if c['exact_gate_time']: args.append('-E')
                    if not c['compression']: args.append('-N')
                    if c['clocks_per_beat'] == 48: args.append('-X')
                    subprocess.run([str(executable), str(vendor / job['source_file']['path']),
                                    str(target), *args], check=True, capture_output=True, timeout=10)
                    text = target.read_text(); symbol = job['source_symbol']
                    for key, suffix in [('master_volume', 'mvl'), ('priority', 'pri')]:
                        self.assertRegex(text, rf'\.equ\s+{symbol}_{suffix}, {c[key]}\s')
                    self.assertIn(f'{symbol}_grp, voicegroup{c["voicegroup"]:03d}', text)
                    self.assertIn(f'{symbol}_rev, reverb_set+{c["reverb"]}', text)
                    for suffix, value in [('exg', int(c['exact_gate_time'])),
                                          ('cmp', int(c['compression'])),
                                          ('tbs', c['clocks_per_beat'] // 24)]:
                        self.assertRegex(text, rf'\.equ\s+{symbol}_{suffix}, {value}\s')
                    tracks = int(re.search(r'\.byte\s+(\d+)\s+@ NumTrks', text)[1])
                    self.assertGreater(tracks, 0)
                    source_loop = [m['text'] for m in job['midi']['markers']] == ['[', ']']
                    self.assertEqual(len(re.findall(r'\.byte\s+GOTO\b', text)), tracks if source_loop else 0)

    def test_conversion_defaults_and_make_overrides(self):
        text = ('STD_REVERB = 50\n$(MID_SUBDIR)/mus_test.s: %.s: %.mid\n'
                '\t$(MID) $< $@ -E -R$(STD_REVERB) -G051 -V100 -P5\n'
                '$(MID_SUBDIR)/mus_default.s: %.s: %.mid\n\t$(MID) $< $@ -E -R40\n')
        r = recipes(text)
        self.assertEqual(r['mus_test'], dict(voicegroup=51, master_volume=100,
            priority=5, reverb=50, exact_gate_time=True, compression=True, clocks_per_beat=24))
        self.assertEqual(r['mus_default']['voicegroup'], 0)
        self.assertEqual(r['mus_default']['master_volume'], 127)

    def test_unknown_duplicate_or_out_of_range_recipe_rejected(self):
        base = 'STD_REVERB = 50\n$(MID_SUBDIR)/mus_test.s: %.s: %.mid\n\t$(MID) $< $@ '
        for text in [base + '-Z', base + '-V128', base + '-G-1', base + '-R$(OTHER)',
                     base + '-E\n' + base, base + '-V80 -V90']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                recipes(text)

    def test_generated_output_and_independent_source_counts(self):
        p = build()
        self.assertEqual((ROOT / 'data/r15/bgm_source_plan.json').read_text(), dump(p))
        self.assertEqual(p['counts']['renderable_music'], 191)
        self.assertEqual(p['counts']['renderable_jingles'], 18)
        self.assertEqual(p['counts']['reserved_zero_track_rows'], 80)
        self.assertEqual([r['source_id'] for r in p['reserved']], list(range(270, 350)))
        self.assertEqual(len(p['render_jobs']), 209)
        self.assertEqual(len({r['identity'] for r in p['render_jobs']}), 209)
        self.assertFalse(p['unreal_import_validated'])
        self.assertFalse(p['hardware_audio_equivalence_verified'])
        for r in p['render_jobs']:
            self.assertIsNone(r['loop_pcm_frames'])
            self.assertFalse(r['render_verified'])

    def test_representative_jobs_and_source_hashes(self):
        p = build(); jobs = {r['source_symbol']: r for r in p['render_jobs']}
        self.assertEqual(jobs['mus_littleroot']['conversion']['voicegroup'], 51)
        self.assertEqual(jobs['mus_littleroot']['conversion']['master_volume'], 100)
        self.assertEqual(jobs['mus_level_up']['conversion']['priority'], 5)
        self.assertEqual(jobs['mus_gsc_route38']['conversion']['voicegroup'], 0)
        self.assertEqual(len(jobs['mus_encounter_suspicious']['midi']['nonstandard_controller_values']), 9)
        vendor = ROOT / 'vendor/vanillaplus'
        for path, sha in p['source_hashes'].items():
            self.assertEqual(hashlib.sha256((vendor / path).read_bytes()).hexdigest(), sha)
        for r in p['render_jobs']:
            self.assertEqual(hashlib.sha256((vendor / r['source_file']['path']).read_bytes()).hexdigest(), r['source_file']['sha256'])


if __name__ == '__main__':
    unittest.main()
