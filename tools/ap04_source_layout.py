#!/usr/bin/env python3
"""Layout-only production AGBCC oracle. Never builds a ROM or runs gameplay."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

AGBCC_PIN='da598c1d918402c42c0c0d7128ba14567f3175e9'
SOURCE_PIN='70db90c9077aed1272e746fc2537d9f12b95a91c'
SOURCE_TREE='5a551f1f9e40184278c57dfb8d25f68a0a1c99dc'
ANCHORS={'sb1.bytes':0x3DC8,'sb2.bytes':0xF44,'storage.bytes':0x83D0,
    'object.bytes':0x28,'object_template.bytes':0x18,'sb1.flags':0x12B0,
    'sb1.vars':0x13DC,'sb1.unused_3598':0x35D8,'sb2.encryptionKey':0xAC,
    'sb2.follower':0xF2C,'storage.boxes':4,'sb1.roamer':0x321C,
    'sb1.playerPartyCount':0x234,'sb1.objectEventTemplates':0xCB0}
EXPRESSIONS={'sb1.bytes':'sizeof(struct SaveBlock1)','sb2.bytes':'sizeof(struct SaveBlock2)',
    'storage.bytes':'sizeof(struct PokemonStorage)','object.bytes':'sizeof(struct ObjectEvent)',
    'object_template.bytes':'sizeof(struct ObjectEventTemplate)'}
for prefix,kind,names in [
    ('sb1','struct SaveBlock1','pos location continueGameWarp dynamicWarp lastHealLocation escapeWarp savedMusic weather mapLayoutId playerPartyCount playerParty objectEventTemplates pcItems flags vars berryTrees secretBases gabbyAndTyData easyChatProfile easyChatBattleStart easyChatBattleWon easyChatBattleLost mail oldMan dewfordTrends contestWinners mysteryGift unused_3598 trainerHillTimes roamer lilycoveLady registeredTexts trainerHill waldaPhrase'),
    ('sb2','struct SaveBlock2','playerName playerGender playerTrainerId localTimeOffset lastBerryTreeUpdate encryptionKey playerApprentice apprentices hallRecords1P hallRecords2P frontier follower'),
    ('storage','struct PokemonStorage','currentBox boxes boxNames boxWallpapers'),
    ('mail','struct Mail','words playerName trainerId species itemId'),
    ('secret_base','struct SecretBase','trainerName'),
    ('contest_winner','struct ContestWinner','monName trainerName'),
    ('apprentice','struct Apprentice','speechWon playerName language'),
    ('ranking_1p','struct RankingHall1P','name'),
    ('ranking_2p','struct RankingHall2P','name1 name2'),
    ('frontier','struct BattleFrontier','opponentNames'),
    ('sb1','struct SaveBlock1','oldMan.bard.songLyrics oldMan.bard.language oldMan.giddy.language oldMan.hipster.language oldMan.trader.playerNames oldMan.trader.decorations oldMan.trader.language oldMan.storyteller.trainerNames mysteryGift.questionnaireWords')]:
    for field in names.split():EXPRESSIONS[prefix+'.'+field]='offsetof('+kind+', '+field+')'
for name,kind in [('old_man','OldMan'),('lady','LilycoveLady'),('contest_winner','struct ContestWinner'),
    ('mail','struct Mail'),('secret_base','struct SecretBase'),('trend','struct DewfordTrend'),
    ('apprentice','struct Apprentice'),('ranking_1p','struct RankingHall1P'),
    ('ranking_2p','struct RankingHall2P'),('player_apprentice','struct PlayersApprentice')]:
    EXPRESSIONS[name+'.bytes']='sizeof('+kind+')'

def parse_words(text: str, count: int) -> list[int]:
    starts=list(re.finditer(r'(?m)^ap04_layout:\s*$',text))
    if len(starts)!=1:raise ValueError('one named ap04_layout array required')
    body=text[starts[0].end():]
    stop=re.search(r'(?m)^\s*\.size\s+ap04_layout\b|^[A-Za-z_][\w.]*:\s*$',body)
    if stop:body=body[:stop.start()]
    values=[]
    for line in body.splitlines():
        if re.match(r'\s*\.word\b',line):
            m=re.fullmatch(r'\s*\.word\s+(0[xX][0-9a-fA-F]+|\d+)\s*(?:@.*)?',line)
            if not m:raise ValueError('nonconstant layout expression')
            values.append(int(m.group(1),0))
    if len(values)!=count:raise ValueError('truncated or enlarged layout array')
    return values

def validate_anchors(values: dict[str,int]) -> None:
    for key,expected in ANCHORS.items():
        if values.get(key)!=expected:raise ValueError('production AGBCC anchor mismatch: '+key)

def sha(path: Path) -> str:
    with path.open('rb') as stream:
        digest=hashlib.sha256()
        for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
    return digest.hexdigest()

def measure(root: Path, agbcc: Path, output: Path) -> dict:
    root=root.resolve();agbcc=agbcc.resolve();output=output.resolve()
    compiler_head=subprocess.check_output(['git','-C',str(agbcc),'rev-parse','HEAD'],text=True).strip()
    source_tree=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD:vendor/vanillaplus'],text=True).strip()
    if compiler_head!=AGBCC_PIN or source_tree!=SOURCE_TREE:raise ValueError('compiler/vendor pin drift')
    source=root/'vendor/vanillaplus';output.mkdir(parents=True,exist_ok=True)
    shim=output/'shim';shim.mkdir(exist_ok=True)
    shims={
        'stdint.h':'typedef unsigned char uint8_t; typedef signed char int8_t;\ntypedef unsigned short uint16_t; typedef signed short int16_t;\ntypedef unsigned int uint32_t; typedef signed int int32_t;\ntypedef unsigned long long uint64_t; typedef signed long long int64_t;\n',
        'string.h':'#include <stddef.h>\nvoid *memset(void *, int, size_t); void *memcpy(void *, const void *, size_t); int memcmp(const void *, const void *, size_t);\n',
        'limits.h':'#define CHAR_BIT 8\n#define INT_MAX 2147483647\n#define UINT_MAX 4294967295U\n',
        'map_groups.h':'/* Layout-only: no map IDs participate in save structs. */\n'}
    for name,text in shims.items():(shim/name).write_text(text,encoding='utf-8')
    probe=output/'probe.c'
    probe.write_text('#include "global.h"\n#include "pokemon_storage_system.h"\n#include <stddef.h>\nconst unsigned ap04_layout[] = {\n'+',\n'.join(EXPRESSIONS.values())+'\n};\n',encoding='utf-8')
    preprocessed=output/'probe.i';assembly=output/'probe.s'
    commands=[['gcc','-E','-P','-nostdinc','-DMODERN=0','-DVERSION=2',
        '-I'+str(shim),'-I'+str(agbcc/'ginclude'),'-I'+str(source/'include'),
        '-I'+str(source/'gflib'),str(probe)],
        [str(agbcc/'gcc/agbcc'),str(preprocessed),'-mthumb-interwork','-O2','-o',str(assembly)]]
    with preprocessed.open('wb') as stdout, (output/'preprocess.log').open('wb') as stderr:
        subprocess.run(commands[0],stdout=stdout,stderr=stderr,check=True,timeout=60)
    with (output/'compiler.log').open('wb') as log:
        subprocess.run(commands[1],stdout=log,stderr=log,check=True,timeout=60)
    text=assembly.read_text(encoding='utf-8')
    if 'gcc2.9-arm' not in text.replace(' ',''):raise ValueError('actual AGBCC assembly identity absent')
    values=dict(zip(EXPRESSIONS,parse_words(text,len(EXPRESSIONS))))
    validate_anchors(values)
    touched=subprocess.check_output(['git','-C',str(root),'ls-files','vendor/vanillaplus/include','vendor/vanillaplus/gflib'],text=True).splitlines()
    report={'Schema':'remaster.ap04.agbcc-layout','Version':1,'SourceCommit':SOURCE_PIN,
        'VendorTree':source_tree,'CheckoutHead':subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip(),
        'CompilerPin':compiler_head,'CompilerBinarySHA256':sha(agbcc/'gcc/agbcc'),
        'MODERN':0,'AssemblySHA256':sha(assembly),'ProbeSHA256':sha(probe),
        'Commands':commands,'SourceHeaderHashes':{p:sha(root/p) for p in touched},
        'Fields':values,'KnownProductionAnchorsVerified':True,
        'ROMBuilt':False,'NewGameInitializationVerified':False,'UnrealRuntimeVerified':False}
    (output/'layout.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report

def main() -> int:
    parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--agbcc',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();report=measure(args.source,args.agbcc,args.output)
    print('AP04 production AGBCC layout: '+str(len(report['Fields']))+' fields, all known anchors verified; no ROM/gameplay/runtime claim')
    return 0
if __name__=='__main__':raise SystemExit(main())
