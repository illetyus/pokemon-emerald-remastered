#!/usr/bin/env python3
"""Compile unchanged pinned functions against semantic stubs; not a production ABI oracle."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('init',root/'tools/build_ap04_new_game_init.py')
init=importlib.util.module_from_spec(spec);spec.loader.exec_module(init)
FUNCTIONS={'src/dewford_trend.c':['InitDewfordTrend','SeedTrendRng','SortTrends','CompareTrends'],
    'src/easy_chat.c':['GetRandomEasyChatWordFromGroup'],
    'src/lottery_corner.c':['ResetLotteryCorner','SetLotteryNumber'],
    'src/lilycove_lady.c':['InitLilycoveLady','InitLilycoveQuizLady','QuizLadyPickQuestion',
        'InitLilycoveFavorLady','FavorLadyPickFavorAndBestItem','GetNumAcceptedItems',
        'InitLilycoveContestLady','ResetContestLadyContestData'],
    'src/apprentice.c':['SetApprenticeId'],'src/mail_data.c':['ClearMail']}

def generate(files,layout):
    text={n:b.decode() for n,b in files.items()}
    blocks,contract=init.derive(files,layout);lists=contract['Lists']
    original_inputs=init.prepare.source_files(root)
    _,oracle,_=init.prepare.derive(original_inputs)
    old='optionsWindowFrameType, optionsSound, optionsBattleStyle, optionsBattleSceneOff, regionMapZoom; };'
    assert oracle.count(old)==1
    oracle=oracle.replace(old,old[:-3]+'''
    struct { u8 id; } playerApprentice;
    struct { u8 number, id; } apprentices[4]; };
''')
    oracle+='''
typedef int16_t s16; typedef int32_t s32; typedef uint8_t bool8;
struct DewfordTrend { u16 trendiness,maxTrendiness,gainingTrendiness,rand,words[2]; };
struct LilycoveLadyQuiz { u8 id,state;u16 question[9],correctAnswer,playerAnswer;
    u8 playerName[8];u16 playerTrainerId[4],prize;u8 waitingForChallenger,prevQuestionId,language,questionId; };
struct LilycoveLadyFavor { u8 id,state,playerName[8],likedItem,numItemsGiven;u16 itemId,bestItem;u8 language,favorId; };
struct LilycoveLadyContest { u8 id,givenPokeblock,playerName[8],numGoodPokeblocksGiven,numOtherPokeblocksGiven,maxSheen,category,language; };
typedef union LilycoveLady { u8 id;struct LilycoveLadyQuiz quiz;struct LilycoveLadyFavor favor;struct LilycoveLadyContest contest; } LilycoveLady;
struct SemanticSaveBlock1 { struct DewfordTrend dewfordTrends[5];LilycoveLady lilycoveLady; };
struct Mail { u16 words[9];u8 playerName[8],trainerId[4];u16 species,itemId; };
static struct SemanticSaveBlock1 source_sb1;
static struct SemanticSaveBlock1 *gSaveBlock1Ptr=&source_sb1;
static struct LilycoveLadyQuiz *sQuizLadyPtr;
static struct LilycoveLadyFavor *sFavorLadyPtr;
static struct LilycoveLadyContest *sContestLadyPtr;
static u16 source_vars[256];
static const u8 gGameLanguage=2;
#define PLAYER_APPRENTICE (gSaveBlock2Ptr->playerApprentice)
#define TRUE 1
#define EOS 255
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define EC_WORD(g,i) (((g)<<9)|(i))
#define SORT_MODE_NORMAL 0
#define SORT_MODE_MAX_FIRST 1
#define SORT_MODE_FULL 2
#define SWAP(a,b,t) { (t)=(a);(a)=(b);(b)=(t); }
'''
    functions={name:init.prepare.function(text[file],name) for file,names in FUNCTIONS.items() for name in names}
    c=init.Constants(text.values())
    used=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b','\n'.join(functions.values())))|{'VARS_START'}
    defined={'PLAYER_APPRENTICE','TRUE','FALSE','EOS','ARRAY_COUNT','EC_WORD','SORT_MODE_NORMAL','SORT_MODE_MAX_FIRST','SORT_MODE_FULL','SWAP'}
    for name in sorted(used-defined):
        if name in c.definitions:oracle+='#define '+name+' '+str(c.value(name))+'\n'
    oracle+='static void VarSet(u16 id,u16 value) { source_vars[id-VARS_START]=value; }\n'
    oracle+='struct WordGroup { u16 numWords;union {const u16 *valueList;} wordData; };\n'
    oracle+='static const struct WordGroup gEasyChatGroups[22] = {\n'
    for group,name in [('EC_GROUP_CONDITIONS','conditions'),('EC_GROUP_LIFESTYLE','lifestyle'),('EC_GROUP_HOBBIES','hobbies')]:
        oracle+='    ['+str(c.value(group))+'] = {'+str(lists[name+'Count'])+',{0}},\n'
    oracle+='};\n'
    def table(name,rows):
        nonlocal oracle
        if isinstance(rows[0],list):
            size=max(map(len,rows));shape='['+str(len(rows))+']['+str(size)+']'
            body=','.join('{'+','.join(map(str,r))+'}' for r in rows)
        else:shape='['+str(len(rows))+']';body=','.join(map(str,rows))
        oracle+='static const u16 '+name+shape+'={'+body+'};\n'
    table('sQuizLadyQuizQuestions',lists['quizQuestions'])
    table('sQuizLadyQuizAnswers',lists['sQuizLadyQuizAnswers']);table('sQuizLadyPrizes',lists['sQuizLadyPrizes'])
    table('sFavorLadyAcceptedItemLists',[row+[0] for row in lists['favorItems']])
    table('sInitialApprenticeIds',lists['initialApprentices'])
    oracle+='static const u8 *sFavorLadyRequests[6]={0};\n'
    for body in functions.values():oracle+=body[:body.index('{')].strip()+';\n'
    oracle+='\n\n'.join(functions.values())+'\n'
    oracle+='''void ap04_source_init_domains(u32 rng,u16 lower,u16 trends[5][6],
    u16 lottery[2],u16 lady[16],u8 *apprentice,u32 *after,u32 *calls) {
    unsigned i;
    memset(&source_save,0,sizeof source_save);memset(&source_sb1,0,sizeof source_sb1);
    memset(source_vars,0,sizeof source_vars);memset(lady,0,16*sizeof *lady);
    gRngValue=rng;sTrainerId=lower;sRandCount=0;InitPlayerTrainerId();
    source_save.apprentices[0].id=NUM_APPRENTICES;
    InitDewfordTrend();ResetLotteryCorner();InitLilycoveLady();SetApprenticeId();
    for(i=0;i<5;++i) {
        const struct DewfordTrend *t=&source_sb1.dewfordTrends[i];
        trends[i][0]=t->trendiness;trends[i][1]=t->maxTrendiness;trends[i][2]=t->gainingTrendiness;
        trends[i][3]=t->rand;trends[i][4]=t->words[0];trends[i][5]=t->words[1];
    }
    lottery[0]=source_vars[VAR_POKELOT_RND1-VARS_START];lottery[1]=source_vars[VAR_POKELOT_RND2-VARS_START];
    lady[0]=source_sb1.lilycoveLady.id;
    if(lady[0]==LILYCOVE_LADY_QUIZ) {
        for(i=0;i<9;++i)lady[i+1]=sQuizLadyPtr->question[i];
        lady[10]=sQuizLadyPtr->correctAnswer;lady[11]=sQuizLadyPtr->prize;lady[12]=sQuizLadyPtr->questionId;
    } else if(lady[0]==LILYCOVE_LADY_FAVOR) {lady[1]=sFavorLadyPtr->favorId;lady[2]=sFavorLadyPtr->bestItem;}
    else lady[1]=sContestLadyPtr->category;
    *apprentice=source_save.playerApprentice.id;*after=gRngValue;*calls=sRandCount;
}
void ap04_source_mail(u16 words[9],u8 name[8],u16 *species) {
    struct Mail mail;unsigned i;memset(&mail,0xA7,sizeof mail);ClearMail(&mail);
    for(i=0;i<9;++i)words[i]=mail.words[i];
    memcpy(name,mail.playerName,8);*species=mail.species;
}
'''
    return oracle,{name:hashlib.sha256(body.encode()).hexdigest() for name,body in functions.items()}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--layout',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();oracle,hashes=generate(init.load(args.source),json.loads(args.layout.read_text()))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(oracle.encode())
    args.output.with_suffix('.json').write_bytes((json.dumps(hashes,indent=2)+'\n').encode())
    layout=json.loads(args.layout.read_text());blocks,_=init.derive(init.load(args.source),layout)
    fields=layout['Fields']
    ranges=[[(0,176),(fields['sb2.playerApprentice'],1)],
        [(fields['sb1.pos'],4),(fields['sb1.location'],8),(fields['sb1.mapLayoutId'],2),
         (fields['sb1.oldMan'],64),(fields['sb1.dewfordTrends'],40),(fields['sb1.lilycoveLady'],64),
         (fields['sb1.vars']+2*(16459-16384),4)],[]]
    header='/* Measured offsets and masked fixed-data digest; domain oracle is separate. */\n'
    for name,value in fields.items():header+='#define F_'+name.replace('.','_').upper()+' '+str(value)+'\n'
    for block,r in zip(blocks,ranges):
        for offset,length in r:block[offset:offset+length]=bytes(length)
    def digest(block):
        result=14695981039346656037
        for byte in block:result=((result^byte)*1099511628211)&0xffffffffffffffff
        return result
    header+='static const uint64_t fixed_hashes[3]={'+','.join('UINT64_C('+str(digest(b))+')' for b in blocks)+'};\n'
    args.output.with_suffix('.h').write_bytes(header.encode())
    print('17 unchanged original C domain functions generated; semantic comparison, not source ABI')
if __name__=='__main__':main()
