#!/usr/bin/env python3
"""Build a source-bound PREPARATION catalog and original-C semantic oracle.

This is not NewGameInitData execution or a complete/usable save initializer.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

SOURCE_PIN='70db90c9077aed1272e746fc2537d9f12b95a91c'
SOURCE_TREE='5a551f1f9e40184278c57dfb8d25f68a0a1c99dc'
INPUTS=('src/new_game.c','src/random.c','src/main.c','src/naming_screen.c',
    'include/random.h','include/constants/global.h','include/constants/layouts.h',
    'charmap.txt','data/maps/map_groups.json','data/layouts/layouts.json',
    'data/maps/InsideOfTruck/map.json')

def source_files(root: Path) -> dict[str,bytes]:
    head=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip()
    tree=subprocess.check_output(['git','-C',str(root),'rev-parse',head+':vendor/vanillaplus'],text=True).strip()
    if tree!=SOURCE_TREE: raise ValueError('production vendor tree drift')
    return {name:subprocess.check_output(['git','-C',str(root),'cat-file','blob',
        tree+':'+name]) for name in INPUTS}

def function(text: str, name: str) -> str:
    # Mask comments and literals while preserving positions and true braces.
    mask=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda m:''.join('\n' if c=='\n' else ' ' for c in m.group()),text,flags=re.S)
    pattern=r'(?m)^[^\n;{}]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{'
    matches=list(re.finditer(pattern,mask))
    if len(matches)!=1: raise ValueError('one definition required: '+name)
    found=matches[0];begin=mask.index('{',found.start());end=begin+1;depth=1
    while depth:
        if end>=len(mask): raise ValueError('unterminated function: '+name)
        depth+=(mask[end]=='{')-(mask[end]=='}');end+=1
    return text[found.start():end].strip()

def derive(files: dict[str,bytes]) -> tuple[str,str,dict]:
    texts={name:data.decode('utf-8') for name,data in files.items()}
    keyboard=re.search(r'sKeyboardChars\[[^;]*?=\s*\{(.*?)\n\};',texts['src/naming_screen.c'],re.S)
    if not keyboard: raise ValueError('source keyboard missing')
    rows=re.findall(r'__\("([^"\\]*)"\)',keyboard.group(1))
    if len(rows)!=12 or any(len(row)!=8 for row in rows): raise ValueError('source keyboard shape changed')
    cmap={}
    for char,number in re.findall(r"^'([^']+)'\s*=\s*([0-9A-Fa-f]{2})\s*$",texts['charmap.txt'],re.M):
        if len(char)==1:
            if char in cmap: raise ValueError('duplicate charmap glyph')
            cmap[char]=int(number,16)
    keys=sorted(set(''.join(rows)))
    if any(char not in cmap or cmap[char]>=0xF7 for char in keys): raise ValueError('unencodable/control keyboard glyph')
    if len({cmap[c] for c in keys})!=len(keys): raise ValueError('duplicate keyboard byte')
    groups=json.loads(texts['data/maps/map_groups.json'])
    destinations=[(i,j) for i,name in enumerate(groups['group_order']) for j,value in enumerate(groups[name])
        if value=='InsideOfTruck']
    if len(destinations)!=1: raise ValueError('truck destination ambiguity')
    layouts=json.loads(texts['data/layouts/layouts.json'])['layouts']
    truck=json.loads(texts['data/maps/InsideOfTruck/map.json'])
    entries=[(i+1,l) for i,l in enumerate(layouts) if l['id']==truck['layout']]
    if len(entries)!=1: raise ValueError('truck layout ambiguity')
    layout_id,layout=entries[0]
    constants=dict(re.findall(r'^#define\s+(\w+)\s+(0x[\da-fA-F]+|\d+)\s*$',texts['include/constants/layouts.h'],re.M))
    if int(constants[truck['layout']],0)!=layout_id: raise ValueError('layout order/constant mismatch')
    definitions=dict(re.findall(r'^#define\s+(OPTIONS_\w+)\s+(\d+)\b',texts['include/constants/global.h'],re.M))
    original=function(texts['src/new_game.c'],'SetDefaultOptions')
    assignments=dict(re.findall(r'gSaveBlock2Ptr->(\w+)\s*=\s*(\w+)\s*;',original))
    expected={'optionsTextSpeed','optionsWindowFrameType','optionsSound','optionsBattleStyle','optionsBattleSceneOff','regionMapZoom'}
    if set(assignments)!=expected: raise ValueError('default-option fields changed')
    values={k:(0 if v=='FALSE' else int(v) if v.isdigit() else int(definitions[v])) for k,v in assignments.items()}
    shifts={'optionsTextSpeed':0,'optionsWindowFrameType':3,'optionsSound':8,
        'optionsBattleStyle':9,'optionsBattleSceneOff':10,'regionMapZoom':11}
    word=sum(values[k]<<shifts[k] for k in values)
    init=function(texts['src/new_game.c'],'NewGameInitData')
    calls=[name for name in re.findall(r'\b(\w+)\s*\(',init[init.index('{')+1:])
        if name not in ('if','sizeof')]
    pending=list(calls);pending.remove('InitPlayerTrainerId')
    group,map_num=destinations[0]
    catalog='/* Derived from immutable Vanilla+ '+SOURCE_PIN+'; preparation only. */\n'
    for key,value in [('DEFAULT_OPTIONS',word),('PENDING_CALLS',len(pending)),('TRUCK_GROUP',group),
        ('TRUCK_MAP',map_num),('TRUCK_LAYOUT',layout_id),('TRUCK_X',layout['width']//2),('TRUCK_Y',layout['height']//2)]:
        catalog+='#define NEW_GAME_'+key+' '+str(value)+'\n'
    catalog+='static const NewGameKey kNewGameKeys[] = {\n'
    for char in keys:
        raw=char.encode('utf-8');escaped=''.join('\\x'+format(b,'02X') for b in raw)
        catalog+='    {"'+escaped+'", '+str(len(raw))+', '+str(cmap[char])+'},\n'
    catalog+='};\n'
    bodies=[original,function(texts['src/new_game.c'],'SetTrainerId'),
        function(texts['src/new_game.c'],'InitPlayerTrainerId'),function(texts['src/random.c'],'Random'),
        function(texts['src/main.c'],'GetGeneratedTrainerIdLower')]
    rng_macro=re.search(r'^#define ISO_RANDOMIZE1[^\n]+',texts['include/random.h'],re.M).group()
    oracle='''/* Original pinned source FUNCTIONS; semantic stub is NOT a source ABI oracle. */
#include <stdint.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
struct SemanticSaveBlock2 { u8 playerTrainerId[4]; unsigned optionsTextSpeed,
    optionsWindowFrameType, optionsSound, optionsBattleStyle, optionsBattleSceneOff, regionMapZoom; };
static struct SemanticSaveBlock2 source_save;
static struct SemanticSaveBlock2 *gSaveBlock2Ptr=&source_save;
static u32 gRngValue, sRandCount;
static u16 sTrainerId;
#define FALSE 0
u16 Random(void); u16 GetGeneratedTrainerIdLower(void);
'''+rng_macro+'\n'
    for name,value in sorted(definitions.items()): oracle+='#define '+name+' '+value+'\n'
    oracle+='\n\n'.join(bodies)+'\n'
    oracle+='''void ap04_source_bootstrap(uint32_t rng, uint16_t lower,
    const uint8_t name[8], uint8_t gender, uint8_t prefix[176], uint32_t *after) {
    unsigned i; uint16_t options;
    memset(&source_save,0,sizeof source_save); memset(prefix,0,176);
    gRngValue=rng; sTrainerId=lower; sRandCount=0;
    SetDefaultOptions(); InitPlayerTrainerId();
    for(i=0;i<8;++i) { prefix[i]=name[i]; if(name[i]==255) break; }
    prefix[8]=gender; memcpy(prefix+10,source_save.playerTrainerId,4);
    options=(uint16_t)(source_save.optionsTextSpeed | (source_save.optionsWindowFrameType<<3)
        | (source_save.optionsSound<<8) | (source_save.optionsBattleStyle<<9)
        | (source_save.optionsBattleSceneOff<<10) | (source_save.regionMapZoom<<11));
    prefix[20]=(uint8_t)options; prefix[21]=(uint8_t)(options>>8); *after=gRngValue;
}
'''
    contract={'Schema':'remaster.ap04.new-game-preparation','Version':1,'SourceCommit':SOURCE_PIN,
        'VendorTree':SOURCE_TREE,'SourceHashes':{n:hashlib.sha256(b).hexdigest() for n,b in files.items()},
        'OriginalFunctionHashes':{name:hashlib.sha256(body.encode()).hexdigest() for name,body in zip(
            ['SetDefaultOptions','SetTrainerId','InitPlayerTrainerId','Random','GetGeneratedTrainerIdLower'],bodies)},
        'KeyboardGlyphs':len(keys),'DefaultOptionWord':word,
        'Truck':{'Group':group,'Map':map_num,'Layout':layout_id,'X':layout['width']//2,'Y':layout['height']//2,
            'WarpId':-1,'WarpX':-1,'WarpY':-1},
        'NewGameInitCalls':calls,'PendingSourceCalls':pending,
        'RTCInitialized':False,'NewGameInitDataExecuted':False,'UsableSaveInitialized':False,
        'SourceABIVerifiedBySemanticStub':False,'UnrealRuntimeVerified':False}
    return catalog,oracle,contract

def main() -> int:
    parser=argparse.ArgumentParser()
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--catalog',type=Path,required=True)
    parser.add_argument('--oracle',type=Path,required=True)
    parser.add_argument('--contract',type=Path,required=True)
    args=parser.parse_args()
    catalog,oracle,contract=derive(source_files(args.source.resolve()))
    for path,data in [(args.catalog,catalog),(args.oracle,oracle),
        (args.contract,json.dumps(contract,indent=2)+'\n')]:
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data.encode('utf-8'))
    print('AP04 preparation catalog/original-C oracle generated; full NewGame/RTC/runtime remain unverified')
    return 0
if __name__=='__main__':raise SystemExit(main())
