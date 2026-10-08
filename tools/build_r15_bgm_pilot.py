#!/usr/bin/env python3
"""Prepare four private source-render candidates; never claim runtime fidelity."""
import argparse
import array
import hashlib
import json
import math
import re
import shutil
import subprocess
import sys
import tempfile
import wave
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'
RENDERER_PIN = '4000591de6c397b6c80adc07af17144e26b30dfd'
SELECTED = {'mus_littleroot', 'mus_vs_wild', 'mus_level_up', 'mus_encounter_suspicious'}
RATE = 48000
ENGINE = ['plugin/' + name + '.c' for name in
          ['m4a_engine', 'm4a_channel', 'm4a_tables', 'm4a_reverb', 'voicegroup_loader']]
ORACLE = r'''#define main pinned_renderer_main
#include "cmd/poryaaaa_render.c"
#undef main
int main(int argc, char **argv) {
    if (argc != 2) return 1;
    uint64_t total=0, start=0, end=0, startTick=0, endTick=0;
    uint32_t division=0; uint16_t format=0; TempoArray tempos={0};
    RenderEventArray *events=parse_midi(argv[1],48000.0,&total,&start,&end,
                                      &startTick,&endTick,&division,&tempos,&format);
    if (!events) return 2;
    printf("{\"events\":%d,\"midi_end_frame\":%llu,\"start_frame\":%llu,\"end_frame\":%llu,\"start_tick\":%llu,\"end_tick\":%llu,\"division\":%u,\"format\":%u}\n",
           events->count,(unsigned long long)total,(unsigned long long)start,
           (unsigned long long)end,(unsigned long long)startTick,
           (unsigned long long)endTick,division,format);
    free(events->events);free(events);free(tempos.events);return 0;
}
'''


def dump(value):
    return json.dumps(value, sort_keys=True, indent=2) + '\n'


def verify_blob(data, expected):
    blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
    sha = hashlib.sha256(data).hexdigest()
    if (type(expected.get('bytes')) is not int or expected['bytes'] != len(data)
            or expected.get('git_blob') != blob or expected.get('sha256') != sha):
        raise ValueError('source byte/hash receipt mismatch')
    return dict(bytes=len(data), git_blob=blob, sha256=sha)


def snapshot(root, pin, required_count):
    receipt = json.loads((root / 'source_receipts.json').read_text())
    files = receipt.get('files', {})
    if receipt.get('commit') != pin or len(files) != required_count:
        raise ValueError('unexpected source snapshot pin/coverage')
    verified = {}
    for name, row in files.items():
        p = Path(name)
        if p.is_absolute() or '..' in p.parts or '\\' in name or str(p) != name:
            raise ValueError('unsafe source path')
        target = root / p
        if any(parent.is_symlink() for parent in [target, *target.parents]) or not target.resolve().is_relative_to(root.resolve()):
            raise ValueError('source symlink/escape')
        if target.stat().st_size > 8 * 1024 * 1024:
            raise ValueError('source audit byte bound')
        verified[name] = verify_blob(target.read_bytes(), row)
    return dict(commit=pin, files=verified)


def frame_at_tick(timeline, tick, rate):
    division = timeline['division_ticks_per_quarter']
    if (type(tick) is not int or tick < 0 or type(rate) is not int or rate <= 0
            or type(division) is not int or not 0 < division < 0x8000):
        raise ValueError('invalid timeline units')
    elapsed = Fraction(0); previous = 0; tempo = 500000; last_event = -1
    for event in timeline['tempo_events']:
        t, value = event['tick'], event['microseconds_per_quarter']
        if (type(t) is not int or t < 0 or t < last_event or type(value) is not int
                or not 0 < value <= 0xffffff):
            raise ValueError('invalid tempo timeline')
        last_event = t
        if t < tick:
            elapsed += Fraction((t-previous) * tempo * rate, division * 1000000)
            previous = t; tempo = value
    elapsed += Fraction((tick-previous) * tempo * rate, division * 1000000)
    rounded = elapsed + Fraction(1, 2)
    return rounded.numerator // rounded.denominator


def loop_receipt(timeline, native, frames):
    markers = timeline['markers']; sentinel = 2**64 - 1
    if not markers:
        if (native.get('start_frame') != sentinel or native.get('end_frame') != sentinel
                or frames != native.get('midi_end_frame', -1) + RATE * 3):
            raise ValueError('one-shot native/tail frame mismatch')
        return None
    if [m['text'] for m in markers] != ['[', ']']:
        raise ValueError('pilot does not support ambiguous source loops')
    start, end = (frame_at_tick(timeline, m['tick'], RATE) for m in markers)
    if (not 0 <= start < end <= frames
            or native.get('start_frame') != start or native.get('end_frame') != end
            or native.get('start_tick') != markers[0]['tick'] or native.get('end_tick') != markers[1]['tick']
            or frames != start + 3 * (end-start)):
        raise ValueError('native loop/timeline/repeat frame mismatch')
    return dict(start_frame=start, end_frame=end, repetitions=3,
                authority='pinned renderer MIDI parser and independent rational timeline; not GBA runtime',
                runtime_loop_verified=False, seam_listening_verified=False)


def wave_receipt(path):
    peak = clipped = energy = count = 0
    try:
        with wave.open(str(path), 'rb') as w:
            frames = w.getnframes()
            if (w.getframerate(), w.getnchannels(), w.getsampwidth(), w.getcomptype()) != (RATE, 2, 2, 'NONE') or frames <= 0:
                raise ValueError('pilot requires nonempty stereo PCM16/48k')
            while True:
                b = w.readframes(65536)
                if not b: break
                if len(b) % 4: raise ValueError('truncated PCM frame')
                samples = array.array('h', b)
                if sys.byteorder != 'little': samples.byteswap()
                count += len(samples); peak = max(peak, max(abs(v) for v in samples))
                clipped += sum(v in (-32768, 32767) for v in samples)
                energy += sum(v*v for v in samples)
    except wave.Error as e:
        raise ValueError('invalid WAV') from e
    if count != frames * 2 or not energy or clipped:
        raise ValueError('truncated, silent or full-scale pilot output')
    return dict(frames=frames, channels=2, sample_rate=RATE, bytes=path.stat().st_size,
                sha256=hashlib.sha256(path.read_bytes()).hexdigest(), peak_sample=peak,
                full_scale_samples=clipped, rms=math.sqrt(energy/count)/32768)


def prepare(source_root, renderer_root, output):
    if output.exists() and any(output.iterdir()):
        raise ValueError('new empty pilot output required')
    source = snapshot(source_root, SOURCE_PIN, 384)
    renderer = snapshot(renderer_root, RENDERER_PIN, 13)
    # Initial evidence was bootstrapped from actual pinned GitHub blob receipts.
    # Replays bind to that versioned receipt rather than trusting a new local
    # manifest that could rewrite both a file and its claimed hashes.
    baseline_path = ROOT / 'data/r15/bgm_pilot_evidence.json'
    if baseline_path.is_file():
        baseline = json.loads(baseline_path.read_text())
        if source != baseline['source_inputs'] or renderer != baseline['renderer_inputs']:
            raise ValueError('snapshot differs from versioned pinned receipts')
    plan = json.loads((ROOT / 'data/r15/bgm_source_plan.json').read_text())
    if plan['source_pin'] != SOURCE_PIN: raise ValueError('source plan pin drift')
    jobs = [j for j in plan['render_jobs'] if j['source_symbol'] in SELECTED]
    if len(jobs) != 4: raise ValueError('pilot identity drift')
    compiler = shutil.which('cc')
    if not compiler: raise ValueError('cc required for pinned renderer and oracle')
    output.mkdir(parents=True, exist_ok=True); renders = []; manifest = []
    compiler_version = subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
    with tempfile.TemporaryDirectory() as tmp:
        temp = Path(tmp); oracle_source = temp / 'oracle.c'; oracle_source.write_text(ORACLE)
        includes = [arg for p in [renderer_root, renderer_root/'plugin', renderer_root/'third_party'] for arg in ['-I', str(p)]]
        builds = [('render', renderer_root/'cmd/poryaaaa_render.c'),
                  ('oracle', oracle_source), ('engine_tests', renderer_root/'test/test_engine.c')]
        binaries = {}
        for name, entry in builds:
            args = [compiler, '-std=gnu11', '-O2', *includes, str(entry),
                    *[str(renderer_root/p) for p in ENGINE], '-lm', '-lpthread', '-ldl', '-o', str(temp/name)]
            subprocess.run(args, check=True, capture_output=True, timeout=120)
            binaries[name] = hashlib.sha256((temp/name).read_bytes()).hexdigest()
        engine = subprocess.run([str(temp/'engine_tests')], check=True, capture_output=True, text=True, timeout=60)
        if not re.search(r'=== Results: 707/707 tests passed ===', engine.stdout):
            raise ValueError('pinned renderer engine test gate failed')
        (output/'engine_tests.log').write_text(engine.stdout + engine.stderr)
        for job in jobs:
            c = job['conversion']; symbol = job['source_symbol']; path = job['source_file']['path']
            if source['files'].get(path, {}).get('sha256') != job['source_file']['sha256']:
                raise ValueError('pilot MIDI provenance mismatch')
            native = json.loads(subprocess.check_output([str(temp/'oracle'), str(source_root/path)], text=True))
            if (native['format'], native['division']) != (job['midi']['format'], job['midi']['division_ticks_per_quarter']):
                raise ValueError('native parser units mismatch')
            wav = output/(symbol+'.wav')
            settings = ['--song-volume', str(c['master_volume']), '--reverb', str(c['reverb']),
                        '--sample-rate', '48000', '--pcm-mix-rate', '13379', '--polyphony', '5',
                        '--loop-count', '3', '--fadeout', '0', '--tail', '3']
            if c['clocks_per_beat'] == 48: settings.append('--extended-clocks')
            run = subprocess.run([str(temp/'render'), str(source_root), f"voicegroup{c['voicegroup']:03d}",
                '--midi', str(source_root/path), '--output', str(wav), *settings],
                check=True, capture_output=True, text=True, timeout=120)
            if run.stderr or re.search(r'warning|missing|failed|error', run.stdout, re.I):
                raise ValueError('renderer reported unresolved input or warning')
            (output/(symbol+'.log')).write_text(run.stdout)
            decoded = wave_receipt(wav); loop = loop_receipt(job['midi'], native, decoded['frames'])
            renders.append(dict(source_id=job['source_id'], identity=job['identity'], symbol=symbol,
                source_midi_sha256=job['source_file']['sha256'], conversion=c, settings=settings,
                native_midi=native, loop_evidence=loop, loop_ready=False, **decoded))
            manifest.append(dict(identity=job['identity'], status='candidate', path=wav.name,
                sha256=decoded['sha256'], sample_rate=RATE, channels=2, frames=decoded['frames'],
                gain=1.0, fade_seconds=job['transition']['crossfade_seconds'], loop=None,
                provenance=dict(repository='illetyus/pokezumrut-vanillaplus',commit=SOURCE_PIN,
                    source_path=path, source_sha256=job['source_file']['sha256'],
                    rights='Pokemon source music copyright The Pokemon Company; private render candidate')))
    (output/'manifest.json').write_text(dump(dict(schema='r15-local-audio-v1',profile='original',entries=manifest)))
    report = dict(schema='r15-bgm-pilot-v1', source_pin=SOURCE_PIN, renderer_pin=RENDERER_PIN,
        source_inputs=source, renderer_inputs=renderer, compiler=compiler_version,
        compile_flags=['-std=gnu11', '-O2'], binaries=binaries,
        oracle_source_sha256=hashlib.sha256(ORACLE.encode()).hexdigest(), engine_tests_passed=707,
        renders=renders, remaining_music_jingle_jobs=len(plan['render_jobs'])-len(renders),
        quality_approved=False, unreal_import_validated=False, hardware_audio_equivalence_verified=False)
    (output/'evidence.json').write_text(dump(report))
    return report


if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--sources',type=Path,required=True)
    parser.add_argument('--renderer',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--public-evidence',type=Path)
    args=parser.parse_args(); report=prepare(args.sources.resolve(),args.renderer.resolve(),args.output.resolve())
    if args.public_evidence: args.public_evidence.write_text(dump(report))
    print(dump(dict(renders=len(report['renders']), engine_tests_passed=report['engine_tests_passed'],
                    remaining_music_jingle_jobs=report['remaining_music_jingle_jobs'])),end='')
