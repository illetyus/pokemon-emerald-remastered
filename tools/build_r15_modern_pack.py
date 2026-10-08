#!/usr/bin/env python3
"""Build all 386 normal modern cries privately. Other semantics stay explicitly missing."""
import argparse
import concurrent.futures
import hashlib
import json
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path
from build_r15_audio_catalog import ROOT, dump
from build_r15_audio_pilot import MODERN_PIN, decode, pcm, audition, stats, wav_bytes
from fetch_r15_modern_cries import verify
from validate_r15_audio_pack import validate

def assemble_entries(catalog,normal):
    if set(normal)!=set(range(1,387)):raise ValueError('386 normal cries required')
    entries=[]
    for song in catalog['songs'][1:]:
        entries.append({'identity':song['identity'],'status':'missing','reason':'music/effect/fanfare/phoneme local render not prepared'})
    for cry in catalog['cries']:
        dex=cry['national_dex'];entries.append(normal[dex]['entry'])
        for mode in range(1,13):entries.append({'identity':cry['identity']+f'.mode.{mode}','status':'missing','reason':'requires source-faithful cry-mode processing; never substitute mode.0'})
    return entries

def build(source,output):
    source=Path(source).resolve();output=Path(output).resolve()
    if output.is_relative_to(ROOT) and not output.is_relative_to(ROOT/'local'):raise ValueError('commercial output must be outside git or in ignored local/')
    if output.exists() and any(output.iterdir()):raise ValueError('use an empty output directory')
    selection=json.loads((ROOT/'data/r15/modern_selection.json').read_text())
    if selection.get('selected_profile')!='modern' or not selection.get('pilot_style_approved'):raise ValueError('pilot style selection required')
    receipts=json.loads((source/'receipts.json').read_text())
    if set(receipts)!={str(i) for i in range(1,387)}:raise ValueError('complete source receipts required')
    catalog=json.loads((ROOT/'data/r15/source_audio_catalog.json').read_text());cries=catalog['cries']
    if len(cries)!=386 or {r['national_dex'] for r in cries}!=set(range(1,387)):raise ValueError('complete source identities required')
    output.mkdir(parents=True,exist_ok=True)
    for folder in ['masters','normal']: (output/folder).mkdir()
    ffmpeg=subprocess.run(['ffmpeg','-version'],check=True,capture_output=True,text=True).stdout.splitlines()[0]
    with tempfile.TemporaryDirectory(prefix='r15-pcm-') as work:
        def one(cry):
            dex=cry['national_dex'];raw=(source/f'{dex}.ogg').read_bytes();verify(raw,receipts[str(dex)])
            master=output/'masters'/f'{dex:03d}.ogg';master.write_bytes(raw)
            decoded=Path(work)/f'{dex}.wav';decode(master,decoded);a=pcm(decoded);normal,gain=audition(a)
            path=output/'normal'/f'{dex:03d}.wav';payload=wav_bytes(normal);path.write_bytes(payload)
            measured=stats(normal)
            if measured['full_scale_samples'] or measured['duration_seconds']>15:raise ValueError(f'invalid normal cry: {dex}')
            provenance={'repository':'PokeAPI/cries','commit':MODERN_PIN,'source_path':f'cries/pokemon/latest/{dex}.ogg','source_sha256':hashlib.sha256(raw).hexdigest(),'rights':'Pokemon audio copyright The Pokemon Company; repository CC0 does not license these audio bytes'}
            entry={'identity':cry['identity']+'.mode.0','status':'candidate','path':str(path.relative_to(output)),
                'sha256':hashlib.sha256(payload).hexdigest(),'sample_rate':48000,'channels':1,'frames':len(normal),
                'gain':1.0,'fade_seconds':0,'loop':None,'provenance':provenance}
            return dex,{'entry':entry,'national_dex':dex,'core_species':cry['core_species'],'name':cry['name'],
                'source_blob_sha':receipts[str(dex)]['sha'],'source_size':len(raw),'source_sha256':provenance['source_sha256'],
                'pcm_sha256':entry['sha256'],'frames':len(normal),'decoded_bytes':len(normal)*2,'duration_seconds':measured['duration_seconds'],
                'peak_dbfs':measured['peak_dbfs'],'rms_dbfs':measured['rms_dbfs'],'full_scale_samples':measured['full_scale_samples'],
                'gain_after_fixed_conversion_headroom':gain,'individual_listening_verified':False,'unreal_import_validated':False}
        normal={}
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            for dex,row in pool.map(one,cries):
                normal[dex]=row
                if len(normal)%50==0:print(f'{len(normal)}/386 normal cries prepared',flush=True)
    entries=assemble_entries(catalog,normal)
    manifest={'schema':'r15-local-audio-v1','profile':'modern','entries':entries}
    (output/'modern_manifest.json').write_text(dump(manifest))
    probe=validate(manifest,output);(output/'preimport_probe.json').write_text(dump(probe))
    (output/'source_receipts.json').write_text(dump(receipts))
    evidence={'schema':'r15-modern-pack-evidence-v1','source_repository':'PokeAPI/cries','source_commit':MODERN_PIN,'ffmpeg':ffmpeg,
        'selected_profile':'modern','pilot_style_approved':True,'individual_full_pack_listening_verified':False,'unreal_import_validated':False,
        'counts':{'species':386,'source_masters':386,'prepared_normal_cries':386,'normal_cry_missing':0,
                  'missing_special_cry_modes':4632,'missing_other_audio_semantics':609,'explicit_missing_semantics':5241,
                  'total_semantic_entries':len(entries),'unrepresented_semantics':probe['unrepresented_semantics'],
                  'full_scale_samples':sum(r['full_scale_samples'] for r in normal.values()),'verified_unreal_imports':0},
        'source_bytes':sum(r['source_size'] for r in normal.values()),'normal_pcm_bytes':sum(r['decoded_bytes'] for r in normal.values()),
        'total_duration_seconds':sum(r['duration_seconds'] for r in normal.values()),
        'processing':'same as selected pilot B: fixed -6.02dB conversion headroom; mono PCM16/48k; RMS target 0.06, peak ceiling -3dBFS and gain cap 4x; no trimming, pitch shift, denoising or AI',
        'budget_note':'aggregate PCM storage size, not measured Android resident memory; load/cache budget and streaming need device tests',
        'species':[{k:v for k,v in normal[dex].items() if k!='entry'} for dex in range(1,387)]}
    (output/'pack_evidence.json').write_text(dump(evidence))
    (output/'KULLANIM.txt').write_text('386 Pokémon için modern normal çığlıklar hazır.\nnormal/001.wav–386.wav: dengelenmiş PCM16, 48 kHz, mono dosyalar.\nmasters/: değiştirilmemiş arşiv kayıtları.\nModern ses tarzı kullanıcı tarafından seçildi. Bütün dosyalar kaynak kimliği ve teknik biçim açısından doğrulandı; tek tek dinleme veya Unreal içe aktarma onayı değildir.\nBayılma, kükreme ve diğer 12 özel biçim; müzik/efekt dosyaları bu pakette açıkça eksiktir.\nOyun çekirdeği tür kimlikleri National Dex sayılarıyla her zaman aynı değildir: manifestte doğru eşleşmeler bulunur.\n')
    archive=output.parent/'Pokemon_386_Modern_Ses_Paketi.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for file in sorted(output.rglob('*')):
            if file.is_file():z.write(file,file.relative_to(output))
    evidence['archive_sha256']=hashlib.sha256(archive.read_bytes()).hexdigest();evidence['archive_bytes']=archive.stat().st_size
    return evidence,archive

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--public-evidence',type=Path);a=p.parse_args()
    evidence,archive=build(a.source,a.output)
    if a.public_evidence:a.public_evidence.write_text(dump(evidence))
    print(dump({'archive':str(archive),'counts':evidence['counts'],'source_bytes':evidence['source_bytes'],'normal_pcm_bytes':evidence['normal_pcm_bytes'],'archive_bytes':evidence['archive_bytes'],'duration_seconds':evidence['total_duration_seconds']}),end='')
