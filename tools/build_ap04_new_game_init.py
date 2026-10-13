#!/usr/bin/env python3
"""Derive bounded persistent initialization data from pinned Git source and AGBCC layout."""
from __future__ import annotations
import argparse
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('prepare',ROOT/'tools/build_ap04_new_game_prepare.py')
prepare=importlib.util.module_from_spec(spec);spec.loader.exec_module(prepare)
EXTRA=('include/global.h','include/global.berry.h','include/pokemon_storage_system.h',
    'src/strings.c','src/new_game.c','src/player_pc.c','src/easy_chat.c','src/mauville_old_man.c',
    'src/trader.c','src/dewford_trend.c','src/berry.c','src/lilycove_lady.c','src/apprentice.c',
    'src/lottery_corner.c','src/frontier_util.c','src/pokemon_size_record.c','src/trainer_hill.c',
    'src/mail_data.c','src/battle_records.c','src/mystery_gift.c','src/union_room_chat.c',
    'src/pokemon_storage_system.c','src/data/lilycove_lady.h','src/data/contest_opponents.h',
    'src/data/battle_frontier/apprentice.h','src/data/wallpapers.h',
    'src/data/easy_chat/easy_chat_group_conditions.h','src/data/easy_chat/easy_chat_group_lifestyle.h',
    'src/data/easy_chat/easy_chat_group_hobbies.h','data/scripts/new_game.inc','charmap.txt')

def load(root: Path) -> dict[str,bytes]:
    tree=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD:vendor/vanillaplus'],text=True).strip()
    if tree!=prepare.SOURCE_TREE:raise ValueError('vendor drift')
    headers=subprocess.check_output(['git','-C',str(root),'ls-tree','-r','--name-only',tree,'--','include/constants'],text=True).splitlines()
    names=sorted(set(EXTRA)|set(headers))
    data=subprocess.check_output(['git','-C',str(root),'cat-file','--batch'],input=''.join(tree+':'+n+'\n' for n in names).encode())
    result={};offset=0
    for name in names:
        end=data.index(b'\n',offset);parts=data[offset:end].split()
        if len(parts)!=3 or parts[1]!=b'blob':raise ValueError('missing pinned input: '+name)
        start=end+1;stop=start+int(parts[2]);result[name]=data[start:stop];offset=stop+1
    if offset!=len(data):raise ValueError('Git input batch drift')
    return result

def uncomment(text: str) -> str:
    return re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)

def array(text: str, name: str) -> str:
    text=uncomment(text)
    found=re.search(r'\b'+re.escape(name)+r'\s*(?:\[[^]]*\])*\s*=\s*\{',text)
    if not found:raise ValueError('array absent: '+name)
    start=found.end();end=start;depth=1
    while depth and end<len(text):
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    if depth:raise ValueError('unterminated array')
    return text[start:end-1]

def split(text: str) -> list[str]:
    return [s.strip() for s in re.split(r',\s*(?![^()]*\))',text) if s.strip()]

class Constants:
    def __init__(self,texts):
        self.definitions={}
        for text in texts:
            for name,value in re.findall(r'^#define[ \t]+(\w+)[ \t]+([^\n]+)',uncomment(text),re.M):
                self.definitions.setdefault(name,value.strip())
        self.busy=set()
    def value(self,expression):
        text=re.sub(r'\b(0x[\da-fA-F]+|\d+)[uUlL]+\b',r'\1',expression.strip())
        return self.node(ast.parse(text,mode='eval').body)
    def node(self,node):
        if isinstance(node,ast.Constant) and type(node.value) is int:return node.value
        if isinstance(node,ast.Name):
            if node.id in self.busy or node.id not in self.definitions:raise ValueError('unknown/recursive constant: '+node.id)
            self.busy.add(node.id)
            try:return self.value(self.definitions[node.id])
            finally:self.busy.remove(node.id)
        if isinstance(node,ast.BinOp):
            a,b=self.node(node.left),self.node(node.right)
            if isinstance(node.op,ast.Add):return a+b
            if isinstance(node.op,ast.Sub):return a-b
            if isinstance(node.op,ast.Mult):return a*b
            if isinstance(node.op,ast.Div):return a//b
            if isinstance(node.op,ast.Mod):return a%b
            if isinstance(node.op,ast.LShift):return a<<b
            if isinstance(node.op,ast.BitOr):return a|b
            if isinstance(node.op,ast.BitAnd):return a&b
        if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and len(node.args)==1 and isinstance(node.args[0],ast.Name):
            prefix={'EC_MOVE1':('EC_GROUP_MOVE_1','MOVE_'),'EC_MOVE2':('EC_GROUP_MOVE_2','MOVE_'),
                'EC_POKEMON':('EC_GROUP_POKEMON','SPECIES_')}.get(node.func.id)
            if prefix:return (self.value(prefix[0])<<self.value('EC_MASK_BITS'))|self.value(prefix[1]+node.args[0].id)
        raise ValueError('unsupported constant expression: '+ast.dump(node))

def derive(files,layout):
    if layout['SourceCommit']!=prepare.SOURCE_PIN or layout['VendorTree']!=prepare.SOURCE_TREE or layout['CompilerPin']!='da598c1d918402c42c0c0d7128ba14567f3175e9':raise ValueError('production layout provenance')
    if not layout['KnownProductionAnchorsVerified'] or not layout['CompilerBuiltFromSnapshot']:raise ValueError('unverified production layout')
    text={n:b.decode('utf-8') for n,b in files.items()};c=Constants(text.values());f=layout['Fields']
    wallpapers=re.search(r'enum\s*\{(.*?)\}',uncomment(text['src/data/wallpapers.h']),re.S)
    for i,name in enumerate(split(wallpapers.group(1))):
        if not re.fullmatch(r'WALLPAPER_\w+',name):raise ValueError('wallpaper enumeration drift')
        c.definitions[name]=str(i)
    blocks=[bytearray(f['sb2.bytes']),bytearray(f['sb1.bytes']),bytearray(f['storage.bytes'])]
    if [len(b) for b in blocks]!=[3908,15816,33744]:raise ValueError('production save size drift')
    def put(block,offset,data):
        if offset<0 or offset+len(data)>len(block):raise ValueError('initialization bounds')
        block[offset:offset+len(data)]=data
    def word(block,offset,value,width=2):put(block,offset,int(value).to_bytes(width,'little'))
    sb2,sb1,storage=blocks
    bits=layout['BitfieldDefaultBytes']
    if bytes.fromhex(bits['ap04_apprentice_bits'])!=b'\x10'+bytes(67) or bytes.fromhex(bits['ap04_berry_bits'])!=b'\x00\x85'+bytes(6):
        raise ValueError('production apprentice/berry bit packing drift')
    if [int.from_bytes(bytes.fromhex(bits[n])[:2],'little') for n in
            ['ap04_trend_current','ap04_trend_max','ap04_trend_gaining']] != [127,16256,16384]:
        raise ValueError('production trend bit packing drift')
    option=bytes.fromhex(bits['ap04_option_bits'])
    if option!=bytes(20)+b'\x01'+bytes(len(sb2)-21):raise ValueError('production option placement drift')
    cmap={}
    for glyph,raw in re.findall(r"^'([^']+)'\s*=\s*((?:[\dA-Fa-f]{2} ?)+)\s*$",text['charmap.txt'],re.M):cmap[glyph]=bytes.fromhex(raw)
    for glyph,raw in re.findall(r'^([A-Z][A-Z_0-9]+)\s*=\s*((?:[\dA-Fa-f]{2} ?)+)\s*$',text['charmap.txt'],re.M):cmap['{'+glyph+'}']=bytes.fromhex(raw)
    def encode(value):
        output=b''
        while value:
            matches=[key for key in cmap if value.startswith(key)]
            if not matches:raise ValueError('unencodable source string: '+value)
            key=max(matches,key=len);output+=cmap[key];value=value[len(key):]
        return output+b'\xff'
    def string(name):
        m=re.search(r'\b'+name+r'\[\]\s*=\s*_\("([^"\\]*)"\)',text['src/strings.c'])
        if not m:raise ValueError('source string absent: '+name)
        return encode(m.group(1))
    def values(file,name):return [c.value(v) for v in split(array(text[file],name))]
    word(sb1,f['sb1.money'],3000,4)
    pc=re.findall(r'\{\s*(\w+)\s*,\s*(\d+)\s*\}',array(text['src/player_pc.c'],'sNewGamePCItems'))
    if pc!=[('ITEM_POTION','1'),('ITEM_NONE','0')]:raise ValueError('PC source changed')
    word(sb1,f['sb1.pcItems'],c.value(pc[0][0]));word(sb1,f['sb1.pcItems']+2,int(pc[0][1]))
    for i in range(c.value('MAIL_COUNT')):
        start=f['sb1.mail']+i*f['mail.bytes']
        put(sb1,start+f['mail.words'],b'\xff\xff'*c.value('MAIL_WORDS_COUNT'))
        put(sb1,start+f['mail.playerName'],b'\xff'*(c.value('PLAYER_NAME_LENGTH')+1))
        word(sb1,start+f['mail.species'],c.value('SPECIES_BULBASAUR'))
    for i in range(c.value('SECRET_BASES_COUNT')):put(sb1,f['sb1.secretBases']+i*f['secret_base.bytes']+f['secret_base.trainerName'],b'\xff'*c.value('PLAYER_NAME_LENGTH'))
    word(sb1,f['sb1.gabbyAndTyData']+f['gabby.quote'],65535)
    for name,field in [('sDefaultProfileWords','easyChatProfile'),('sDefaultBattleStartWords','easyChatBattleStart'),('sDefaultBattleWonWords','easyChatBattleWon'),('sDefaultBattleLostWords','easyChatBattleLost')]:
        for i,value in enumerate(values('src/easy_chat.c',name)):word(sb1,f['sb1.'+field]+i*2,value)
    for i in range(c.value('LINK_B_RECORDS_COUNT')):put(sb1,f['sb1.linkBattleRecords']+i*f['link_record.bytes']+f['link_record.name'],b'\xff')
    for i in range(c.value('NUM_QUESTIONNAIRE_WORDS')):word(sb1,f['sb1.mysteryGift.questionnaireWords']+i*2,65535)
    for i,name in enumerate(['Hello','Pokemon2','Trade','Battle','Lets','Ok','Sorry','YaySmileEmoji','ThankYou','ByeBye']):
        data=string('gText_'+name)
        if len(data)>21:raise ValueError('registered phrase overflow')
        put(sb1,f['sb1.registeredTexts']+i*21,data)
    for i in range(c.value('NUM_TRAINER_HILL_MODES')):word(sb1,f['sb1.trainerHillTimes']+i*4,c.value('HILL_MAX_TIME'),4)
    word(sb1,f['sb1.waldaPhrase'],21|(25<<5)|(30<<10));word(sb1,f['sb1.waldaPhrase']+2,6|(12<<5)|(24<<10))
    put(sb1,f['sb1.waldaPhrase.text'],b'\xff')
    for i in range(c.value('TOTAL_BOXES_COUNT')):
        data=string('gText_Box')[:-1]+encode(str(i+1))
        if len(data)>9:raise ValueError('box name overflow')
        put(storage,f['storage.boxNames']+i*9,data)
        put(storage,f['storage.boxWallpapers']+i,bytes([i%(c.value('MAX_DEFAULT_WALLPAPER')+1)]))
    for i in range(2):put(sb2,f['sb2.frontier']+f['frontier.opponentNames']+i*8,b'\xff')
    for i in range(c.value('APPRENTICE_COUNT')):
        start=f['sb2.apprentices']+i*f['apprentice.bytes']
        put(sb2,start,bytes.fromhex(layout['BitfieldDefaultBytes']['ap04_apprentice_bits']))
        put(sb2,start+f['apprentice.speechWon'],b'\xff\xff'*c.value('EASY_CHAT_BATTLE_WORDS_COUNT'))
        put(sb2,start+f['apprentice.playerName'],b'\xff');put(sb2,start+f['apprentice.language'],bytes([c.value('GAME_LANGUAGE')]))
    for i in range(c.value('HALL_FACILITIES_COUNT')*c.value('FRONTIER_LVL_MODE_COUNT')*c.value('HALL_RECORDS_COUNT')):put(sb2,f['sb2.hallRecords1P']+i*f['ranking_1p.bytes']+f['ranking_1p.name'],b'\xff')
    for i in range(c.value('FRONTIER_LVL_MODE_COUNT')*c.value('HALL_RECORDS_COUNT')):
        for field in ['name1','name2']:put(sb2,f['sb2.hallRecords2P']+i*f['ranking_2p.bytes']+f['ranking_2p.'+field],b'\xff')
    # Museum names use EOS; hall winners come from unchanged source records.
    for i in range(c.value('NUM_CONTEST_WINNERS')):
        for field in ['monName','trainerName']:put(sb1,f['sb1.contestWinners']+i*f['contest_winner.bytes']+f['contest_winner.'+field],b'\xff')
    winners=array(text['src/data/contest_opponents.h'],'gDefaultContestWinners')
    entries=re.findall(r'\[([^]]+)\]\s*=\s*\{(.*?)\}',winners,re.S)
    if len(entries)!=c.value('MUSEUM_CONTEST_WINNERS_START'):raise ValueError('default winner coverage')
    for index,body in entries:
        start=f['sb1.contestWinners']+c.value(index)*f['contest_winner.bytes']
        fields=dict(re.findall(r'\.(\w+)\s*=\s*([^,]+)',body))
        for field,offset,width in [('personality',0,4),('trainerId',4,4),('species',8,2),('contestCategory',10,1),('contestRank',30,1)]:word(sb1,start+offset,c.value(fields[field]),width)
        for field in ['monName','trainerName']:
            m=re.fullmatch(r'_\("([^"]*)"\)',fields[field].strip())
            if not m:raise ValueError('winner name source')
            put(sb1,start+f['contest_winner.'+field],encode(m.group(1)))
    script=uncomment(text['data/scripts/new_game.inc'])
    flags_part=script.split('EventScript_ResetAllMapFlags::',1)[1]
    if set(re.findall(r'^\s*(\w+)\b',flags_part,re.M))-{'setflag','call','end'}:raise ValueError('unsupported reset script command')
    flags=[c.value(n) for n in re.findall(r'^\s*setflag\s+(\w+)',flags_part,re.M)]
    for flag in flags:
        if not 0<=flag<c.value('FLAGS_COUNT'):raise ValueError('flag bounds')
        sb1[f['sb1.flags']+flag//8]|=1<<(flag%8)
    berry_rows=re.findall(r'^\s*setberrytree\s+(\w+),\s*ITEM_TO_BERRY\((\w+)\),\s*(\w+)',script,re.M)
    for tree,item,stage in berry_rows:
        tree_id=c.value(tree);berry=c.value(item)-c.value('FIRST_BERRY_INDEX')+1
        pattern=r'\['+item+r'\s*-\s*FIRST_BERRY_INDEX\]\s*=\s*\{(.*?)\}'
        data=re.search(pattern,text['src/berry.c'],re.S).group(1)
        minimum=int(re.search(r'\.minYield\s*=\s*(\d+)',data).group(1))
        duration=int(re.search(r'\.stageDuration\s*=\s*(\d+)',data).group(1))*60*4
        if not 0<=tree_id<c.value('BERRY_TREES_COUNT') or c.value(stage)!=c.value('BERRY_STAGE_BERRIES'):raise ValueError('berry source bounds/stage')
        start=f['sb1.berryTrees']+tree_id*f['berry_tree.bytes']
        put(sb1,start,bytes.fromhex(layout['BitfieldDefaultBytes']['ap04_berry_bits']))
        put(sb1,start+f['berry_tree.berry'],bytes([berry]));word(sb1,start+f['berry_tree.minutesUntilNextStage'],duration)
        put(sb1,start+f['berry_tree.berryYield'],bytes([minimum]))
    # Selected variables that survive InitEventData and source callbacks.
    vars_={n:c.value(n) for n in ['VAR_SEEDOT_SIZE_RECORD','VAR_LOTAD_SIZE_RECORD','VAR_OBJ_GFX_ID_0','VAR_POKELOT_RND1','VAR_POKELOT_RND2']}
    for name in ['VAR_SEEDOT_SIZE_RECORD','VAR_LOTAD_SIZE_RECORD']:word(sb1,f['sb1.vars']+2*(vars_[name]-c.value('VARS_START')),0x8000)
    word(sb1,f['sb1.vars']+2*(vars_['VAR_OBJ_GFX_ID_0']-c.value('VARS_START')),c.value('OBJ_EVENT_GFX_BARD'))
    constants={k:c.value(k) for k in ['GAME_LANGUAGE','VARS_START','EC_MASK_BITS','EC_GROUP_CONDITIONS','EC_GROUP_LIFESTYLE','EC_GROUP_HOBBIES','NUM_APPRENTICES']}
    constants.update(vars_)
    # Source arrays for RNG-dependent domains are retained as numeric data, not guessed.
    lists={}
    for name in ['sQuizLadyQuizAnswers','sQuizLadyPrizes']:
        lists[name]=values('src/data/lilycove_lady.h',name)
    lists['quizQuestions']=[values('src/data/lilycove_lady.h',name) for name in split(array(text['src/data/lilycove_lady.h'],'sQuizLadyQuizQuestions'))]
    lists['favorItems']=[values('src/data/lilycove_lady.h',name)[:-1] for name in split(array(text['src/data/lilycove_lady.h'],'sFavorLadyAcceptedItemLists'))]
    lists['initialApprentices']=values('src/data/battle_frontier/apprentice.h','sInitialApprenticeIds')
    if (c.value('SAVED_TRENDS_COUNT')!=5 or c.value('QUIZ_QUESTION_LEN')!=9
            or c.value('CONTEST_CATEGORIES_COUNT')!=5 or c.value('GAME_LANGUAGE')!=2
            or len(lists['quizQuestions'])!=16 or len(lists['favorItems'])!=6
            or any(len(row)!=9 for row in lists['quizQuestions'])
            or len(lists['sQuizLadyQuizAnswers'])!=16 or len(lists['sQuizLadyPrizes'])!=16
            or any(not row or len(row)>255 for row in lists['favorItems'])
            or any(value>=c.value('NUM_APPRENTICES') for value in lists['initialApprentices'])):
        raise ValueError('source domain dimensions drift')
    lists['bardLyrics']=values('src/mauville_old_man.c','sDefaultBardSongLyrics')
    for group in ['conditions','lifestyle','hobbies']:
        body=array(text['src/data/easy_chat/easy_chat_group_'+group+'.h'],'gEasyChatGroup_'+group.capitalize())
        lists[group+'Count']=len(re.findall(r'\.text\s*=',body))
        if lists[group+'Count']<1:raise ValueError('word group empty')
    lists['traderDecorations']=values('src/trader.c','sDefaultTraderDecorations')
    lists['traderNames']=[list(string(name)) for name in split(array(text['src/trader.c'],'sDefaultTraderNames'))]
    men=[bytearray(f['old_man.bytes']) for i in range(5)]
    for i,man in enumerate(men):man[0]=i
    for i,value in enumerate(lists['bardLyrics']):word(men[0],f['sb1.oldMan.bard.songLyrics']-f['sb1.oldMan']+i*2,value)
    for man,field in [(0,'bard'),(1,'hipster'),(4,'giddy')]:
        men[man][f['sb1.oldMan.'+field+'.language']-f['sb1.oldMan']]=c.value('GAME_LANGUAGE')
    for i,data in enumerate(lists['traderNames']):
        put(men[2],f['sb1.oldMan.trader.playerNames']-f['sb1.oldMan']+i*11,bytes(data))
        men[2][f['sb1.oldMan.trader.decorations']-f['sb1.oldMan']+i]=lists['traderDecorations'][i]
        men[2][f['sb1.oldMan.trader.language']-f['sb1.oldMan']+i]=c.value('GAME_LANGUAGE')
    put(men[3],f['sb1.oldMan.storyteller.trainerNames']-f['sb1.oldMan'],b'\xff'*c.value('NUM_STORYTELLER_TALES'))
    lists['oldMen']=[list(m) for m in men]
    contract={'Schema':'remaster.ap04.new-game-persistent-data','Version':1,
        'SourceCommit':prepare.SOURCE_PIN,'VendorTree':prepare.SOURCE_TREE,
        'LayoutAssemblySHA256':layout['AssemblySHA256'],
        'SourceHashes':{n:hashlib.sha256(b).hexdigest() for n,b in files.items()},
        'BlockSHA256':[hashlib.sha256(b).hexdigest() for b in blocks],
        'InitialFlags':flags,'InitialBerryTrees':len(berry_rows),'Constants':constants,'Lists':lists,
        'RankingIdPolicy':'EXPLICIT_SOURCE_UBFIX_ZERO_ID','PersistentStateProjected':True,
        'FullOriginalNewGameInitDataExecuted':False,'UnrealRuntimeVerified':False,
        'PendingHostActions':['SOURCE_RTC_RESET','GLOBAL_RUNTIME_RESET','TRUCK_MAP_LOAD_AND_INTRO']}
    return blocks,contract

def emit(blocks,contract,layout):
    lines=['/* Pinned source-derived persistent data; production AGBCC offsets. */']
    spans=[]
    for block,data in enumerate(blocks):
        positions=[i for i,v in enumerate(data) if v]
        while positions:
            start=positions.pop(0);end=start+1
            while positions and positions[0]<=end+3:end=positions.pop(0)+1
            values=data[start:end];name='ng_data_'+str(len(spans))
            lines.append('static const uint8_t '+name+'[] = {'+','.join(str(v) for v in values)+'};')
            spans.append((block,start,len(values),name))
    lines.append('static const NgSpan kNgSpans[] = {')
    lines.extend('    {'+str(b)+','+str(s)+','+str(n)+','+name+'},' for b,s,n,name in spans);lines.append('};')
    for key,value in layout['Fields'].items():lines.append('#define NG_'+key.replace('.','_').upper()+' '+str(value))
    for key,value in contract['Constants'].items():lines.append('#define NG_'+key+' '+str(value))
    for key,value in contract['Lists'].items():
        if isinstance(value,int):lines.append('#define NG_'+key.upper()+' '+str(value));continue
        if key not in ('sQuizLadyQuizAnswers','sQuizLadyPrizes','quizQuestions','favorItems','initialApprentices','oldMen'):continue
        rows=value if isinstance(value[0],list) else [value]
        size=max(map(len,rows))
        shape='['+str(len(rows))+']['+str(size)+']' if isinstance(value[0],list) else '['+str(size)+']'
        raw=','.join('{'+','.join(map(str,row))+'}' for row in rows) if isinstance(value[0],list) else ','.join(map(str,value))
        lines.append('static const uint16_t ng_'+key+shape+' = {'+raw+'};')
        if key=='favorItems':lines.append('static const uint8_t ng_'+key+'Counts[] = {'+','.join(str(len(row)) for row in rows)+'};')
    for name in ['ap04_trend_current','ap04_trend_max','ap04_trend_gaining']:
        raw=bytes.fromhex(layout['BitfieldDefaultBytes'][name]);lines.append('#define NG_'+name.upper()+' '+str(int.from_bytes(raw[:2],'little')))
    return '\n'.join(lines)+'\n'

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--layout',type=Path,required=True);parser.add_argument('--catalog',type=Path,required=True)
    parser.add_argument('--contract',type=Path,required=True);args=parser.parse_args()
    layout=json.loads(args.layout.read_text());blocks,contract=derive(load(args.source),layout)
    for path,data in [(args.catalog,emit(blocks,contract,layout)),(args.contract,json.dumps(contract,indent=2)+'\n')]:
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data.encode())
    print('Source-derived persistent data generated; runtime and full original initialization execution are not claimed')
if __name__=='__main__':main()
