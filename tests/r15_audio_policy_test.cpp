#include "RemasterAudioRead.h"
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace RemasterAudio;
unsigned checks=0;
void Check(bool v,const char* why) { ++checks;if (!v) throw std::runtime_error(why); }
int main() try {
    Request r;
    for (unsigned id=1;id<610;++id) Check(Song(id,r)&&r.identity=="song."+std::to_string(id)&&!r.stopMusic,"song coverage");
    Check(Song(0,r)&&r.stopMusic,"silence");Check(Song(65535,r)&&r.stopMusic,"MUS_NONE");
    Check(!Song(610,r)&&!Song(65536,r),"invalid song");
    std::set<unsigned> dex;
    for (const auto& c:Cries) {
        dex.insert(c.nationalDex);
        for (unsigned m=0;m<13;++m) Check(Cry(c.coreSpecies,m,r)&&r.identity=="cry."+std::to_string(c.nationalDex)+".mode."+std::to_string(m),"cry modes");
    }
    Check(dex.size()==386&&*dex.begin()==1&&*dex.rbegin()==386,"national dex");
    for (auto pair:{std::pair<unsigned,unsigned>{277,252},{385,351},{410,386},{411,358}})
        Check(Cry(pair.first,0,r)&&r.identity=="cry."+std::to_string(pair.second)+".mode.0","core ID is not dex");
    for (unsigned s=413;s<=439;++s) Check(Cry(s,0,r)&&r.identity=="cry.201.mode.0","Unown shared cry");
    for (unsigned s=252;s<277;++s) Check(!Cry(s,0,r),"old Unown placeholders");
    for (unsigned s:{0u,412u,440u,65535u}) Check(!Cry(s,0,r),"invalid species");
    Check(!Cry(25,13,r),"invalid mode");
    for (const auto& f:Fanfares) Check(Fanfare(f.id,r)&&r.identity=="song."+std::to_string(f.song),"fanfare index alias");
    Check(!Fanfare(18,r),"invalid fanfare index");
    RemasterEmeraldScriptRequest q{};q.type=REMASTER_EMERALD_SCRIPT_REQUEST_SOUND;q.action=REMASTER_EMERALD_SCRIPT_ASYNC_START;q.local_id=5;q.value_u16=0;
    auto before=q;Check(Script(q,r)==ScriptRead::Ready&&r.identity=="song.5","runtime uses .a, not .b");
    Check(std::memcmp(&before,&q,sizeof q)==0,"immutable source request");
    q.action=REMASTER_EMERALD_SCRIPT_ASYNC_WAIT;Check(Script(q,r)==ScriptRead::HostOwnedWait,"source wait remains with host");
    q.type=REMASTER_EMERALD_SCRIPT_REQUEST_FANFARE;q.action=REMASTER_EMERALD_SCRIPT_ASYNC_START;q.local_id=Fanfares[0].song;
    Check(Script(q,r)==ScriptRead::Ready&&r.category==Category::Jingle,"script fanfare carries song number");
    q.local_id=5;Check(Script(q,r)==ScriptRead::Unsupported,"wrong fanfare category");
    q.type=REMASTER_EMERALD_SCRIPT_REQUEST_BGM;q.local_id=0;q.value_u16=1;
    Check(Script(q,r)==ScriptRead::Ready&&r.stopMusic,"BGM flag is not sound ID");
    q.action=REMASTER_EMERALD_SCRIPT_BGM_FADE_DEFAULT;Check(Script(q,r)==ScriptRead::Unsupported,"default fade needs map owner");
    q.type=REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE;Check(Script(q,r)==ScriptRead::NotAudio,"non-audio boundary");
    Settings s;s.master=std::numeric_limits<float>::quiet_NaN();s.music=2;s.sfx=-1;s.ambience=std::numeric_limits<float>::infinity();s.Sanitize();
    Check(s.master==1&&s.music==1&&s.sfx==0&&s.ambience==.6f,"settings finite and bounded");
    s.master=0;Check(s.Gain(Category::Music)==0&&s.Gain(Category::Cry)==0,"master mute");
    Policy p;Check(!p.Admit(0,1).accepted,"invalid voice ID");
    for (uint64_t id=1;id<=16;++id) Check(p.Admit(id,1).accepted,"voice budget accepts");
    Check(!p.Admit(17,1).accepted&&p.Active()==16,"equal priority drop");Check(!p.Admit(1,10).accepted,"duplicate ID");
    auto high=p.Admit(17,10);Check(high.accepted&&high.evicted==1&&p.Active()==16,"oldest low priority eviction");
    p.Finished(17);Check(p.Active()==15,"completion frees slot");
    p.Background(true);p.Focus(false);Check(!p.Admit(18,10).accepted,"background rejects");
    p.Background(false);Check(p.Suspended(),"foreground retains focus loss");
    p.Focus(true);Check(!p.Suspended()&&p.Admit(18,10).accepted,"resume after both clear");
    p.Focus(false);p.Reset();Check(p.Active()==0&&p.Suspended(),"pack reset keeps lifecycle state");
    std::cout<<checks<<" R15 resolver and policy checks passed\n";return 0;
} catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
