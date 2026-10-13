#!/usr/bin/env python3
"""Layout-only production AGBCC oracle. Never builds a ROM or runs gameplay."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
from pathlib import PurePosixPath
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


# Additional persistent fields needed by canonical NewGameInitData.
for prefix,kind,names in [
    ('sb1','struct SaveBlock1','money coins unlockedTrendySayings linkBattleRecords'),
    ('sb2','struct SaveBlock2','optionsButtonMode'),
    ('gabby','struct GabbyAndTyData','quote'),
    ('walda','struct WaldaPhrase','colors text'),
    ('trend','struct DewfordTrend','rand words'),
    ('apprentice','struct Apprentice','numQuestions number'),
    ('quiz','struct LilycoveLadyQuiz','question correctAnswer playerAnswer playerName prize questionId prevQuestionId language'),
    ('favor','struct LilycoveLadyFavor','playerName favorId bestItem language'),
    ('contest_lady','struct LilycoveLadyContest','playerName category language'),
    ('berry_tree','struct BerryTree','berry minutesUntilNextStage berryYield'),
    ('link_record','struct LinkBattleRecord','name'),
    ('sb1','struct SaveBlock1','waldaPhrase.text trainerHillTimes')]:
    for field in names.split():EXPRESSIONS[prefix+'.'+field]='offsetof('+kind+', '+field+')'
for name,kind in [('berry_tree','struct BerryTree'),('link_record','struct LinkBattleRecord')]:
    EXPRESSIONS[name+'.bytes']='sizeof('+kind+')'

BIT_PROBES={
    'ap04_option_bits':('struct SaveBlock2','.optionsTextSpeed = 1','sb2.bytes'),
    'ap04_apprentice_bits':('struct Apprentice','.id = NUM_APPRENTICES','apprentice.bytes'),
    'ap04_berry_bits':('struct BerryTree','.stage = BERRY_STAGE_BERRIES, .stopGrowth = 1','berry_tree.bytes'),
    'ap04_trend_current':('struct DewfordTrend','.trendiness = 127','trend.bytes'),
    'ap04_trend_max':('struct DewfordTrend','.maxTrendiness = 127','trend.bytes'),
    'ap04_trend_gaining':('struct DewfordTrend','.gainingTrendiness = 1','trend.bytes')}

def parse_data(text: str, label: str, size: int) -> bytes:
    if size<1 or size>65536: raise ValueError('bounded data size required')
    starts=list(re.finditer(r'(?m)^'+re.escape(label)+r':\s*$',text))
    if len(starts)!=1: raise ValueError('one named data block required: '+label)
    body=text[starts[0].end():]
    stop=re.search(r'(?m)^\s*\.size\s+'+re.escape(label)+r'\b|^\s*\.(?:globl|global|type)\b|^[A-Za-z_][\w.]*:\s*$',body)
    if stop:body=body[:stop.start()]
    data=bytearray()
    for line in body.splitlines():
        line=line.split('@')[0].strip()
        if not line:continue
        m=re.fullmatch(r'\.(byte|short|hword|word|space|skip|zero)\s+(.+)',line)
        if not m:raise ValueError('nonconstant named data directive: '+line)
        directive,raw=m.groups()
        parts=[p.strip() for p in raw.split(',')]
        if any(not re.fullmatch(r'-?(?:0[xX][0-9a-fA-F]+|\d+)',p) for p in parts):
            raise ValueError('nonconstant data value')
        values=[int(p,0) for p in parts]
        if directive in ('space','skip','zero'):
            if len(values)!=1 or values[0]<0 or values[0]>size-len(data):raise ValueError('oversized fill')
            data.extend(bytes(values[0]))
        else:
            width={'byte':1,'short':2,'hword':2,'word':4}[directive]
            for value in values:
                if len(data)+width>size:raise ValueError('oversized data')
                data.extend((value&((1<<(width*8))-1)).to_bytes(width,'little'))
    if len(data)!=size:raise ValueError('truncated data')
    return bytes(data)

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

def validate_certificate(report: dict, certificate: dict) -> None:
    for key in ['SourceCommit','VendorTree','CompilerPin','Fields','BitfieldDefaultBytes']:
        if report.get(key) != certificate.get(key):
            raise ValueError('initialization layout certificate drift: '+key)
    if not report.get('KnownProductionAnchorsVerified') or not report.get('CompilerBuiltFromSnapshot'):
        raise ValueError('initialization requires actual production measurement')


def sha(path: Path) -> str:
    with path.open('rb') as stream:
        digest=hashlib.sha256()
        for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
    return digest.hexdigest()

def export_tree(repo: Path, tree: str, destination: Path,
                prefixes: tuple[str, ...] = ()) -> dict[str, str]:
    """Read immutable Git blobs, never working-tree files or generated output."""
    if destination.exists():
        raise ValueError('fresh snapshot directory required')
    entries = subprocess.check_output(['git', '-C', str(repo), 'ls-tree',
        '-r', '-z', tree, '--', *prefixes]).split(b'\0')
    parsed = []
    for entry in entries:
        if not entry: continue
        metadata, raw_path = entry.split(b'\t', 1)
        mode, kind, oid = metadata.decode('ascii').split()
        name = raw_path.decode('utf-8')
        path = PurePosixPath(name)
        if (mode not in ('100644', '100755') or kind != 'blob'
                or path.is_absolute() or '..' in path.parts or '\\' in name):
            raise ValueError('unsupported snapshot input: ' + name)
        parsed.append((path, oid, mode))
    if not parsed: raise ValueError('empty source snapshot')
    destination.mkdir(parents=True)
    hashes = {}
    for relative, oid, mode in parsed:
        blob = subprocess.check_output(['git', '-C', str(repo), 'cat-file', 'blob', oid])
        # Bind actual bytes to the exact tree object, independently of checkout EOL.
        if hashlib.sha1(b'blob ' + str(len(blob)).encode() + b'\0' + blob).hexdigest() != oid:
            raise ValueError('Git blob identity mismatch: ' + str(relative))
        target = destination.joinpath(*relative.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(blob)
        if mode == '100755': target.chmod(0o755)
        hashes[relative.as_posix()] = hashlib.sha256(blob).hexdigest()
    return hashes

def measure(root: Path, agbcc: Path, output: Path) -> dict:
    root=root.resolve();agbcc=agbcc.resolve();output=output.resolve()
    compiler_head=subprocess.check_output(['git','-C',str(agbcc),'rev-parse','HEAD'],text=True).strip()
    source_tree=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD:vendor/vanillaplus'],text=True).strip()
    if compiler_head!=AGBCC_PIN or source_tree!=SOURCE_TREE:raise ValueError('compiler/vendor pin drift')
    if output.exists(): raise ValueError('fresh measurement output required')
    output.mkdir(parents=True)
    source=output/'source'
    header_hashes=export_tree(root,source_tree,source,('include','gflib'))
    compiler_source=output/'compiler'
    compiler_tree=subprocess.check_output(['git','-C',str(agbcc),
        'rev-parse',AGBCC_PIN+'^{tree}'],text=True).strip()
    compiler_inputs=export_tree(agbcc,compiler_tree,compiler_source,('gcc','ginclude'))
    if (compiler_source/'gcc/agbcc').exists():
        raise ValueError('compiler tree unexpectedly contains an executable')
    build_command=['make','-C',str(compiler_source/'gcc'),'-j1']
    with (output/'compiler-build.log').open('wb') as log:
        subprocess.run(build_command,stdout=log,stderr=log,check=True,timeout=600)
    compiler=compiler_source/'gcc/agbcc'
    if not compiler.is_file(): raise ValueError('fresh compiler binary missing')
    shim=output/'shim';shim.mkdir()
    shims={
        'stdint.h':'typedef unsigned char uint8_t; typedef signed char int8_t;\ntypedef unsigned short uint16_t; typedef signed short int16_t;\ntypedef unsigned int uint32_t; typedef signed int int32_t;\ntypedef unsigned long long uint64_t; typedef signed long long int64_t;\n',
        'string.h':'#include <stddef.h>\nvoid *memset(void *, int, size_t); void *memcpy(void *, const void *, size_t); int memcmp(const void *, const void *, size_t);\n',
        'limits.h':'#define CHAR_BIT 8\n#define INT_MAX 2147483647\n#define UINT_MAX 4294967295U\n',
        'map_groups.h':'/* Layout-only: no map IDs participate in save structs. */\n'}
    for name,text in shims.items():(shim/name).write_text(text,encoding='utf-8')
    probe=output/'probe.c'
    probe.write_text('#include "global.h"\n#include "pokemon_storage_system.h"\n#include "apprentice.h"\n#include <stddef.h>\nconst unsigned ap04_layout[] = {\n'+',\n'.join(EXPRESSIONS.values())+'\n};\n'+''.join('const '+kind+' '+name+' = {'+initial+'};\n' for name,(kind,initial,size) in BIT_PROBES.items()),encoding='utf-8')
    preprocessed=output/'probe.i';assembly=output/'probe.s'
    commands=[['gcc','-E','-P','-nostdinc','-DMODERN=0','-DVERSION=2',
        '-I'+str(shim),'-I'+str(compiler_source/'ginclude'),'-I'+str(source/'include'),
        '-I'+str(source/'gflib'),str(probe)],
        [str(compiler),str(preprocessed),'-mthumb-interwork','-O2','-o',str(assembly)]]
    with preprocessed.open('wb') as stdout, (output/'preprocess.log').open('wb') as stderr:
        subprocess.run(commands[0],stdout=stdout,stderr=stderr,check=True,timeout=60)
    with (output/'compiler.log').open('wb') as log:
        subprocess.run(commands[1],stdout=log,stderr=log,check=True,timeout=60)
    text=assembly.read_text(encoding='utf-8')
    if 'gcc2.9-arm' not in text.replace(' ',''):raise ValueError('actual AGBCC assembly identity absent')
    values=dict(zip(EXPRESSIONS,parse_words(text,len(EXPRESSIONS))))
    validate_anchors(values)
    report={'Schema':'remaster.ap04.agbcc-layout','Version':2,'SourceCommit':SOURCE_PIN,
        'VendorTree':source_tree,'CheckoutHead':subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip(),
        'CompilerPin':compiler_head,'CompilerTree':compiler_tree,
        'CompilerBinarySHA256':sha(compiler),'CompilerBuiltFromSnapshot':True,
        'CompilerInputHashes':compiler_inputs,'CompilerBuildCommand':build_command,
        'MODERN':0,'AssemblySHA256':sha(assembly),'ProbeSHA256':sha(probe),
        'Commands':commands,'SourceInputsFromGitBlobs':True,
        'SourceHeaderHashes':{'vendor/vanillaplus/'+p:h for p,h in header_hashes.items()},
        'Fields':values,'BitfieldDefaultBytes':{name:parse_data(text,name,values[size]).hex() for name,(kind,initial,size) in BIT_PROBES.items()},'KnownProductionAnchorsVerified':True,
        'ROMBuilt':False,'NewGameInitializationVerified':False,'UnrealRuntimeVerified':False}
    (output/'layout.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report

def main() -> int:
    parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--agbcc',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--certificate',type=Path)
    args=parser.parse_args();report=measure(args.source,args.agbcc,args.output)
    if args.certificate:validate_certificate(report,json.loads(args.certificate.read_text()))
    print('AP04 production AGBCC layout: '+str(len(report['Fields']))+' fields, all known anchors verified; no ROM/gameplay/runtime claim')
    return 0
if __name__=='__main__':raise SystemExit(main())
