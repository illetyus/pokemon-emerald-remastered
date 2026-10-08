#!/usr/bin/env python3
"""Offline A/B pilot. Copies commercial bytes only to an explicit private/local output.
Needs ffmpeg on PATH. No AI, trimming, pitch shifting or perceptual approval.
"""
import argparse
import array
import base64
import hashlib
import html
import io
import json
import math
import shutil
import subprocess
import sys
import wave
import zipfile
from pathlib import Path
from build_r15_audio_catalog import ROOT, VENDOR, PILOT, PIN, dump
from validate_r15_audio_pack import validate

MODERN_PIN='ef687b18f0ce17169b4b4c09175819f7ade92f0f'

def digest(b): return hashlib.sha256(b).hexdigest()

def pcm(path):
    with wave.open(str(path),'rb') as w:
        if (w.getnchannels(),w.getsampwidth(),w.getframerate())!=(1,2,48000): raise ValueError('pilot requires mono PCM16 48k')
        data=w.readframes(w.getnframes())
    a=array.array('h',data)
    if sys.byteorder!='little': a.byteswap()
    return a

def wav_bytes(a):
    raw=array.array('h',a)
    if sys.byteorder!='little': raw.byteswap()
    buf=io.BytesIO()
    with wave.open(buf,'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes(raw.tobytes())
    return buf.getvalue()

def stats(a):
    if not a: raise ValueError('empty audio')
    peak=max(abs(v) for v in a)/32768
    rms=math.sqrt(sum((v/32768)**2 for v in a)/len(a))
    return {'frames':len(a),'duration_seconds':len(a)/48000,'peak_dbfs':20*math.log10(max(peak,1e-12)),
            'rms_dbfs':20*math.log10(max(rms,1e-12)),'full_scale_samples':sum(abs(v)>=32767 for v in a)}

def audition(a):
    rms=math.sqrt(sum((v/32768)**2 for v in a)/len(a));peak=max(abs(v) for v in a)/32768
    if rms<1e-8: raise ValueError('silent candidate')
    gain=min(4.0,0.06/rms,10**(-3/20)/peak)
    return array.array('h',(round(v*gain) for v in a)),gain

def decode(source,dest):
    subprocess.run(['ffmpeg','-nostdin','-hide_banner','-loglevel','error','-y','-i',str(source),
        '-map_metadata','-1','-af','volume=0.5,aresample=resampler=swr:filter_size=32:phase_shift=10:exact_rational=1:dither_method=none',
        '-ar','48000','-ac','1','-c:a','pcm_s16le','-fflags','+bitexact','-flags:a','+bitexact',str(dest)],check=True)

def build(modern,output):
    modern=Path(modern).resolve();output=Path(output).resolve()
    if output.is_relative_to(ROOT) and not output.is_relative_to(ROOT/'local'): raise ValueError('commercial pilot output must be outside git or inside ignored local/')
    if output.exists() and any(output.iterdir()): raise ValueError('use a new empty output directory')
    output.mkdir(parents=True,exist_ok=True)
    for n in ['masters','decoded','comparison']: (output/n).mkdir()
    source=json.loads((ROOT/'data/r15/source_audio_catalog.json').read_text())
    rows={s['national_dex']:s for s in source['cries']}
    combined=array.array('h',[0]*48000);timeline=[];cards=[];report=[]
    manifests={p:{'schema':'r15-local-audio-v1','profile':p,'entries':[]} for p in ['original','modern']}
    ffmpeg=subprocess.run(['ffmpeg','-version'],capture_output=True,text=True,check=True).stdout.splitlines()[0]
    for dex in PILOT:
        row=rows[dex]; pair={'national_dex':dex,'name':row['name'],'identity':row['identity'],'quality_approved':False,'tracks':{}}
        for profile in ['original','modern']:
            original=profile=='original';src=VENDOR/row['source_pcm']['path'] if original else modern/f'{dex}.ogg'
            raw=src.read_bytes();source_sha=digest(raw)
            if original and source_sha!=row['source_pcm']['sha256']: raise ValueError('pinned Emerald cry mismatch')
            # Modern input requires a Git blob receipt from the pinned Contents API.
            if not original:
                receipts=json.loads((modern/'receipts.json').read_text());receipt=receipts[str(dex)]
                blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()
                if receipt['commit']!=MODERN_PIN or receipt['sha']!=blob or receipt['size']!=len(raw): raise ValueError('modern source receipt mismatch')
            stem=f'{dex:03d}_{row["name"].replace(" ","_")}_{profile}'
            shutil.copyfile(src,output/'masters'/(stem+src.suffix))
            target=output/'decoded'/(stem+'.wav');decode(src,target);a=pcm(target);normalized,gain=audition(a)
            audition_path=output/'comparison'/(stem+'.wav');audition_path.write_bytes(wav_bytes(normalized))
            provenance={'repository':'pret/pokeemerald via pinned Vanilla+' if original else 'PokeAPI/cries',
                'commit':PIN if original else MODERN_PIN,'source_path':row['source_pcm']['path'] if original else f'cries/pokemon/latest/{dex}.ogg',
                'source_sha256':source_sha,'rights':'Pokemon audio copyright The Pokemon Company; private evaluation only; not redistributable by repository CC0'}
            entry={'identity':row['identity']+'.mode.0','status':'candidate','path':str(target.relative_to(output)),
                'sha256':digest(target.read_bytes()),'sample_rate':48000,'channels':1,'frames':len(a),
                'gain':1.0,'fade_seconds':0,'loop':None,'provenance':provenance}
            manifests[profile]['entries'].append(entry)
            start=len(combined)/48000;combined.extend(normalized);end=len(combined)/48000
            timeline.append({'pokemon':row['name'],'dex':dex,'profile':profile,'start_seconds':round(start,3),'end_seconds':round(end,3)})
            combined.extend([0]*(31200 if original else 60000))
            pair['tracks'][profile]={'source':provenance,'decoded':stats(a),'comparison':stats(normalized),'comparison_gain':gain,
                'path':str(audition_path.relative_to(output)),'sha256':digest(audition_path.read_bytes())}
            data=base64.b64encode(audition_path.read_bytes()).decode()
            label='A — Emerald kaynak kaydı' if original else 'B — Modern ses adayı'
            cards.append(f'<div class="track"><strong>{label}</strong><audio controls preload="none" src="data:audio/wav;base64,{data}"></audio></div>')
        report.append(pair)
        cards.insert(len(cards)-2,f'<h2>{dex:03d} · {html.escape(row["name"])}</h2>')
    report_data={'schema':'r15-pilot-evidence-v1','source_pin':PIN,'modern_pin':MODERN_PIN,'ffmpeg':ffmpeg,'species':report,'timeline':timeline,
        'processing':'mono PCM16 48k resampling with fixed -6.02 dB conversion headroom; preserved masters; listening copy RMS target -24.44 dBFS, peak ceiling -3 dBFS, gain cap 4x; no trimming, denoising, pitch shift or AI',
        'original_label':'Emerald source PCM, not certified hardware engine/mixer output','quality_approved':False,'unreal_import_validated':False}
    (output/'pilot_evidence.json').write_text(dump(report_data))
    for profile,manifest in manifests.items():
        (output/(profile+'_manifest.json')).write_text(dump(manifest))
        (output/(profile+'_probe.json')).write_text(dump(validate(manifest,output)))
    wav=output/'Pokemon_12_AB.wav';wav.write_bytes(wav_bytes(combined))
    text='''<!doctype html><html lang="tr"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Pokémon ses karşılaştırması</title>
<style>body{font:17px system-ui;background:#101826;color:#f4f7fb;max-width:760px;margin:auto;padding:22px;line-height:1.6}h1{font-size:27px}h2{margin-top:35px;border-top:1px solid #3d4b61;padding-top:18px}.track{background:#1d2b40;padding:15px;margin:10px 0;border-radius:12px}audio{display:block;width:100%;margin-top:10px}small{color:#bcc8dc}</style>
<h1>12 Pokémon · Ses karşılaştırması</h1><p>Her Pokémon için önce A, sonra B. A: Emerald kaynak kaydı. B: modern arşivden alınan aday. İki dinleme kopyasında ses yüksekliği karşılaştırmayı kolaylaştırmak için dengelendi.</p>
<p>Kimlik, ton, ilk vuruş, pürüz ve rahatsız edici yeni sesler açısından karşılaştırın. Ses seviyesini sabit tutun. Bu paket yapay zekâ işlemi içermiyor.</p><small>A kayıtları kaynak PCM sesleridir; Emerald'ın bütün ses motoru ve donanım çıktısı olarak doğrulanmadı. Bu dosya çevrimdışı çalışır.</small>'''+''.join(cards)+'''<h2>Karar</h2><p>B genel olarak daha iyi mi? Özellikle Pikachu, Gengar, Rayquaza ve Deoxys için A/B tercihini kaydedin. Tanınan sesi bozan adayları istisna olarak ele alacağız.</p></html>'''
    (output/'Pokemon_ses_karsilastirmasi.html').write_text(text)
    listing=['Her Pokémon için A: Emerald kaynak PCM, B: modern aday.','A ile B arasında 0,65 saniye; Pokémon çiftleri arasında 1,25 saniye sessizlik.','Kaynak kayıtlar donanım motor çıktısı olarak doğrulanmadı. AI uygulanmadı.','']
    listing += [f'{t["start_seconds"]:06.2f}–{t["end_seconds"]:06.2f} sn | {t["dex"]:03d} {t["pokemon"]} | '+('A' if t['profile']=='original' else 'B') for t in timeline]
    listing += ['','HTML dosyası çevrimdışı A/B oynatıcıdır. masters: değiştirilmemiş kaynaklar; decoded: sabit -6.02 dB dönüşüm payı olan WAV; comparison: dinleme kopyaları.','12 modern dosya adayı; 0 kalite onayı; 0 Unreal içe aktarma doğrulaması.']
    (output/'DINLEME_KILAVUZU.txt').write_text('\n'.join(listing)+'\n')
    archive=output.parent/'Pokemon_12_Ses_Paketi.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(output.rglob('*')):
            if p.is_file(): z.write(p,p.relative_to(output))
    return {'species':len(report),'source_files':24,'decoded_files':24,'comparison_files':24,'duration_seconds':len(combined)/48000,
            'quality_approved':False,'unreal_import_validated':False,'audio':str(wav),'zip':str(archive),'guide':str(output/'DINLEME_KILAVUZU.txt')}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--modern',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();print(dump(build(a.modern,a.output)),end='')
