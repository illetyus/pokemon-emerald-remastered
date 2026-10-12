#include "RemasterSessionModel.h"
#include <iostream>
#include <limits>
static unsigned checks, failures;
static void check(bool ok,const char* message) {
    ++checks; if(!ok){++failures;std::cerr<<"session: "<<message<<'\n';}
}
int main() {
    using namespace RemasterSession;
    Lifecycle session;
    check(session.state==State::Idle && session.generation==0,"boot cannot invent a playing session");
    check(!session.Playing(),"no gameplay before validated load");
    for(auto status : {REMASTER_EMERALD_SAVE_EMPTY,REMASTER_EMERALD_SAVE_CORRUPT,
        REMASTER_EMERALD_SAVE_IO_ERROR,REMASTER_EMERALD_SAVE_UNSUPPORTED}) {
        check(!CanContinue(status),"unusable file cannot continue");
        check(CanNewGame(status)==(status==REMASTER_EMERALD_SAVE_EMPTY),"only confirmed absence permits missing-save new-game path");
        check(session.Begin(),"load begins from idle/error");
        const auto token=session.generation;
        check(session.state==State::Loading && !session.Playing(),"loading is not usable gameplay");
        check(!session.Begin() && session.generation==token,"duplicate start never produces a second session");
        check(session.Finish(token,status,true),"current unsuccessful completion is observed");
        check(session.state==State::Error && !session.Playing(),"ready map cannot make a bad save usable");
        check(!session.Finish(token,REMASTER_EMERALD_SAVE_OK,true),"duplicate completion cannot turn error into playing");
    }
    for(auto status : {REMASTER_EMERALD_SAVE_OK,REMASTER_EMERALD_SAVE_DEGRADED}) {
        check(CanContinue(status) && !CanNewGame(status),"valid save offers continue and protects missing-save initialization");
        check(session.Begin(),"valid load begin");const auto token=session.generation;
        check(session.Finish(token,status,false),"map failure completion");
        check(session.state==State::Error && !session.Playing(),"decoded save with failed map is not playing");
        check(session.Begin(),"retry from map failure");const auto next=session.generation;
        check(next>token,"strictly monotonic session generation");
        check(!session.Finish(token,status,true),"old async map completion cannot publish a new session");
        check(session.Finish(next,status,true) && session.Playing(),"only real current decoded save plus map load plays");
        check(!session.Finish(next,status,true),"success completion exactly once");
        session.Cancel();check(session.state==State::Idle && session.generation>next,"title/cancel invalidates old generation");
        check(!session.Finish(next,status,true),"cancelled generation cannot reopen gameplay");
    }
    check(session.Begin(),"cancel active load begin");const auto cancelled=session.generation;
    session.Cancel();check(!session.Finish(cancelled,REMASTER_EMERALD_SAVE_OK,true),"in-flight cancel fences completion");
    session.generation=std::numeric_limits<uint64_t>::max()-1;
    check(session.Begin() && session.generation==std::numeric_limits<uint64_t>::max(),"last representable generation is unique");
    session.Cancel();check(session.state==State::Error && !session.Begin(),"generation exhaustion fails closed instead of wrapping");
    check(!session.Finish(std::numeric_limits<uint64_t>::max(),REMASTER_EMERALD_SAVE_OK,true),"exhausted owner cannot accept stale completion");
    std::cout<<"AP04 session "<<checks<<" checks, "<<failures<<" failures\n";return failures ? 1 : 0;
}
