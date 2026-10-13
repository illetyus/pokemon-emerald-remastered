#include "remaster/emerald_new_game.h"
#include <string.h>

typedef struct NewGameKey { const char *utf8; uint8_t length, code; } NewGameKey;
#include "emerald_new_game_catalog.inc"

static int new_game_allowed_byte(uint8_t code)
{
    size_t i;
    for (i=0;i<sizeof kNewGameKeys/sizeof kNewGameKeys[0];++i)
        if (kNewGameKeys[i].code==code) return 1;
    return 0;
}
static int new_game_name_length(const uint8_t name[8])
{
    unsigned i, nonspace=0;
    for (i=0;i<8;++i) {
        if (name[i]==0xFF) return i>0 && nonspace ? (int)i : -1;
        if (i==7 || !new_game_allowed_byte(name[i])) return -1;
        if (name[i]!=0) nonspace=1;
    }
    return -1;
}
RemasterEmeraldNewGameResult remaster_emerald_new_game_encode_name(
    const char *utf8, size_t length, uint8_t out_name[8])
{
    uint8_t encoded[8]={0};
    size_t pos=0, count=0, nonspace=0;
    if (!utf8 || !out_name || length==0 || length>28)
        return REMASTER_EMERALD_NEW_GAME_INVALID_NAME;
    while (pos<length) {
        size_t i;
        int found=0;
        if (count==7) return REMASTER_EMERALD_NEW_GAME_INVALID_NAME;
        for (i=0;i<sizeof kNewGameKeys/sizeof kNewGameKeys[0];++i) {
            const NewGameKey *key=&kNewGameKeys[i];
            if (key->length<=length-pos && memcmp(utf8+pos,key->utf8,key->length)==0) {
                encoded[count++]=key->code;
                pos+=key->length;nonspace|=key->code!=0;found=1;break;
            }
        }
        if (!found) return REMASTER_EMERALD_NEW_GAME_INVALID_NAME;
    }
    if (!nonspace) return REMASTER_EMERALD_NEW_GAME_INVALID_NAME;
    encoded[count]=0xFF;
    memcpy(out_name,encoded,sizeof encoded);
    return REMASTER_EMERALD_NEW_GAME_PREPARED;
}
RemasterEmeraldNewGameResult remaster_emerald_new_game_prepare(
    const RemasterEmeraldNewGameRequest *request, RemasterEmeraldNewGameDraft *out_draft)
{
    RemasterEmeraldNewGameDraft candidate;
    uint32_t next, trainer;
    int name_length;
    unsigned i;
    if (!request || !out_draft) return REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT;
    if (request->format!=REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS)
        return REMASTER_EMERALD_NEW_GAME_UNSUPPORTED_FORMAT;
    if (request->avatar>1) return REMASTER_EMERALD_NEW_GAME_INVALID_AVATAR;
    name_length=new_game_name_length(request->player_name);
    if (name_length<0) return REMASTER_EMERALD_NEW_GAME_INVALID_NAME;
    memset(&candidate,0,sizeof candidate);
    candidate.stage=REMASTER_EMERALD_NEW_GAME_DRAFT;
    memcpy(candidate.save_block2_prefix,request->player_name,(size_t)name_length+1);
    candidate.save_block2_prefix[8]=request->avatar;
    candidate.save_block2_prefix[0x14]=NEW_GAME_DEFAULT_OPTIONS & 0xFFu;
    candidate.save_block2_prefix[0x15]=NEW_GAME_DEFAULT_OPTIONS >> 8;
    next=request->rng_state*UINT32_C(1103515245)+UINT32_C(24691);
    trainer=(next & UINT32_C(0xFFFF0000))|request->trainer_id_lower;
    for(i=0;i<4;++i) candidate.save_block2_prefix[10+i]=(uint8_t)(trainer>>(8*i));
    candidate.rng_after_trainer_id=next;
    candidate.pending_source_calls=NEW_GAME_PENDING_CALLS;
    candidate.map_group=NEW_GAME_TRUCK_GROUP;candidate.map_num=NEW_GAME_TRUCK_MAP;
    candidate.map_layout_id=NEW_GAME_TRUCK_LAYOUT;
    candidate.player_x=NEW_GAME_TRUCK_X;candidate.player_y=NEW_GAME_TRUCK_Y;
    candidate.warp_id=-1;candidate.warp_x=-1;candidate.warp_y=-1;
    *out_draft=candidate;
    return REMASTER_EMERALD_NEW_GAME_PREPARED;
}

/* Canonical persistent NewGameInitData projection; hardware/world reset is
 * returned to the session host, never performed as machine or disk I/O here. */
typedef struct NgSpan { uint8_t block; uint16_t offset, length; const uint8_t *data; } NgSpan;
#include "emerald_new_game_init_catalog.inc"

typedef struct NgRng { uint32_t state, calls; } NgRng;
typedef struct NgTrend { uint16_t current, maximum, gaining, random, words[2]; } NgTrend;
static uint16_t ng_random(NgRng *rng)
{
    rng->state=rng->state*UINT32_C(1103515245)+UINT32_C(24691);
    ++rng->calls;
    return (uint16_t)(rng->state>>16);
}
static void ng_u16(uint8_t *dest, uint16_t value)
{ dest[0]=(uint8_t)value;dest[1]=(uint8_t)(value>>8); }
static void ng_variable(RemasterEmeraldSave *save, unsigned id, uint16_t value)
{ ng_u16(save->save_block1+NG_SB1_VARS+2*(id-NG_VARS_START),value); }
static void ng_trends(RemasterEmeraldSave *save, NgRng *rng)
{
    NgTrend trends[5];
    unsigned i,j;
    for(i=0;i<5;++i) {
        uint16_t count,group,rand;
        trends[i].words[0]=(uint16_t)((NG_EC_GROUP_CONDITIONS<<NG_EC_MASK_BITS)
            | ng_random(rng)%NG_CONDITIONSCOUNT);
        if(ng_random(rng)&1) { count=NG_LIFESTYLECOUNT;group=NG_EC_GROUP_LIFESTYLE; }
        else { count=NG_HOBBIESCOUNT;group=NG_EC_GROUP_HOBBIES; }
        trends[i].words[1]=(uint16_t)((group<<NG_EC_MASK_BITS)|ng_random(rng)%count);
        trends[i].gaining=ng_random(rng)&1;
        rand=ng_random(rng)%98;
        if(rand>50) { rand=ng_random(rng)%98;if(rand>80)rand=ng_random(rng)%98; }
        trends[i].maximum=rand+30;
        trends[i].current=(ng_random(rng)%(rand+1))+30;
        trends[i].random=ng_random(rng);
    }
    for(i=0;i<5;++i) for(j=i+1;j<5;++j) {
        int higher;
        if(trends[j].current!=trends[i].current) higher=trends[j].current>trends[i].current;
        else if(trends[j].maximum!=trends[i].maximum) higher=trends[j].maximum>trends[i].maximum;
        else higher=ng_random(rng)&1;
        if(higher) { NgTrend temporary=trends[i];trends[i]=trends[j];trends[j]=temporary; }
    }
    for(i=0;i<5;++i) {
        uint8_t *dest=save->save_block1+NG_SB1_DEWFORDTRENDS+i*NG_TREND_BYTES;
        /* Actual AGBCC masks are checked by the data generator, not host C bitfields. */
        ng_u16(dest,(uint16_t)(trends[i].current|(trends[i].maximum<<7)|(trends[i].gaining<<14)));
        ng_u16(dest+NG_TREND_RAND,trends[i].random);
        ng_u16(dest+NG_TREND_WORDS,trends[i].words[0]);ng_u16(dest+NG_TREND_WORDS+2,trends[i].words[1]);
    }
}
static void ng_lady(RemasterEmeraldSave *save, NgRng *rng, uint16_t lower)
{
    unsigned id=(lower%6)>>1;
    uint8_t *dest=save->save_block1+NG_SB1_LILYCOVELADY;
    dest[0]=(uint8_t)id;
    if(id==0) {
        unsigned i,question=ng_random(rng)%(sizeof ng_quizQuestions/sizeof ng_quizQuestions[0]);
        for(i=0;i<9;++i)ng_u16(dest+NG_QUIZ_QUESTION+i*2,ng_quizQuestions[question][i]);
        ng_u16(dest+NG_QUIZ_CORRECTANSWER,ng_sQuizLadyQuizAnswers[question]);
        ng_u16(dest+NG_QUIZ_PLAYERANSWER,UINT16_MAX);
        ng_u16(dest+NG_QUIZ_PRIZE,ng_sQuizLadyPrizes[question]);
        dest[NG_QUIZ_PLAYERNAME]=0xFF;dest[NG_QUIZ_QUESTIONID]=(uint8_t)question;
        dest[NG_QUIZ_PREVQUESTIONID]=(uint8_t)(sizeof ng_quizQuestions/sizeof ng_quizQuestions[0]);
        dest[NG_QUIZ_LANGUAGE]=NG_GAME_LANGUAGE;
    } else if(id==1) {
        unsigned favor=ng_random(rng)%(sizeof ng_favorItems/sizeof ng_favorItems[0]);
        unsigned item=ng_random(rng)%ng_favorItemsCounts[favor];
        dest[NG_FAVOR_PLAYERNAME]=0xFF;dest[NG_FAVOR_FAVORID]=(uint8_t)favor;
        ng_u16(dest+NG_FAVOR_BESTITEM,ng_favorItems[favor][item]);dest[NG_FAVOR_LANGUAGE]=NG_GAME_LANGUAGE;
    } else {
        dest[NG_CONTEST_LADY_PLAYERNAME]=0xFF;dest[NG_CONTEST_LADY_CATEGORY]=(uint8_t)(ng_random(rng)%5);
        dest[NG_CONTEST_LADY_LANGUAGE]=NG_GAME_LANGUAGE;
    }
}
static int ng_overlaps(const void *a,size_t na,const void *b,size_t nb)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y ? y-x<na : x-y<nb;
}
RemasterEmeraldNewGameResult remaster_emerald_new_game_initialize(
    const RemasterEmeraldNewGameInitRequest *request,
    RemasterEmeraldSave *workspace, RemasterEmeraldSave *out_save,
    RemasterEmeraldNewGameRuntime *out_runtime)
{
    RemasterEmeraldNewGameDraft draft;
    RemasterEmeraldNewGameRuntime runtime;
    RemasterEmeraldNewGameResult result;
    NgRng rng;
    size_t i;
    if(!request || !workspace || !out_save || !out_runtime
        || ng_overlaps(workspace,sizeof *workspace,out_save,sizeof *out_save)
        || ng_overlaps(workspace,sizeof *workspace,request,sizeof *request)
        || ng_overlaps(workspace,sizeof *workspace,out_runtime,sizeof *out_runtime)
        || ng_overlaps(out_save,sizeof *out_save,out_runtime,sizeof *out_runtime)
        || ng_overlaps(out_save,sizeof *out_save,request,sizeof *request)
        || ng_overlaps(out_runtime,sizeof *out_runtime,request,sizeof *request))
        return REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT;
    if(request->observed_save_status!=REMASTER_EMERALD_SAVE_EMPTY)
        return REMASTER_EMERALD_NEW_GAME_NOT_EMPTY;
    if(request->ranking_id_policy!=REMASTER_EMERALD_NEW_GAME_SOURCE_UBFIX_ZERO_ID)
        return REMASTER_EMERALD_NEW_GAME_INVALID_POLICY;
    if(request->rtc_after_reset.days<0 || request->rtc_after_reset.hours<0
        || request->rtc_after_reset.hours>23 || request->rtc_after_reset.minutes<0
        || request->rtc_after_reset.minutes>59 || request->rtc_after_reset.seconds<0
        || request->rtc_after_reset.seconds>59)
        return REMASTER_EMERALD_NEW_GAME_INVALID_CLOCK;
    result=remaster_emerald_new_game_prepare(&request->identity,&draft);
    if(result!=REMASTER_EMERALD_NEW_GAME_PREPARED)return result;
    memset(workspace,0,sizeof *workspace);
    for(i=0;i<sizeof kNgSpans/sizeof kNgSpans[0];++i) {
        const NgSpan *span=&kNgSpans[i];
        uint8_t *block=span->block==0?workspace->save_block2:span->block==1?workspace->save_block1:workspace->pokemon_storage;
        memcpy(block+span->offset,span->data,span->length);
    }
    memcpy(workspace->save_block2,draft.save_block2_prefix,sizeof draft.save_block2_prefix);
    ng_u16(workspace->save_block1+NG_SB1_POS,(uint16_t)draft.player_x);
    ng_u16(workspace->save_block1+NG_SB1_POS+2,(uint16_t)draft.player_y);
    workspace->save_block1[NG_SB1_LOCATION]=(uint8_t)draft.map_group;
    workspace->save_block1[NG_SB1_LOCATION+1]=(uint8_t)draft.map_num;
    workspace->save_block1[NG_SB1_LOCATION+2]=0xFF;
    ng_u16(workspace->save_block1+NG_SB1_LOCATION+4,UINT16_MAX);
    ng_u16(workspace->save_block1+NG_SB1_LOCATION+6,UINT16_MAX);
    ng_u16(workspace->save_block1+NG_SB1_MAPLAYOUTID,draft.map_layout_id);
    for(i=0;i<NG_OLD_MAN_BYTES;++i)workspace->save_block1[NG_SB1_OLDMAN+i]=(uint8_t)ng_oldMen[(request->identity.trainer_id_lower%10)/2][i];
    rng.state=draft.rng_after_trainer_id;rng.calls=1;
    ng_trends(workspace,&rng);
    { uint16_t low=ng_random(&rng),high=ng_random(&rng);
      ng_variable(workspace,NG_VAR_POKELOT_RND1,low);ng_variable(workspace,NG_VAR_POKELOT_RND2,high); }
    ng_lady(workspace,&rng,request->identity.trainer_id_lower);
    workspace->save_block2[NG_SB2_PLAYERAPPRENTICE]=(uint8_t)ng_initialApprentices[
        ng_random(&rng)%(sizeof ng_initialApprentices/sizeof ng_initialApprentices[0])];
    /* EMPTY is the existing R17 deliberate initial checkpoint, not a missing-file
     * assertion. store_platform independently requires actual typed MISSING. */
    workspace->status=REMASTER_EMERALD_SAVE_EMPTY;
    memset(&runtime,0,sizeof runtime);
    runtime.rng_state=rng.state;runtime.rng_calls=rng.calls;
    runtime.match_call_minutes=(uint32_t)request->rtc_after_reset.days*24u*60u
        +(uint32_t)request->rtc_after_reset.hours*60u+(uint32_t)request->rtc_after_reset.minutes+10u;
    runtime.host_actions=REMASTER_EMERALD_NEW_GAME_HOST_RTC_RESET
        | REMASTER_EMERALD_NEW_GAME_HOST_GLOBAL_RESET | REMASTER_EMERALD_NEW_GAME_HOST_TRUCK_INTRO;
    memcpy(out_save,workspace,sizeof *out_save);*out_runtime=runtime;
    return REMASTER_EMERALD_NEW_GAME_INITIALIZED;
}
