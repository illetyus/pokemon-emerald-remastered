#include "RemasterBattleRead.h"
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace RemasterBattlePresentation;
namespace {
unsigned checks = 0;
void Check(bool value, const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
void Drain(Feed& feed) {
    Cue cue;
    while (feed.Next(cue)) {
        Check(!feed.Complete(cue.epoch + 1, cue.token), "wrong epoch callback");
        Check(!feed.Complete(cue.epoch, cue.token + 1), "wrong token callback");
        Check(feed.Complete(cue.epoch, cue.token), "consume cue");
        Check(!feed.Complete(cue.epoch, cue.token), "duplicate completion");
    }
}
RemasterEmeraldPartyPokemon Mon(uint16_t species, uint8_t level) {
    RemasterEmeraldPartyPokemon mon{}; RemasterEmeraldCalculatedStats stats{};
    const uint8_t ivs[6] = {31,31,31,31,31,31}, evs[6]{};
    mon.box.personality = species * 25u + 3u; mon.box.ot_id = 0x12345678;
    Check(remaster_emerald_box_pokemon_set_species(&mon.box, species), "fixture species");
    Check(remaster_emerald_box_pokemon_set_experience(&mon.box,
        remaster_emerald_experience_for_level(remaster_emerald_species_info(species)->growth_rate, level)), "fixture exp");
    for (unsigned i=0;i<6;++i) Check(remaster_emerald_box_pokemon_set_iv(&mon.box,i,31), "fixture iv");
    Check(remaster_emerald_box_pokemon_set_move(&mon.box,0,1,35), "fixture pound");
    Check(remaster_emerald_calculate_stats(species,level,remaster_emerald_box_pokemon_nature(&mon.box),ivs,evs,&stats), "fixture stats");
    mon.hp=mon.max_hp=stats.hp; mon.attack=stats.attack; mon.defense=stats.defense;
    mon.speed=stats.speed; mon.sp_attack=stats.sp_attack; mon.sp_defense=stats.sp_defense; mon.level=level;
    mon.box.checksum=remaster_emerald_box_pokemon_checksum(&mon.box); return mon;
}
void RealBattle(unsigned flags, bool cancelVisuals) {
    const auto player=Mon(4,50), opponent=Mon(1,5);
    RemasterEmeraldPartyPokemon players[2]={player,player}, opponents[2]={opponent,opponent};
    RemasterEmeraldBattleState fast{}, slow{};
    remaster_emerald_battle_state_init(&fast,flags,20261007);
    Check(remaster_emerald_battle_start(&fast,players, flags?2:1, opponents,flags?2:1), "start actual R13 battle");
    slow=fast; Feed immediate, delayed; Check(immediate.Begin(1)&&delayed.Begin(1), "begin timing scenarios");
    uint64_t serial=1;
    Check(immediate.Submit(1,serial,fast)&&delayed.Submit(1,serial,slow), "start batch"); Drain(immediate);
    for (unsigned turn=0;turn<20 && !fast.ended;++turn) {
        remaster_emerald_battle_clear_events(&fast); remaster_emerald_battle_clear_events(&slow);
        RemasterEmeraldBattleAction actions[4]{};
        for (unsigned i=0;i<4;++i) if (fast.battlers[i].active && !fast.battlers[i].fainted) {
            actions[i].kind=REMASTER_EMERALD_BATTLE_ACTION_MOVE; actions[i].move_slot=0;
            actions[i].target=fast.battlers[i].side==0?1:0;
        }
        Check(remaster_emerald_battle_resolve_turn(&fast,actions), "fast core turn");
        Check(remaster_emerald_battle_resolve_turn(&slow,actions), "slow core turn");
        const auto before=fast; ++serial;
        Check(immediate.Submit(1,serial,fast)&&delayed.Submit(1,serial,slow), "turn event capture");
        Drain(immediate);
        if (cancelVisuals) delayed.ClearPresentation();
        Check(std::memcmp(&before,&fast,sizeof fast)==0, "presentation cannot change any core byte");
        Check(std::memcmp(&fast,&slow,sizeof fast)==0, "animation timing cannot change outcomes or RNG");
    }
    Check(fast.ended, "actual fixture reaches authoritative end");
    const auto before=slow; Drain(delayed);
    Check(std::memcmp(&before,&slow,sizeof slow)==0, "post-battle visual draining immutable");
    Check(immediate.Latest().ended && delayed.Latest().ended, "core outcome snapshot");
}
}
int main() try {
    std::set<unsigned> identities, dex;
    for (const auto& entry:SpeciesCatalog) {
        identities.insert(entry.coreSpecies); dex.insert(entry.nationalDex);
        Check(ResolveSpecies(entry.coreSpecies)==&entry,"source species resolution");
        Check(AssetKey(entry.coreSpecies,0x12345678,0,0).find("pokemon."+std::to_string(entry.nationalDex)+".")==0,"exact dex identity");
    }
    Check(identities.size()==386 && dex.size()==386 && *dex.begin()==1 && *dex.rbegin()==386,"complete unique dex");
    Check(ResolveSpecies(277)->nationalDex==252 && ResolveSpecies(411)->nationalDex==358,"noncontiguous Emerald IDs");
    for (unsigned id=252;id<277;++id) Check(!ResolveSpecies(id),"old Unown placeholders rejected");
    for (unsigned id:{0u,412u,440u,65535u}) Check(!ResolveSpecies(id),"unsupported ID fallback");
    for (unsigned i=0;i<28;++i) {
        const unsigned p=(i&3)|((i&12)<<6)|((i&48)<<12)|((i&192)<<18);
        Check(Form(201,p,0)=="unown."+std::to_string(i),"personality Unown form");
        if (i) Check(Form(412+i,0,0)=="unown."+std::to_string(i),"extended graphics form");
    }
    Check(Form(410,0,0)=="speed","Emerald Deoxys");
    Check(Form(385,0,10)=="sunny","Castform uses actual fire type"); // source ID 385
    Check(Form(385,0,11)=="rainy" && Form(385,0,15)=="snowy" && Form(385,0,0)=="normal","Castform forms");
    Check(Form(308,42,0)=="spots.42","Spinda personality");
    Check(Shiny(0,7)&&!Shiny(0,8)&&Shiny(0x12341234,0),"source shiny threshold");
    RemasterEmeraldBattleState core{}; Snapshot snapshot;
    core.battle_type_flags=REMASTER_EMERALD_BATTLE_TYPE_DOUBLE;
    for (unsigned i=0;i<4;++i) { auto& mon=core.battlers[i]; mon.active=1; mon.side=i%2; mon.species=1+i; mon.pokemon.hp=10; mon.pokemon.max_hp=20; }
    const auto before=core;
    Check(Read(core,snapshot),"double roster");
    Check(snapshot.battlers[0].position.x==-180 && snapshot.battlers[1].position.x==180 && snapshot.battlers[2].position.y==90,"side/slot positions");
    Check(snapshot.battlers[0].position.y==-90 && snapshot.battlers[1].position.y==-90,"double slot zero");
    Check(std::memcmp(&before,&core,sizeof core)==0,"snapshot immutable");
    core.battlers[0].side=2; Check(!Read(core,snapshot),"bad side"); core=before;
    core.battlers[0].pokemon.hp=21; Check(!Read(core,snapshot),"invalid HP"); core=before;
    core.battle_type_flags=0; Check(!Read(core,snapshot),"excess single roster"); core=before;
    core.battlers[2].active=core.battlers[3].active=0; core.battle_type_flags=0;
    Check(Read(core,snapshot)&&snapshot.battlers[0].position.y==0,"single position");
    core.battlers[0].species=410; Check(Read(core,snapshot)&&snapshot.battlers[0].form=="speed","current transformed species, not party box");
    core.battle_type_flags=REMASTER_EMERALD_BATTLE_TYPE_TRAINER;
    for (unsigned id=1;id<2000;++id) if (const auto* trainer=remaster_emerald_battle_trainer_find(id)) {
        core.opponent_trainer_id=static_cast<uint16_t>(id);
        Check(Read(core,snapshot)&&snapshot.trainerPic==trainer->trainer_pic,"actual trainer pic");
        Check(snapshot.trainerIdentity==TrainerIdentity(trainer->trainer_pic),"exact trainer battle binding"); break;
    }
    core.opponent_trainer_id=65535; Check(Read(core,snapshot)&&snapshot.trainerIdentity=="trainer.unsupported","missing trainer fallback");
    for (unsigned kind=1;kind<=25;++kind) {
        RemasterEmeraldBattleEvent event{}; event.kind=static_cast<uint16_t>(kind);event.battler=0;event.target=1;event.move_id=52;event.value=-42;event.aux=17;
        const auto cue=MakeCue(event,snapshot);
        Check(std::memcmp(&event,&cue.event,sizeof event)==0,"event payload preserved");
        Check(cue.identity!="battle.unsupported" && cue.vfxIdentity.find("vfx.")==0,"all canonical event/VFX semantics");
        const auto camera=Camera(cue); Check(std::isfinite(camera.target.z)&&camera.fov>0,"finite camera");
    }
    RemasterEmeraldBattleEvent damage{}; damage.kind=REMASTER_EMERALD_BATTLE_EVENT_DAMAGE;damage.target=1;damage.aux=1;
    Check(MakeCue(damage,snapshot).actor==-1,"substitute does not hit actual model");
    damage.aux=0; Check(MakeCue(damage,snapshot).actor==1,"actual target hit");
    damage.kind=60000; Check(MakeCue(damage,snapshot).identity=="battle.unsupported","unknown event visible fallback");
    Feed feed; Check(!feed.Begin(0)&&feed.Begin(10)&&!feed.Begin(10),"strict epoch");
    core.event_count=256; for (auto& event:core.events) event.kind=REMASTER_EMERALD_BATTLE_EVENT_MESSAGE;
    for (unsigned batch=1;batch<=4;++batch) Check(feed.Submit(10,batch,core),"capacity accepted");
    Check(feed.Pending()==1024 && !feed.Submit(10,5,core)&&feed.Pending()==1024,"atomic capacity rejection");
    Check(!feed.Submit(9,5,core)&&!feed.Submit(10,6,core),"stale/out-of-order batches");
    Cue old; Check(feed.Next(old),"active token");
    Check(feed.Resync(10,5,core)&&feed.DroppedVisualCues()==1024&&feed.Pending()==0,"explicit discontinuity counted");
    Check(!feed.Complete(old.epoch,old.token)&&!feed.Resync(10,5,core),"old callback rejected after resync");
    core.event_count=1; Check(feed.Submit(10,6,core),"retry next serial after resync");
    Cue newCue; Check(feed.Next(newCue)&&newCue.token!=old.token,"no token reuse after resync");
    Check(feed.Begin(11)&&!feed.Complete(newCue.epoch,newCue.token),"world/session epoch invalidation");
    core.event_count=257; Check(!feed.Submit(11,1,core),"invalid core event capacity"); core.event_count=1;
    Check(!feed.Submit(11,1,core,2)&&feed.Submit(11,1,core,1)&&feed.Pending()==0,"bounded suffix capture");
    Check(FallbackOffset(Motion::Attack,.5,0).x>0 && FallbackOffset(Motion::Attack,.5,1).x<0,"side-relative fallback");
    Check(FallbackOffset(Motion::Faint,2,0).z==-60,"visual progress clamped");
    RealBattle(0,false); RealBattle(0,true); RealBattle(REMASTER_EMERALD_BATTLE_TYPE_DOUBLE,false);
    std::cout << checks << " R14 native identity, queue and authority checks passed\n"; return 0;
} catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
