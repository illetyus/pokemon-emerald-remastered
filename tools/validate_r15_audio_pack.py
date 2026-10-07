#!/usr/bin/env python3
"""Strict PCM/metadata probe. A passing local probe is not an Unreal import."""
import argparse
import hashlib
import json
import math
import re
import wave
from pathlib import Path

def strict(text):
    def pairs(items):
        result={}
        for k,v in items:
            if k in result: raise ValueError('duplicate JSON key: '+k)
            result[k]=v
        return result
    return json.loads(text, object_pairs_hook=pairs,
                      parse_constant=lambda x: (_ for _ in ()).throw(ValueError('nonfinite JSON')))

def validate(manifest, root):
    root=Path(root).resolve()
    if manifest.get('schema')!='r15-local-audio-v1': raise ValueError('unsupported schema')
    if manifest.get('profile') not in ['original','modern']: raise ValueError('invalid profile')
    entries=manifest.get('entries')
    if not isinstance(entries,list) or len(entries)>10000: raise ValueError('invalid entry count')
    source=json.loads((Path(__file__).resolve().parents[1]/'data/r15/source_audio_catalog.json').read_text())
    required={s['identity'] for s in source['songs'] if s['source_id']!=0}
    required.update(c['identity']+f'.mode.{m}' for c in source['cries'] for m in range(13))
    seen=set();result=[]
    for e in entries:
        key=e.get('identity')
        if not isinstance(key,str) or key in seen or key not in required: raise ValueError('duplicate/invalid identity')
        seen.add(key)
        if e.get('status')=='missing':
            if not e.get('reason'): raise ValueError('missing entry needs reason')
            result.append({'identity':key,'status':'missing','reason':e['reason']});continue
        if e.get('status')!='candidate': raise ValueError('unknown status; approval is external to probe')
        relative=e.get('path')
        if not isinstance(relative,str) or not relative or Path(relative).is_absolute(): raise ValueError('relative local path required')
        path=(root/relative).resolve()
        if not path.is_relative_to(root): raise ValueError('audio path escapes local root')
        if path.stat().st_size>64*1024*1024: raise ValueError('per-file preimport budget exceeded')
        payload=path.read_bytes()
        if hashlib.sha256(payload).hexdigest()!=e.get('sha256'): raise ValueError('audio hash mismatch')
        try:
            with wave.open(str(path),'rb') as w:
                rate=w.getframerate();channels=w.getnchannels();frames=w.getnframes()
                if w.getsampwidth()!=2 or w.getcomptype()!='NONE': raise ValueError('PCM16 WAV required')
                if rate!=48000 or channels not in (1,2) or frames<=0: raise ValueError('invalid PCM shape')
                if len(w.readframes(frames))!=frames*channels*2: raise ValueError('truncated WAV data')
        except wave.Error as ex: raise ValueError('invalid WAV') from ex
        if any(type(e.get(k)) is not int for k in ['sample_rate','frames','channels']) or e.get('sample_rate')!=rate or e.get('frames')!=frames or e.get('channels')!=channels: raise ValueError('PCM metadata mismatch')
        for field in ['gain','fade_seconds']:
            value=e.get(field,1 if field=='gain' else 0)
            if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not 0<=value<= (4 if field=='gain' else 10): raise ValueError('invalid '+field)
        loop=e.get('loop')
        if loop is not None:
            start,end=loop.get('start_frame'),loop.get('end_frame')
            if any(type(v) is not int for v in (start,end)) or not 0<=start<end<=frames: raise ValueError('invalid loop frames')
        provenance=e.get('provenance',{})
        if not all(isinstance(provenance.get(k),str) and provenance[k] for k in ['repository','commit','source_path','source_sha256','rights']): raise ValueError('incomplete provenance')
        if not re.fullmatch('[0-9a-f]{40}',provenance['commit']) or not re.fullmatch('[0-9a-f]{64}',provenance['source_sha256']): raise ValueError('invalid provenance hashes')
        result.append({'identity':key,'status':'local_pcm_probe_pass','frames':frames,
                       'decoded_bytes':frames*channels*2,'unreal_import_validated':False})
    return {'schema':'r15-local-probe-v1','profile':manifest['profile'],'entries':result,
            'required_semantics':len(required),'unrepresented_semantics':len(required-seen),
            'quality_approved':False,'unreal_import_validated':False}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('manifest',type=Path);p.add_argument('--root',type=Path,required=True);a=p.parse_args()
    print(json.dumps(validate(strict(a.manifest.read_text()),a.root),sort_keys=True,indent=2))
