#!/usr/bin/env python3
"""Render all source music/jingles privately; keep playback/fidelity gates explicit."""
import argparse
import json
import math
import re
from pathlib import Path

from build_r15_bgm_pilot import (ROOT, SOURCE_PIN, RENDERER_PIN, snapshot, render_jobs,
                               loop_receipt, dump)


def bind_inputs(source, renderer, plan, pilot):
    if (source.get('commit') != SOURCE_PIN or plan.get('source_pin') != SOURCE_PIN
            or renderer.get('commit') != RENDERER_PIN or renderer != pilot['renderer_inputs']):
        raise ValueError('pinned source/renderer drift')
    jobs=plan['render_jobs']
    if len(jobs)!=209 or len({j['source_id'] for j in jobs})!=209:
        raise ValueError('source render inventory drift')
    bank={p:r for p,r in pilot['source_inputs']['files'].items() if not p.startswith('sound/songs/midi/')}
    midi={j['source_file']['path']:j['source_file']['sha256'] for j in jobs}
    files=source['files']
    if len(bank)!=380 or len(midi)!=209 or set(files)!=set(bank)|set(midi):
        raise ValueError('source bank/MIDI coverage drift')
    if any(files[p]!=r for p,r in bank.items()) or any(files[p]['sha256']!=sha for p,sha in midi.items()):
        raise ValueError('source differs from pinned bank or MIDI plan')


def validate_coverage(report, plan, pilot):
    if (report.get('schema')!='r15-bgm-coverage-v1' or report.get('source_pin')!=SOURCE_PIN
            or report.get('renderer_pin')!=RENDERER_PIN or report.get('engine_tests_passed')!=707
            or report.get('remaining_music_jingle_jobs')!=0):
        raise ValueError('incomplete or unsupported render report')
    bind_inputs(report['source_inputs'],report['renderer_inputs'],plan,pilot)
    if any(report.get(k) is not False for k in
           ['quality_approved','unreal_import_validated','hardware_audio_equivalence_verified']):
        raise ValueError('offline coverage cannot approve runtime or fidelity')
    jobs={j['source_id']:j for j in plan['render_jobs']}; rows=report['renders']
    if len(rows)!=len(jobs) or {r['source_id'] for r in rows}!=set(jobs):
        raise ValueError('missing, duplicate or unsupported render identity')
    prior={r['source_id']:r for r in pilot['renders']}
    for row in rows:
        job=jobs[row['source_id']]
        if (row['identity']!=job['identity'] or row['symbol']!=job['source_symbol']
                or row['conversion']!=job['conversion']
                or row['source_midi_sha256']!=job['source_file']['sha256']):
            raise ValueError('render identity/source/recipe drift')
        c=job['conversion']
        settings=['--song-volume',str(c['master_volume']),'--reverb',str(c['reverb']),
                  '--sample-rate','48000','--pcm-mix-rate','13379','--polyphony','5',
                  '--loop-count','3','--fadeout','0','--tail','3']
        if c['clocks_per_beat']==48:settings.append('--extended-clocks')
        if (row['settings']!=settings or row['native_midi']['format']!=job['midi']['format']
                or row['native_midi']['division']!=job['midi']['division_ticks_per_quarter']):
            raise ValueError('renderer settings or native MIDI units drift')
        if (type(row['frames']) is not int or row['frames']<=0 or row['bytes']!=44+4*row['frames']
                or row['sample_rate']!=48000 or row['channels']!=2
                or type(row['peak_sample']) is not int or not 0<row['peak_sample']<32767
                or row['full_scale_samples']!=0 or not math.isfinite(row['rms']) or not 0<row['rms']<1
                or not re.fullmatch('[0-9a-f]{64}',row['sha256']) or row['loop_ready'] is not False):
            raise ValueError('failed decoded audio or unsupported acceptance claim')
        loop=loop_receipt(job['midi'],row['native_midi'],row['frames'])
        if row['loop_evidence']!=loop:
            raise ValueError('loop evidence drift')
        if row['source_id'] in prior and any(row[k]!=prior[row['source_id']][k] for k in
                ['sha256','frames','native_midi','loop_evidence','settings']):
            raise ValueError('pilot replay changed rendered bytes or scheduling')
    return dict(renders=len(rows),loops=sum(r['loop_evidence'] is not None for r in rows),
                remaining_music_jingle_jobs=0)


def prepare(source_root,renderer_root,output,public_evidence=None):
    plan=json.loads((ROOT/'data/r15/bgm_source_plan.json').read_text())
    pilot=json.loads((ROOT/'data/r15/bgm_pilot_evidence.json').read_text())
    source=snapshot(source_root,SOURCE_PIN,589)
    renderer=snapshot(renderer_root,RENDERER_PIN,13)
    bind_inputs(source,renderer,plan,pilot)
    report=render_jobs(source_root,renderer_root,output,source,renderer,plan['render_jobs'],209,
                       schema='r15-bgm-coverage-v1')
    result=validate_coverage(report,plan,pilot)
    if public_evidence: public_evidence.write_text(dump(report))
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--sources',type=Path,required=True)
    parser.add_argument('--renderer',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--public-evidence',type=Path)
    args=parser.parse_args()
    print(dump(prepare(args.sources.resolve(),args.renderer.resolve(),args.output.resolve(),args.public_evidence)),end='')
