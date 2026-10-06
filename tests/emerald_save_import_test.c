#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"
#include "remaster/emerald_quest.h"
#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_items.h"
#include "remaster/emerald_object_state.h"
#include "remaster/emerald_encounter.h"
#include <stdio.h>
#include <string.h>

/* Independently constructed source-layout bytes, not production accessors. */
static const uint8_t kKnownBoxPokemon[REMASTER_EMERALD_BOX_POKEMON_BYTES] = {
    0x11,0x00,0x00,0x00,0xD4,0xC3,0xB2,0xA1,
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,
    0x02,0x06,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0xA5,
    0x0B,0xC1,0xEF,0xBE,
    0xC4,0xC1,0xB1,0xA5,0xC0,0xC5,0xB5,0xA9,
    0xCC,0xC9,0xB9,0xAD,0xD7,0xF7,0x27,0x0B,
    0xE4,0x80,0xD7,0x26,0x81,0xF0,0x90,0xB0,
    0xE4,0xC3,0xE7,0xA1,0x90,0xC3,0x8B,0xA1,
    0xE6,0xE2,0xBD,0xB8,0xD0,0xC3,0xED,0xA1,
    0xBD,0x95,0x86,0xB3,0xCA,0x85,0x5D,0x1F
};
static unsigned checks, failures;
static unsigned char image[131072], before[131072];
static unsigned char sb1[0x3DC8], sb2[0xF44], storage[0x83D0];
static RemasterEmeraldSave save;
static void check(int ok, const char *what) {
    ++checks;
    if (!ok) { ++failures; fprintf(stderr, "import: %s\n", what); }
}
static void put16(unsigned char *p, unsigned v) {
    p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8);
}
static void put32(unsigned char *p, uint32_t v) {
    put16(p,v); put16(p+2,v>>16);
}
static uint16_t sum(const unsigned char *p, size_t n) {
    uint32_t v=0; size_t i;
    for(i=0;i<n;i+=4) v+=(uint32_t)p[i]|((uint32_t)p[i+1]<<8)
        |((uint32_t)p[i+2]<<16)|((uint32_t)p[i+3]<<24);
    return (uint16_t)((v>>16)+v);
}
#ifdef R17_BASELINE
/* Run actual pre-I2 decoder against the new import contract for RED evidence. */
static RemasterEmeraldSaveStatus baseline_decode(const uint8_t *p, size_t n,
    RemasterEmeraldSaveFormat format, RemasterEmeraldSave *out) {
    (void)format; return remaster_emerald_save_decode(p,n,out);
}
#define remaster_emerald_save_decode_format baseline_decode
#endif
static void flag(size_t base, unsigned id) { sb1[base+id/8]|=(unsigned char)(1u<<(id%8)); }
static void fixture(int stock, unsigned rotation, unsigned slot) {
    unsigned id,i; size_t flags=stock?0x1270:0x12B0, vars=stock?0x139C:0x13DC;
    size_t templates=stock?0xC70:0xCB0, stride=stock?0x24:0x28;
    memset(sb1,0,sizeof(sb1)); memset(sb2,0,sizeof(sb2)); memset(storage,0,sizeof(storage));
    put16(sb1,0xFFF9); put16(sb1+2,22);
    sb1[4]=0; sb1[5]=16; sb1[6]=0xFF; put16(sb1+8,12); put16(sb1+10,15);
    for(i=0;i<4;i++) { unsigned char *p=sb1+0xC+i*8;
        p[0]=(unsigned char)(i+1);p[1]=(unsigned char)(i+2);p[2]=0xFF;
        put16(p+4,100+i);put16(p+6,200+i); }
    put16(sb1+0x2C,0x1234);sb1[0x2E]=3;sb1[0x2F]=2;sb1[0x30]=1;put16(sb1+0x32,441);
    sb1[0x234]=6; sb1[0x235]=0xA1;sb1[0x236]=0xB2;sb1[0x237]=0xC3;
    for(i=0;i<6;i++) { unsigned char *p=sb1+0x238+i*100;
        memcpy(p,kKnownBoxPokemon,80);put32(p+80,8);p[84]=42;p[85]=0xFF;
        put16(p+86,73+i);put16(p+88,100);put16(p+90,81);put16(p+92,65);
        put16(p+94,91);put16(p+96,102);put16(p+98,77); }
    put32(sb2+0xAC,0x89ABCDEF);sb2[8]=1;
    put32(sb1+0x490,543210u^0x89ABCDEFu);put16(sb1+0x494,4321u^0xCDEFu);put16(sb1+0x496,259);
    /* PC quantities plaintext, five bag pockets encrypted with low16(key). */
    for(i=0;i<50;i++) { put16(sb1+0x498+i*4,13+i);put16(sb1+0x49A+i*4,20+i); }
    { const unsigned offsets[]={0x560,0x650,0x690,0x790,0x5D8};
      const unsigned sizes[]={30,16,64,46,30};
      for(i=0;i<5;i++) { unsigned j;for(j=0;j<sizes[i];j++) {
        put16(sb1+offsets[i]+j*4,100+i);put16(sb1+offsets[i]+j*4+2,(j+1)^0xCDEFu); } } }
    flag(flags,0x860);flag(flags,0x82);flag(flags,0x867);flag(flags,0x95F);
    for(i=0;i<256;i++) put16(sb1+vars+i*2,0x5500+i);
    for(i=0;i<16;i++) memset(sb1+0xA30+i*stride,(int)(0x80+i),stride);
    for(i=0;i<64;i++) { unsigned char *p=sb1+templates+i*24;
        p[0]=(unsigned char)(i+1);
        if(stock){p[1]=(unsigned char)(40+i);p[2]=0;p[3]=0xD9;}
        else {p[1]=0;put16(p+2,400+i);}
        put16(p+4,100+i);put16(p+6,200+i);p[8]=3;p[9]=7;p[10]=0x42;
        put16(p+12,1);put16(p+14,5);put32(p+16,0x08123456u+i);put16(p+20,0x100+i); }
    storage[0]=13;for(i=0;i<420;i++) memcpy(storage+4+i*80,kKnownBoxPokemon,80);
    memset(storage+0x8344,0xBB,126);memset(storage+0x83C2,0xCC,14);
    put16(sb2+0x98,0xFFF4);sb2[0x9A]=2;sb2[0x9B]=3;sb2[0x9C]=4;
    put16(sb2+0xA0,1234);sb2[0xA2]=5;sb2[0xA3]=6;sb2[0xA4]=7;
    if(!stock) memset(sb2+0xF2C,0x5A,24); /* follower bytes stay opaque */
    memset(image,0xFF,sizeof(image));
    for(id=0;id<14;id++) { const unsigned char *p;size_t n;
        unsigned char *s=image+(slot*14+(id+rotation)%14)*4096;
        if(id==0) {p=sb2;n=stock?0xF2C:0xF44;}
        else if(id<=4) {p=sb1+(id-1)*0xF80;n=id==4?(stock?0xF08:0xF48):0xF80;}
        else {p=storage+(id-5)*0xF80;n=id==13?0x7D0:0xF80;}
        memset(s,0,4096);memcpy(s,p,n);
        put16(s+4084,id);put16(s+4086,sum(p,n));put32(s+4088,0x08012025);put32(s+4092,101+slot);
    }
    memcpy(before,image,sizeof(image));
}
static void domains(int stock) {
    RemasterEmeraldOverworldState world;
    RemasterEmeraldWarpState warp;
    RemasterEmeraldPartyPokemon party;
    RemasterEmeraldBoxPokemon boxed;
    RemasterEmeraldObjectTemplate obj;
    RemasterEmeraldItemSlot item;
    const RemasterEmeraldQuestObjective *quest;
    RemasterEmeraldTime time;
    unsigned i,j;int valid,value;uint16_t var;uint8_t raw[40];
    size_t flags=stock?0x1270:0x12B0, vars=stock?0x139C:0x13DC;
    check(!memcmp(save.save_block1,sb1,stock?0x3D88:0x3DC8),"complete SB1 reconstruction");
    check(!memcmp(save.save_block2,sb2,stock?0xF2C:0xF44),"complete SB2 reconstruction");
    check(!memcmp(save.pokemon_storage,storage,0x83D0),"complete storage reconstruction");
    check(remaster_emerald_overworld_get(&save,&world)&&world.player_x==-7&&world.player_y==22
        &&world.map_group==0&&world.map_num==16&&world.warp_id==-1&&world.warp_x==12&&world.warp_y==15
        &&world.saved_music==0x1234&&world.weather==3&&world.weather_cycle_stage==2&&world.flash_level==1
        &&world.map_layout_id==441&&world.party_count==6&&world.money==543210&&world.coins==4321
        &&world.registered_item==259,"world and encrypted currency");
    check(remaster_emerald_continue_game_warp_get(&save,&warp)&&warp.map_group==1&&warp.x==100,"continue warp");
    check(remaster_emerald_dynamic_warp_get(&save,&warp)&&warp.map_group==2&&warp.x==101,"dynamic warp");
    check(remaster_emerald_last_heal_warp_get(&save,&warp)&&warp.map_group==3&&warp.x==102,"heal warp");
    check(remaster_emerald_escape_warp_get(&save,&warp)&&warp.map_group==4&&warp.x==103,"escape warp");
    for(i=1;i<0x960;i++) check(remaster_emerald_flag_get(&save,(uint16_t)i,&value)
        &&value==((sb1[flags+i/8]>>(i%8))&1),"persistent flag domain");
    for(i=0;i<256;i++) check(remaster_emerald_var_get(&save,(uint16_t)(0x4000+i),&var)&&var==0x5500+i,"var domain");
    quest=remaster_emerald_quest_active(&save);
    check(quest&&quest->id==REMASTER_EMERALD_QUEST_RETURN_TO_BIRCH,"objective derived from imported flags");
    check(remaster_emerald_party_count(&save)==6,"u8 party count ignores nonzero padding");
    for(i=0;i<6;i++) check(remaster_emerald_party_get(&save,i,&party,&valid)&&valid&&party.hp==73+i
        &&party.level==42&&party.status==8&&party.mail==255,"party checksum and extensions");
    check(remaster_emerald_storage_current_box(&save)==13,"current PC box");
    for(i=0;i<14;i++) for(j=0;j<30;j++) check(remaster_emerald_storage_get(&save,i,j,&boxed,&valid)
        &&valid&&boxed.personality==17&&boxed.ot_id==0xA1B2C3D4,"all 420 boxed Pokemon");
    { const unsigned sizes[]={30,16,64,46,30};
      for(i=0;i<5;i++) for(j=0;j<sizes[i];j++) check(remaster_emerald_bag_slot_get(&save,(uint8_t)(i+1),j,&item)
        &&item.item_id==100+i&&item.quantity==j+1,"all five encrypted bag pockets"); }
    time=remaster_emerald_save_get_local_time_offset(&save);
    check(time.days==-12&&time.hours==2&&time.minutes==3&&time.seconds==4,"signed RTC offset");
    time=remaster_emerald_save_get_last_berry_update(&save);
    check(time.days==1234&&time.hours==5&&time.minutes==6&&time.seconds==7,"berry RTC update");
    for(i=0;i<64;i++) check(remaster_emerald_object_template_get(&save,i,&obj)&&obj.local_id==i+1
        &&obj.graphics_id==(stock?40:400)+i&&obj.kind==0&&obj.x==(int)(100+i)&&obj.y==(int)(200+i)
        &&obj.movement_range_x==2&&obj.movement_range_y==4&&obj.trainer_type==1
        &&obj.legacy_script_address==0x08123456u+i&&obj.flag_id==0x100+i,"source NPC templates");
    for(i=0;i<16;i++) { memset(raw,0xFF,40);
        check(remaster_emerald_saved_object_event_read(&save,i,raw)
            &&raw[0]==0x80+i&&raw[(stock?36:40)-1]==0x80+i,"source live object stride");
        if(stock) check(raw[36]==0&&raw[39]==0,"stock opaque record zero padding"); }
    check(!memcmp(before,image,sizeof(image)),"import does not mutate physical image");
    check(remaster_emerald_object_template_get(&save,63,&obj),"last template read");
    obj.x=-25;obj.y=31;
    check(remaster_emerald_object_template_set(&save,63,&obj)
        &&save.save_block1[(stock?0xC70:0xCB0)+63*24+4]==0xE7,"template mutation source offset");
    if(stock) {
        obj.graphics_id=256;
        check(!remaster_emerald_object_template_set(&save,63,&obj),"stock graphics width cannot truncate");
    }
    memset(raw,0x75,sizeof(raw));
    check(remaster_emerald_saved_object_event_write(&save,15,raw)
        &&save.save_block1[0xA30+15*(stock?36:40)]==0x75
        &&save.save_block1[stock?0xC70:0xCB0]==1,"last opaque record write preserves first template");
    check(remaster_emerald_flag_set(&save,0x861,1)&&save.save_block1[flags+0x861/8]==0x83,"progression writes selected source flag range");
    check(remaster_emerald_flag_set(&save,0x867,0)&&remaster_emerald_var_set(&save,0x4085,0),"stage early story conditions from populated fixture");
    quest=remaster_emerald_quest_active(&save);
    check(quest&&quest->id==REMASTER_EMERALD_QUEST_VISIT_PETALBURG_GYM,"objective follows imported progression mutation");
    check(remaster_emerald_var_set(&save,0x40FF,0xABCD)&&save.save_block1[vars+510]==0xCD
        &&save.save_block1[vars+511]==0xAB,"var mutation uses selected layout");
    check(remaster_emerald_party_set_count(&save,5)&&save.save_block1[0x235]==0xA1
        &&save.save_block1[0x236]==0xB2&&save.save_block1[0x237]==0xC3,"party count write preserves padding");
}
#ifndef R17_NO_ENCOUNTERS
static void encounters(int stock) {
    RemasterEmeraldEncounterRuntime runtime;RemasterEmeraldEncounterResult result;
    size_t offset=stock?0x2B90:0x2BD0,roamer=stock?0x31DC:0x321C;
    unsigned char *p=save.save_block1+offset;
    /* The populated var-domain fixture has Repel active: disable it for the
     * lower-level special encounter checks, independently of production readers. */
    put16(save.save_block1+(stock?0x139C:0x13DC)+0x21*2,0);
    put16(p,277);p[2]=16;p[3]=0;p[4]=12;put16(p+8,1);put16(p+10,43);p[17]=100;
    remaster_emerald_encounter_runtime_init(&runtime,7);
    check(remaster_emerald_encounter_generate_after_rate(&runtime,&save,0,16,
        REMASTER_EMERALD_ENCOUNTER_AREA_LAND,REMASTER_EMERALD_ROD_NONE,&result)&&result.occurred
        &&result.kind==REMASTER_EMERALD_ENCOUNTER_OUTBREAK&&result.species==277&&result.level==12,"source outbreak domain");
    memset(p,0,20);p=save.save_block1+roamer;
    put32(p,0x12345678);put32(p+4,0x89ABCDEF);put16(p+8,407);put16(p+10,100);p[12]=40;p[19]=1;
    remaster_emerald_encounter_runtime_init(&runtime,0);remaster_emerald_encounter_roamer_set_location(&runtime,0,16);
    check(remaster_emerald_encounter_generate_after_rate(&runtime,&save,0,16,
        REMASTER_EMERALD_ENCOUNTER_AREA_LAND,REMASTER_EMERALD_ROD_NONE,&result)&&result.occurred
        &&result.kind==REMASTER_EMERALD_ENCOUNTER_ROAMER&&result.species==407&&result.level==40
        &&result.pokemon.hp==100&&result.pokemon.box.personality==0x89ABCDEF,"source roamer domain");
}
#endif
int main(void) {
    unsigned stock,rotation,slot;
    for(stock=0;stock<2;stock++) for(slot=0;slot<2;slot++) for(rotation=0;rotation<14;rotation++) {
        RemasterEmeraldSaveFormat format=stock?REMASTER_EMERALD_SAVE_FORMAT_STOCK:REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
        fixture((int)stock,rotation,slot);
        check(remaster_emerald_save_decode_format(image,sizeof(image),format,&save)==REMASTER_EMERALD_SAVE_OK,"explicit format import");
        check(save.counter==101+slot&&save.selected_slot==slot&&save.last_written_sector==rotation,"selected slot metadata");
        domains((int)stock);
#ifndef R17_NO_ENCOUNTERS
        encounters((int)stock);
#endif
    }
    /* Failed import clears reconstructed state, never retains a previous save. */
    memset(&save,0xA5,sizeof(save));
    check(remaster_emerald_save_decode_format(image,sizeof(image)-1,
        REMASTER_EMERALD_SAVE_FORMAT_STOCK,&save)==REMASTER_EMERALD_SAVE_CORRUPT
        &&save.counter==0&&save.save_block1[0]==0,"short import clears previous domain");
    memset(&save,0xA5,sizeof(save));
    check(remaster_emerald_save_decode_format(image,sizeof(image),(RemasterEmeraldSaveFormat)99,
        &save)==REMASTER_EMERALD_SAVE_CORRUPT&&save.save_block2[0]==0,"unknown format cannot reconstruct");
    check(remaster_emerald_save_decode_format(image,sizeof(image),
        REMASTER_EMERALD_SAVE_FORMAT_STOCK,0)==REMASTER_EMERALD_SAVE_CORRUPT,"null output");
    fixture(1,0,0);
    check(remaster_emerald_save_decode_format(image,sizeof(image),
        REMASTER_EMERALD_SAVE_FORMAT_STOCK,&save)==REMASTER_EMERALD_SAVE_OK,"stock export setup");
    check(remaster_emerald_save_encode_next(image,sizeof(image),&save)
        &&save.counter==102,"stock import exports in source format");
    check(remaster_emerald_save_decode_format(image,sizeof(image),
        REMASTER_EMERALD_SAVE_FORMAT_STOCK,&save)==REMASTER_EMERALD_SAVE_OK
        &&save.source_is_stock&&save.counter==102
        &&!memcmp(save.save_block1,sb1,0x3D88)
        &&!memcmp(save.save_block2,sb2,0xF2C)
        &&!memcmp(save.pokemon_storage,storage,0x83D0),"stock domain bytes reimport after export");
    printf("R17 domain import: %u checks, %u failures.\n",checks,failures);
    return failures?1:0;
}
