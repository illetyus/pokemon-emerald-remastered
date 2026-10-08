#!/usr/bin/env python3
"""Transfer pinned public sound bytes into ignored/private storage using tree receipts.
The tree receipt must be collected separately from the pinned GitHub tree API.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import time
import urllib.request
from pathlib import Path
from build_r15_audio_catalog import ROOT, dump
from build_r15_audio_pilot import MODERN_PIN

def verify(data,receipt):
    if receipt.get('commit')!=MODERN_PIN or type(receipt.get('size')) is not int or not 0<receipt['size']<=1024*1024 or not re.fullmatch('[0-9a-f]{40}',str(receipt.get('sha'))): raise ValueError('invalid pinned receipt')
    if len(data)!=receipt['size'] or hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()!=receipt['sha']: raise ValueError('source byte mismatch')

def fetch(receipts,output,workers=8):
    output=Path(output).resolve()
    if output.is_relative_to(ROOT) and not output.is_relative_to(ROOT/'local'): raise ValueError('audio output must be private/outside git or ignored local/')
    if set(receipts)!={str(i) for i in range(1,387)}: raise ValueError('exact 386 source receipts required')
    output.mkdir(parents=True,exist_ok=True)
    def one(dex):
        receipt=receipts[str(dex)];path=output/f'{dex}.ogg'
        if path.exists():
            try:verify(path.read_bytes(),receipt);return {'dex':dex,'status':'verified_cached'}
            except ValueError:pass
        request=urllib.request.Request(f'https://raw.githubusercontent.com/PokeAPI/cries/{MODERN_PIN}/cries/pokemon/latest/{dex}.ogg',headers={'User-Agent':'EmeraldRemaster-R15-LocalAudioPreparation'})
        for attempt in range(3):
            try:
                with urllib.request.urlopen(request,timeout=30) as response:data=response.read(1024*1024+1)
                verify(data,receipt);tmp=output/f'.{dex}.ogg.tmp';tmp.write_bytes(data);os.replace(tmp,path)
                return {'dex':dex,'status':'verified_download'}
            except Exception:
                if attempt==2:raise
                time.sleep(.25*(attempt+1))
    rows=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1,min(8,workers))) as pool:
        futures={pool.submit(one,dex):dex for dex in range(1,387)}
        for future in concurrent.futures.as_completed(futures):
            rows.append(future.result())
            if len(rows)%50==0:print(f'{len(rows)}/386 pinned audio files verified',flush=True)
    (output/'receipts.json').write_text(dump(receipts))
    return {'count':len(rows),'cached':sum(r['status']=='verified_cached' for r in rows),'source_bytes':sum(r['size'] for r in receipts.values())}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--receipts',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--workers',type=int,default=8);a=p.parse_args();print(dump(fetch(json.loads(a.receipts.read_text()),a.output,a.workers)),end='')
