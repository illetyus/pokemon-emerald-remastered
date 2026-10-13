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
