#include "remaster/emerald_new_game.h"
#include "remaster/platform.h"
#include <stdio.h>
#include <string.h>
#include "full_domains.h"

void ap04_source_init_domains(uint32_t rng,uint16_t lower,uint16_t trends[5][6],
    uint16_t lottery[2],uint16_t lady[16],uint8_t *apprentice,uint32_t *after,uint32_t *calls);
void ap04_source_mail(uint16_t words[9],uint8_t name[8],uint16_t *species);
static RemasterEmeraldSave work,save,previous,decoded,masked;
static uint8_t image[REMASTER_EMERALD_SAVE_IMAGE_BYTES],disk[REMASTER_EMERALD_SAVE_IMAGE_BYTES];
static unsigned checks,failures,reads,writes;
static RemasterSaveReadResult read_result=REMASTER_SAVE_READ_MISSING;
static void expect(int ok,const char *message)
{
    ++checks;
    if(!ok) { if(failures<20)fprintf(stderr,"new game init: %s\n",message);++failures; }
}
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0]|((uint16_t)p[1]<<8)); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)u16(p)|((uint32_t)u16(p+2)<<16); }
static uint64_t hash(const uint8_t *p,size_t n)
{
    uint64_t h=UINT64_C(14695981039346656037);size_t i;
    for(i=0;i<n;++i)h=(h^p[i])*UINT64_C(1099511628211);
    return h;
}
static RemasterSaveReadResult read_slot(void *unused,const char *slot,uint8_t *p,size_t n,size_t *size)
{
    (void)unused;(void)slot;++reads;
    if(read_result==REMASTER_SAVE_READ_OK) {
        if(n<sizeof disk)return REMASTER_SAVE_READ_ERROR;
        memcpy(p,disk,sizeof disk);*size=sizeof disk;
    }
    return read_result;
}
static int write_slot(void *unused,const char *slot,const uint8_t *p,size_t n)
{
    (void)unused;(void)slot;++writes;
    if(n!=sizeof disk)return 0;
    memcpy(disk,p,n);read_result=REMASTER_SAVE_READ_OK;return 1;
}
static void check_fixed(void)
{
    masked=save;
    memset(masked.save_block2,0,176);masked.save_block2[F_SB2_PLAYERAPPRENTICE]=0;
    memset(masked.save_block1+F_SB1_POS,0,4);memset(masked.save_block1+F_SB1_LOCATION,0,8);
    memset(masked.save_block1+F_SB1_MAPLAYOUTID,0,2);memset(masked.save_block1+F_SB1_OLDMAN,0,64);
    memset(masked.save_block1+F_SB1_DEWFORDTRENDS,0,40);memset(masked.save_block1+F_SB1_LILYCOVELADY,0,64);
    memset(masked.save_block1+F_SB1_VARS+2*(16459-16384),0,4);
    expect(hash(masked.save_block2,sizeof masked.save_block2)==fixed_hashes[0],"all fixed SB2 bytes match pinned data");
    expect(hash(masked.save_block1,sizeof masked.save_block1)==fixed_hashes[1],"all fixed SB1 bytes match pinned data");
    expect(hash(masked.pokemon_storage,sizeof masked.pokemon_storage)==fixed_hashes[2],"all storage bytes match pinned data");
}
static void rejected(RemasterEmeraldNewGameInitRequest *request,RemasterEmeraldNewGameResult wanted)
{
    RemasterEmeraldNewGameRuntime runtime,before;
    memset(&save,0xA7,sizeof save);previous=save;memset(&before,0xAC,sizeof before);runtime=before;
    expect(remaster_emerald_new_game_initialize(request,&work,&save,&runtime)==wanted,"invalid request rejected");
    expect(memcmp(&save,&previous,sizeof save)==0,"failed initialization preserves whole previous save");
    expect(memcmp(&runtime,&before,sizeof runtime)==0,"failed initialization preserves runtime");
}
int main(void)
{
    RemasterEmeraldNewGameInitRequest request,valid;
    RemasterEmeraldNewGameRuntime runtime,again;
    RemasterPlatformVTable platform;
    unsigned seed,lower,i,j,domains=0;
    uint16_t trends[5][6],lottery[2],lady[16],mail_words[9],mail_species;
    uint8_t apprentice,mail_name[8];uint32_t after,calls;
    memset(&platform,0,sizeof platform);platform.save_read_result=read_slot;platform.save_write=write_slot;
    remaster_platform_install(&platform);
    memset(&valid,0,sizeof valid);valid.identity.format=REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    valid.observed_save_status=REMASTER_EMERALD_SAVE_EMPTY;
    valid.ranking_id_policy=REMASTER_EMERALD_NEW_GAME_SOURCE_UBFIX_ZERO_ID;
    valid.rtc_after_reset.days=3;valid.rtc_after_reset.hours=23;valid.rtc_after_reset.minutes=59;valid.rtc_after_reset.seconds=59;
    expect(remaster_emerald_new_game_encode_name("Cemal",5,valid.identity.player_name)==REMASTER_EMERALD_NEW_GAME_PREPARED,"valid name");
    for(seed=0;seed<256;++seed)for(lower=0;lower<30;++lower) {
        const uint8_t *p;
        request=valid;request.identity.rng_state=seed*UINT32_C(0x13579BDF);
        request.identity.trainer_id_lower=(uint16_t)(seed*30+lower);request.identity.avatar=(uint8_t)(seed&1);
        memset(&work,0xA7,sizeof work);memset(&save,0xAC,sizeof save);
        expect(remaster_emerald_new_game_initialize(&request,&work,&save,&runtime)==REMASTER_EMERALD_NEW_GAME_INITIALIZED,"persistent initialization succeeds");
        ++domains;
        ap04_source_init_domains(request.identity.rng_state,request.identity.trainer_id_lower,trends,lottery,lady,&apprentice,&after,&calls);
        expect(runtime.rng_state==after && runtime.rng_calls==calls,"RNG final state and call count agree with original C");
        for(i=0;i<5;++i) {
            uint16_t packed;p=save.save_block1+F_SB1_DEWFORDTRENDS+i*F_TREND_BYTES;packed=u16(p);
            expect((packed&127)==trends[i][0] && ((packed>>7)&127)==trends[i][1] && ((packed>>14)&1)==trends[i][2],"original trend order and production bits");
            expect(u16(p+F_TREND_RAND)==trends[i][3] && u16(p+F_TREND_WORDS)==trends[i][4] && u16(p+F_TREND_WORDS+2)==trends[i][5],"original trend words and random value");
        }
        expect(u16(save.save_block1+F_SB1_VARS+2*(16459-16384))==lottery[0] && u16(save.save_block1+F_SB1_VARS+2*(16460-16384))==lottery[1],"original lottery order");
        p=save.save_block1+F_SB1_LILYCOVELADY;
        expect(p[0]==lady[0],"original Lilycove family");
        if(lady[0]==0) {
            for(i=0;i<9;++i)expect(u16(p+F_QUIZ_QUESTION+2*i)==lady[i+1],"original quiz words");
            expect(u16(p+F_QUIZ_CORRECTANSWER)==lady[10] && u16(p+F_QUIZ_PRIZE)==lady[11] && p[F_QUIZ_QUESTIONID]==lady[12],"original quiz selection");
            expect(u16(p+F_QUIZ_PLAYERANSWER)==65535 && p[F_QUIZ_PREVQUESTIONID]==16 && p[F_QUIZ_PLAYERNAME]==255 && p[F_QUIZ_LANGUAGE]==2,"quiz reset sentinels");
        } else if(lady[0]==1)expect(p[F_FAVOR_FAVORID]==lady[1] && u16(p+F_FAVOR_BESTITEM)==lady[2] && p[F_FAVOR_PLAYERNAME]==255 && p[F_FAVOR_LANGUAGE]==2,"original favor selection");
        else expect(p[F_CONTEST_LADY_CATEGORY]==lady[1] && p[F_CONTEST_LADY_PLAYERNAME]==255 && p[F_CONTEST_LADY_LANGUAGE]==2,"original contest selection");
        expect(save.save_block2[F_SB2_PLAYERAPPRENTICE]==apprentice,"original apprentice selection");
        expect(save.save_block1[F_SB1_OLDMAN]==(request.identity.trainer_id_lower%10)/2,"original old-man family");
        expect(save.status==REMASTER_EMERALD_SAVE_EMPTY && !save.counter && !save.last_written_sector && !save.selected_slot && !save.source_is_stock,"deliberate initial checkpoint metadata");
        expect(u16(save.save_block1+F_SB1_POS)==2 && u16(save.save_block1+F_SB1_POS+2)==2 && save.save_block1[F_SB1_LOCATION]==25 && save.save_block1[F_SB1_LOCATION+1]==40 && u16(save.save_block1+F_SB1_MAPLAYOUTID)==237,"source truck placement");
        expect(runtime.match_call_minutes==5769 && runtime.host_actions==7,"clock observation and explicit pending host actions");
        if(lower==0)check_fixed();
    }
    expect(reads==0 && writes==0,"initialization performs no platform IO");
    request=valid;request.identity.rng_state=UINT32_MAX;request.identity.trainer_id_lower=UINT16_MAX;
    expect(remaster_emerald_new_game_initialize(&request,&work,&save,&runtime)==REMASTER_EMERALD_NEW_GAME_INITIALIZED,"maximum seed and timer");
    previous=save;
    expect(remaster_emerald_new_game_initialize(&request,&work,&save,&again)==REMASTER_EMERALD_NEW_GAME_INITIALIZED && memcmp(&save,&previous,sizeof save)==0 && memcmp(&again,&runtime,sizeof runtime)==0,"explicit replay is deterministic");
    request.rtc_after_reset.days=0;request.rtc_after_reset.hours=0;request.rtc_after_reset.minutes=0;
    expect(remaster_emerald_new_game_initialize(&request,&work,&save,&again)==REMASTER_EMERALD_NEW_GAME_INITIALIZED && memcmp(&save,&previous,sizeof save)==0 && again.match_call_minutes==10,"host clock changes runtime only");
    ap04_source_mail(mail_words,mail_name,&mail_species);
    for(i=0;i<16;++i) {
        const uint8_t *p=save.save_block1+F_SB1_MAIL+i*F_MAIL_BYTES;
        for(j=0;j<9;++j)expect(u16(p+F_MAIL_WORDS+2*j)==mail_words[j],"all mail words match original ClearMail");
        expect(memcmp(p+F_MAIL_PLAYERNAME,mail_name,8)==0 && u16(p+F_MAIL_SPECIES)==mail_species && !u16(p+F_MAIL_ITEMID) && !u32(p+F_MAIL_TRAINERID),"all mail fields match original ClearMail");
    }
    expect(u32(save.save_block1+F_SB1_MONEY)==3000 && u16(save.save_block1+F_SB1_PCITEMS)==13 && u16(save.save_block1+F_SB1_PCITEMS+2)==1,"money and PC potion");
    for(i=0;i<14;++i)expect(save.pokemon_storage[F_STORAGE_BOXWALLPAPERS+i]==i%4,"source four default wallpaper cycle");
    expect(!save.save_block1[F_SB1_PLAYERPARTYCOUNT] && !u32(save.save_block2+F_SB2_ENCRYPTIONKEY),"empty party and source key");
    read_result=REMASTER_SAVE_READ_ERROR;previous=save;
    expect(!remaster_emerald_save_store_platform("new",image,sizeof image,&save) && !writes && memcmp(&save,&previous,sizeof save)==0,"IO error cannot overwrite or mutate new checkpoint");
    read_result=REMASTER_SAVE_READ_MISSING;
    expect(remaster_emerald_save_store_platform("new",image,sizeof image,&save) && writes==1,"independent typed MISSING permits initial save");
    expect(remaster_emerald_save_decode(disk,sizeof disk,&decoded)==REMASTER_EMERALD_SAVE_OK,"new image decodes");
    expect(memcmp(decoded.save_block2,save.save_block2,sizeof save.save_block2)==0 && memcmp(decoded.save_block1,save.save_block1,sizeof save.save_block1)==0 && memcmp(decoded.pokemon_storage,save.pokemon_storage,sizeof save.pokemon_storage)==0,"all three blocks survive store and reopen");
    previous=save;
    expect(remaster_emerald_new_game_initialize(&valid,&work,&save,&runtime)==REMASTER_EMERALD_NEW_GAME_INITIALIZED,"prepare another new checkpoint");
    expect(!remaster_emerald_save_store_platform("new",image,sizeof image,&save) && writes==1,"found existing image blocks new overwrite");
    expect(memcmp(disk,image,sizeof disk)==0,"found image kept unchanged");
    for(i=1;i<7;++i) { request=valid;request.observed_save_status=(RemasterEmeraldSaveStatus)i;rejected(&request,REMASTER_EMERALD_NEW_GAME_NOT_EMPTY); }
    request=valid;request.ranking_id_policy=REMASTER_EMERALD_NEW_GAME_RANKING_POLICY_UNSPECIFIED;rejected(&request,REMASTER_EMERALD_NEW_GAME_INVALID_POLICY);
    request=valid;request.rtc_after_reset.days=-1;rejected(&request,REMASTER_EMERALD_NEW_GAME_INVALID_CLOCK);
    request=valid;request.rtc_after_reset.hours=24;rejected(&request,REMASTER_EMERALD_NEW_GAME_INVALID_CLOCK);
    request=valid;request.rtc_after_reset.minutes=-1;rejected(&request,REMASTER_EMERALD_NEW_GAME_INVALID_CLOCK);
    request=valid;request.rtc_after_reset.seconds=60;rejected(&request,REMASTER_EMERALD_NEW_GAME_INVALID_CLOCK);
    request=valid;request.identity.format=REMASTER_EMERALD_SAVE_FORMAT_STOCK;rejected(&request,REMASTER_EMERALD_NEW_GAME_UNSUPPORTED_FORMAT);
    request=valid;request.identity.avatar=2;rejected(&request,REMASTER_EMERALD_NEW_GAME_INVALID_AVATAR);
    request=valid;memset(request.identity.player_name,0xFC,8);rejected(&request,REMASTER_EMERALD_NEW_GAME_INVALID_NAME);
    rejected(NULL,REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT);
    expect(remaster_emerald_new_game_initialize(&valid,&save,&save,&runtime)==REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT,"aliased workspace rejected");
    expect(remaster_emerald_new_game_initialize(&valid,&work,&save,(RemasterEmeraldNewGameRuntime *)&save)==REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT,"overlapping publications rejected");
    remaster_platform_install(NULL);
    printf("AP04 persistent initialization: %u original-C domain cases, %u checks, %u failures\n",domains,checks,failures);
    return failures?1:0;
}
