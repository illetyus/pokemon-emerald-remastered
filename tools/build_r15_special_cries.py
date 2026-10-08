#!/usr/bin/env python3
"""Prepare normal + all source-keyed special cries privately, as audio candidates."""
import argparse
import array
import concurrent.futures
import hashlib
import json
import math
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
from build_r15_audio_catalog import ROOT, PIN, PILOT, dump
from build_r15_audio_pilot import MODERN_PIN, pcm, stats, wav_bytes
from build_r15_cry_modes import build as build_recipes, RATE, CLOCK, FRAME_CYCLES
from fetch_r15_modern_cries import verify
from validate_r15_audio_pack import strict, validate

PEAK_CEILING = 32768 * 10 ** (-3 / 20)

def release_curve(release):
    if type(release) is not int or not 0 <= release < 256:
        raise ValueError('invalid release byte')
    levels = [255]
    while levels[-1]:
        levels.append(levels[-1] * release >> 8)
    return levels

def render(tracks, recipe):
    """Tracks contain resampler headroom; wide mixing avoids intermediate clipping."""
    values = recipe['source_parameters']
    curve = release_curve(values['release'])
    gate = recipe['gate_seconds'] * RATE
    frame_samples = RATE * FRAME_CYCLES / CLOCK
    count = min(max(map(len, tracks)), math.ceil(gate + (len(curve)-1) * frame_samples))
    mixed = array.array('d')
    for i in range(count):
        gain = recipe['relative_volume'] * 2  # undo fixed resampler headroom
        if i >= gate:
            position = (i - gate) / frame_samples
            n = int(position); fraction = position - n
            a = curve[min(n, len(curve)-1)]
            b = curve[min(n+1, len(curve)-1)]
            gain *= (a + (b-a) * fraction) / 255
        mixed.append(sum(t[i] for t in tracks if i < len(t)) * gain)
    if not mixed:
        raise ValueError('empty cry mode')
    peak = max(abs(v) for v in mixed)
    if peak < 1:
        raise ValueError('silent cry mode')
    guard = min(1.0, PEAK_CEILING / peak)
    result = array.array('h', (round(v * guard) for v in mixed))
    return result, guard

def transformed(normal_path, recipes, work):
    rates = sorted({t['render_input_rate'] for r in recipes for t in r['tracks']})
    filters = ['[0:a]volume=0.5,asplit=' + str(len(rates)) + ''.join(f'[in{i}]' for i in range(len(rates)))]
    cmd = ['ffmpeg','-nostdin','-hide_banner','-loglevel','error','-y','-i',str(normal_path)]
    for i, rate in enumerate(rates):
        filters.append(f'[in{i}]asetrate={rate},aresample=48000:resampler=swr:filter_size=32:phase_shift=10:exact_rational=1:dither_method=none[out{i}]')
    cmd += ['-filter_complex', ';'.join(filters), '-filter_complex_threads','1']
    for i, rate in enumerate(rates):
        cmd += ['-map',f'[out{i}]','-map_metadata','-1','-ac','1','-c:a','pcm_s16le',
                '-fflags','+bitexact','-flags:a','+bitexact',str(work/f'{rate}.wav')]
    subprocess.run(cmd,check=True,capture_output=True)
    return {rate:pcm(work/f'{rate}.wav') for rate in rates}

def assemble_entries(catalog, candidates):
    expected = {f'cry.{dex}.mode.{mode}' for dex in range(1,387) for mode in range(13)}
    if set(candidates) != expected:
        raise ValueError('complete exact cry-mode coverage required')
    return ([{'identity':s['identity'],'status':'missing','reason':'music/effect/fanfare/phoneme render not prepared'}
             for s in catalog['songs'] if s['source_id']] +
            [candidates[f'cry.{dex}.mode.{mode}'] for dex in range(1,387) for mode in range(13)])

def checked_normal_pack(source):
    manifest = strict((source/'modern_manifest.json').read_text())
    probe = validate(manifest, source)
    candidates = {e['identity']:e for e in manifest['entries'] if e['status']=='candidate'}
    if set(candidates) != {f'cry.{i}.mode.0' for i in range(1,387)}:
        raise ValueError('input must be the complete prepared normal pack')
    receipts = strict((source/'source_receipts.json').read_text())
    if set(receipts) != {str(i) for i in range(1,387)}:
        raise ValueError('complete master receipts required')
    for dex in range(1,387):
        entry = candidates[f'cry.{dex}.mode.0']
        raw = (source/'masters'/f'{dex:03d}.ogg').read_bytes()
        verify(raw, receipts[str(dex)])
        p = entry['provenance']
        if (p['repository'],p['commit'],p['source_path'],p['source_sha256']) != (
                'PokeAPI/cries', MODERN_PIN, f'cries/pokemon/latest/{dex}.ogg', hashlib.sha256(raw).hexdigest()):
            raise ValueError('normal/master provenance mismatch')
    return candidates, receipts, probe

def build(source, output):
    source = Path(source).resolve(); output = Path(output).resolve()
    if output.is_relative_to(ROOT) and not output.is_relative_to(ROOT/'local'):
        raise ValueError('commercial output must be private/ignored')
    if source == output or (output.exists() and any(output.iterdir())):
        raise ValueError('new empty output directory required')
    normals, receipts, _ = checked_normal_pack(source)
    selection = strict((ROOT/'data/r15/modern_selection.json').read_text())
    if not selection.get('pilot_style_approved') or selection.get('selected_profile') != 'modern':
        raise ValueError('modern pilot selection required')
    recipes = build_recipes()
    recipe_sha = hashlib.sha256(dump(recipes).encode()).hexdigest()
    catalog = strict((ROOT/'data/r15/source_audio_catalog.json').read_text())
    output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(source/'masters',output/'masters')
    shutil.copytree(source/'normal',output/'normal')
    (output/'special').mkdir()
    for mode in range(1,13): (output/'special'/f'mode_{mode:02d}').mkdir()
    candidates = dict(normals); measurements = []
    ffmpeg = subprocess.run(['ffmpeg','-version'],capture_output=True,text=True,check=True).stdout.splitlines()[0]
    with tempfile.TemporaryDirectory(prefix='r15-special-') as scratch:
        def one(dex):
            row = normals[f'cry.{dex}.mode.0']; normal_path = source/row['path']
            a = pcm(normal_path); work = Path(scratch)/str(dex);work.mkdir()
            forward = transformed(normal_path,recipes['modes'][1:],work)
            reverse_folder = work/'reverse';reverse_folder.mkdir()
            reverse_path = reverse_folder/'source.wav';reverse_path.write_bytes(wav_bytes(reversed(a)))
            reverse = transformed(reverse_path,[r for r in recipes['modes'] if r['source_parameters']['reverse']],reverse_folder)
            result = []
            for r in recipes['modes'][1:]:
                bank = reverse if r['source_parameters']['reverse'] else forward
                rendered, guard = render([bank[t['render_input_rate']] for t in r['tracks']],r)
                relative = f'special/mode_{r["id"]:02d}/{dex:03d}.wav';path = output/relative
                payload = wav_bytes(rendered);path.write_bytes(payload);measured = stats(rendered)
                if measured['full_scale_samples'] or measured['duration_seconds'] > 15:
                    raise ValueError('invalid rendered mode')
                entry = dict(row, identity=f'cry.{dex}.mode.{r["id"]}', path=relative,
                             sha256=hashlib.sha256(payload).hexdigest(), frames=len(rendered))
                entry['processing'] = {'kind':'authored_modern_special_candidate','recipe_sha256':recipe_sha,
                    'source_parameter_pin':PIN,'base_normal_sha256':row['sha256'],'mode':r['id'],
                    'peak_guard_gain':guard,'hardware_audio_equivalence_verified':False}
                result.append((entry,{'identity':entry['identity'],'sha256':entry['sha256'],
                    'base_normal_sha256':row['sha256'],'recipe_sha256':recipe_sha,
                    'peak_guard_gain':guard,**measured}))
            return result
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            for index, rows in enumerate(pool.map(one,range(1,387)),1):
                for entry, measured in rows:
                    candidates[entry['identity']] = entry; measurements.append(measured)
                if index % 25 == 0:print(f'{index}/386 species: 12 special modes prepared',flush=True)
    manifest = {'schema':'r15-local-audio-v1','profile':'modern','entries':assemble_entries(catalog,candidates)}
    probe = validate(manifest,output)
    evidence = {'schema':'r15-special-cries-evidence-v1','source_commit':MODERN_PIN,
        'source_parameter_pin':PIN,'recipe_sha256':recipe_sha,'ffmpeg':ffmpeg,
        'counts':{'species':386,'prepared_normal_cries':386,'prepared_special_candidates':len(measurements),
            'total_cry_candidates':len(candidates),'missing_cry_semantics':0,'missing_other_audio_semantics':609,
            'unrepresented_semantics':probe['unrepresented_semantics'],
            'full_scale_samples':sum(m['full_scale_samples'] for m in measurements),'verified_unreal_imports':0},
        'special_pcm_bytes':sum(m['frames']*2 for m in measurements),
        'special_duration_seconds':sum(m['duration_seconds'] for m in measurements),
        'pilot_style_approved':True,'special_mode_listening_approved':False,
        'hardware_audio_equivalence_verified':False,'unreal_import_validated':False,
        'processing':recipes['render_policy'],'modes':measurements}
    evidence['manifest_sha256'] = hashlib.sha256(dump(manifest).encode()).hexdigest()
    for filename, data in [('modern_manifest.json',manifest),('preimport_probe.json',probe),
                           ('source_receipts.json',receipts),('cry_mode_recipes.json',recipes),('special_evidence.json',evidence)]:
        (output/filename).write_text(dump(data))
    # A short separately usable audition file keeps the complete ZIP optional.
    combined = array.array('h',[0]*RATE); timeline = []
    for dex in PILOT:
        for mode in [0,11,5,7,8,9,10]:
            entry = candidates[f'cry.{dex}.mode.{mode}']; a = pcm(output/entry['path'])
            start = len(combined)/RATE;combined.extend(a)
            timeline.append({'dex':dex,'mode':mode,'start_seconds':round(start,3),'end_seconds':round(len(combined)/RATE,3)})
            combined.extend([0]*24000)
    listening = output/'Pokemon_Ozel_Ses_Dinleme.wav';listening.write_bytes(wav_bytes(combined))
    (output/'DINLEME_KILAVUZU.txt').write_text('Her Pokémon: normal → zayıf → bayılma → Roar 1 → Roar 2 → Growl 1 → Growl 2.\n'
        'Bu sesler modern kayıtlar üzerine Emerald kaynak parametrelerinden türetilmiş adaylardır.\n'
        'Bütün özel kayıtların tek tek dinlenmesi ve kaynak oyunla karşılaştırılması henüz yapılmadı.\n'
        'Gerçek Unreal/Android aktarımı ve kalite onayı tamamlanmadı.\n\n' +
        '\n'.join(f'{t["start_seconds"]:.2f}–{t["end_seconds"]:.2f} sn | Pokémon {t["dex"]:03d} | biçim {t["mode"]}' for t in timeline)+'\n')
    (output/'KULLANIM.txt').write_text('386 Pokémon × 13 biçim: 5.018 çığlık adayı.\n'
        'normal/: önceki onaylı tarzdaki normal sesler değiştirilmeden korundu.\n'
        'special/mode_01–12/: kaynaktan türetilmiş özel biçim adayları. masters/: orijinal modern OGG kayıtları.\n'
        '609 müzik/efekt/melodi kaydı halen eksik. Manifestte eksikler açıkça yazılıdır.\n'
        'Perde ve chorus hesapları kaynak motorun tablolarından çıkarıldı. Genel bir yankı efekti uygulanmadı.\n'
        'Salınım kesilmesi yumuşatıldı; taşmayı önlemek için gerektiğinde yalnızca ses kısıldı. Özel biçimler eşit ses yüksekliğine zorlanmadı.\n'
        'Bu dosyalar kaynak motorun bire bir donanım çıktısı olarak onaylanmış değildir.\n'
        'Ses uzunlukları oyun içindeki beklemeleri veya savaş sonucunu değiştiremez.\n'
        'Seslerin telif hakkı The Pokémon Company’ye aittir; arşiv lisansı bu hakkı değiştirmez.\n')
    archive = output.parent/'Pokemon_386_Ozel_Ses_Adaylari.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for path in sorted(output.rglob('*')):
            if path.is_file(): z.write(path,path.relative_to(output))
    with archive.open('rb') as stream: evidence['archive_sha256'] = hashlib.file_digest(stream,'sha256').hexdigest()
    evidence['archive_bytes'] = archive.stat().st_size
    # Public summary includes no audio payloads and keeps the larger per-file manifest private.
    summary = {k:v for k,v in evidence.items() if k != 'modes'}
    summary['per_mode'] = [{'mode':m,'files':sum(x['identity'].endswith(f'.mode.{m}') for x in measurements),
        'pcm_bytes':sum(x['frames']*2 for x in measurements if x['identity'].endswith(f'.mode.{m}'))} for m in range(1,13)]
    return summary, archive

if __name__ == '__main__':
    p = argparse.ArgumentParser();p.add_argument('--normal-pack',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--public-evidence',type=Path)
    a=p.parse_args();evidence,archive=build(a.normal_pack,a.output)
    if a.public_evidence:a.public_evidence.write_text(dump(evidence))
    print(dump({'archive':str(archive),'counts':evidence['counts'],'archive_bytes':evidence['archive_bytes'],
                'special_duration_seconds':evidence['special_duration_seconds']}),end='')
