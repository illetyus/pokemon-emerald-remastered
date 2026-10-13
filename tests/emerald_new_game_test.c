#include "remaster/emerald_new_game.h"
#include "remaster/platform.h"
#include <stdio.h>
#include <string.h>

void ap04_source_bootstrap(uint32_t rng, uint16_t lower,
    const uint8_t name[8], uint8_t gender, uint8_t prefix[176], uint32_t *after);
static unsigned checks, failures, io;
static void expect(int ok, const char *message)
{
    ++checks;
    if (!ok) { ++failures; fprintf(stderr, "new game prepare: %s\n", message); }
}
static int forbidden_write(void *unused, const char *slot, const uint8_t *data, size_t size)
{ (void)unused; (void)slot; (void)data; (void)size; ++io; return 0; }
static RemasterSaveReadResult forbidden_read(void *unused, const char *slot,
    uint8_t *data, size_t capacity, size_t *size)
{ (void)unused; (void)slot; (void)data; (void)capacity; (void)size; ++io; return REMASTER_SAVE_READ_ERROR; }
int main(void)
{
    RemasterEmeraldNewGameRequest request;
    RemasterEmeraldNewGameDraft draft, sentinel;
    RemasterPlatformVTable platform;
    uint8_t reference[176], encoded[8], before[8];
    uint32_t after;
    unsigned gender, seed;
    const char *names[] = {"BRENDAN", "Cemal", "\xC3\x87\xC4\xB1\xC4\x9F", "MAY", "A B", "0123456", "!?"};
    memset(&platform, 0, sizeof platform);
    platform.save_write=forbidden_write; platform.save_read_result=forbidden_read;
    remaster_platform_install(&platform);
    memset(&request, 0, sizeof request);
    request.format=REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    memset(&sentinel, 0xA7, sizeof sentinel);
    for (gender=0;gender<2;++gender) for (seed=0;seed<64;++seed) {
        const char *name=names[seed % (sizeof names / sizeof names[0])];
        expect(remaster_emerald_new_game_encode_name(name, strlen(name), request.player_name)
            == REMASTER_EMERALD_NEW_GAME_PREPARED, "source keyboard UTF8 encoding");
        request.avatar=(uint8_t)gender;
        request.rng_state=seed*UINT32_C(0x13579BDF);
        request.trainer_id_lower=(uint16_t)(seed*1009u);
        expect(remaster_emerald_new_game_prepare(&request,&draft)
            == REMASTER_EMERALD_NEW_GAME_PREPARED, "explicit request prepared");
        ap04_source_bootstrap(request.rng_state,request.trainer_id_lower,
            request.player_name,request.avatar,reference,&after);
        expect(memcmp(draft.save_block2_prefix,reference,sizeof reference)==0,
            "byte projection agrees with compiled original default/trainer functions");
        expect(draft.rng_after_trainer_id==after, "original Random consumption agrees");
        expect(draft.pending_source_calls>0 && draft.stage==REMASTER_EMERALD_NEW_GAME_DRAFT,
            "preparation is an unfinished draft, never a playable save");
        expect(draft.map_group==25 && draft.map_num==40 && draft.player_x==2 && draft.player_y==2,
            "source truck destination is center, never Brendan house");
        expect(draft.warp_id==-1 && draft.warp_x==-1 && draft.warp_y==-1,
            "original warp sentinel retained separately from derived player coordinates");
    }
    memset(before,0xAC,sizeof before);
    {
        const char *bad[]={"", "        ", "12345678", "A\nB", "A$B", "A\xF0\x9F\x98\x80", "\xC0\xAF", "e\xCC\x81"};
        size_t i;
        for(i=0;i<sizeof bad/sizeof bad[0];++i) {
            memcpy(encoded,before,sizeof encoded);
            expect(remaster_emerald_new_game_encode_name(bad[i],strlen(bad[i]),encoded)
                == REMASTER_EMERALD_NEW_GAME_INVALID_NAME, "unsupported/control/oversize name rejected");
            expect(memcmp(encoded,before,sizeof encoded)==0, "invalid encoding leaves caller bytes unchanged");
        }
    }
    expect(remaster_emerald_new_game_encode_name("A\0B",3,encoded)
        == REMASTER_EMERALD_NEW_GAME_INVALID_NAME, "embedded NUL is not a prefix success");
    expect(remaster_emerald_new_game_encode_name(NULL,1,encoded)
        == REMASTER_EMERALD_NEW_GAME_INVALID_NAME, "null source rejected");
    expect(remaster_emerald_new_game_encode_name("A",1,NULL)
        == REMASTER_EMERALD_NEW_GAME_INVALID_NAME, "null output rejected");
    expect(remaster_emerald_new_game_encode_name("AB",29,encoded)
        == REMASTER_EMERALD_NEW_GAME_INVALID_NAME, "oversize input rejected before reading");
    expect(remaster_emerald_new_game_encode_name("A",1,request.player_name)
        == REMASTER_EMERALD_NEW_GAME_PREPARED, "reset valid name");
    {
        unsigned bad;
        for(bad=0;bad<4;++bad) {
            request.format=(RemasterEmeraldSaveFormat)(bad==3?99:bad);
            if(request.format==REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS) continue;
            draft=sentinel;
            expect(remaster_emerald_new_game_prepare(&request,&draft)
                == REMASTER_EMERALD_NEW_GAME_UNSUPPORTED_FORMAT,
                "new game supports canonical VP format only; import remains separate");
            expect(memcmp(&draft,&sentinel,sizeof draft)==0, "unsupported format preserves complete previous draft");
        }
    }
    request.format=REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    request.avatar=2;draft=sentinel;
    expect(remaster_emerald_new_game_prepare(&request,&draft)==REMASTER_EMERALD_NEW_GAME_INVALID_AVATAR,
        "unknown avatar cannot publish state");
    expect(memcmp(&draft,&sentinel,sizeof draft)==0, "bad avatar preserves previous draft");
    request.avatar=0;
    memset(request.player_name,0xBB,sizeof request.player_name);draft=sentinel;
    expect(remaster_emerald_new_game_prepare(&request,&draft)==REMASTER_EMERALD_NEW_GAME_INVALID_NAME,
        "missing EOS in fixed name rejected");
    expect(memcmp(&draft,&sentinel,sizeof draft)==0, "bad name preserves previous draft");
    request.player_name[0]=0xFC;request.player_name[1]=0xFF;
    expect(remaster_emerald_new_game_prepare(&request,&draft)==REMASTER_EMERALD_NEW_GAME_INVALID_NAME,
        "raw extended control code rejected");
    expect(remaster_emerald_new_game_prepare(NULL,&draft)==REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT,
        "null request rejected");
    expect(remaster_emerald_new_game_prepare(&request,NULL)==REMASTER_EMERALD_NEW_GAME_INVALID_ARGUMENT,
        "null draft rejected");
    expect(io==0, "draft preparation performs no storage IO or overwrite");
    remaster_platform_install(NULL);
    printf("AP04 source bootstrap: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
