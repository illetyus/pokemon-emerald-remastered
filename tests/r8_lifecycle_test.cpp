#include "RemasterInputLifecycle.h"
#include <cstdlib>
#include <iostream>
using namespace RemasterControls;
static unsigned checks=0;
static void Check(bool ok,const char* why){++checks;if(!ok){std::cerr<<why<<"\n";std::exit(1);}}
int main(){
    Lifecycle life;Router r;Touch touch;Digital digital;Analog enhanced,pad(Source::Gamepad);
    Context c;c.mapReady=c.saveUsable=c.uiAttached=c.fieldAttached=true;r.SetContext(c);
    auto held=digital.Process(Source::Keyboard,0,0,Action::Up,Phase::Pressed,r.Epoch());
    Check(r.Submit(held.event).status==Status::Dispatch,"initial physical command");
    const auto epoch=r.Epoch();
    auto packet=touch.Press(0,.90,.70,epoch);r.Submit(packet.event);
    auto axis=pad.Process(.8,0,epoch);r.Submit(axis.events[0]);
    Check(r.Held(Action::Up)&&r.Held(Action::Confirm)&&r.Held(Action::Right),"mixed sources held");
    Check(life.Set(Suspension::Background,true),"background transition");
    FenceSources(r,touch,digital,enhanced,pad);c.suspensionReasons=life.Reasons();r.SetContext(c);
    Check(r.CurrentFocus()==Focus::Blocked&&!r.Held(Action::Up)&&!r.Held(Action::Right),"suspend clears common holds");
    Check(!touch.Release(0,r.Epoch()).hasEvent,"suspend clears touch captures");
    Check(r.Submit(held.event).status==Status::Stale,"queued old press fenced");
    Check(life.Set(Suspension::Inactive,true)&&life.Set(Suspension::Paused,true),"independent suspension reasons");
    Check(!life.Set(Suspension::Background,true),"duplicate reason idempotent");
    Check(life.Set(Suspension::Background,false)&&life.Reasons()!=0,"foreground cannot clear inactive/pause");
    c.suspensionReasons=life.Reasons();r.SetContext(c);
    Check(r.Submit({Source::Touch,0,0,Action::Confirm,Phase::Pressed,r.Epoch()}).status==Status::Blocked,
          "partial resume still blocks actions");
    Check(life.Set(Suspension::Inactive,false)&&life.Reasons()!=0,"focus cannot clear pause");
    Check(life.Set(Suspension::Paused,false)&&life.Reasons()==0,"all reasons cleared");
    FenceSources(r,touch,digital,enhanced,pad);c.suspensionReasons=life.Reasons();r.SetContext(c);
    Check(pad.Process(.8,0,r.Epoch()).count==0,"held analog cannot restore after resume");
    pad.Process(0,0,r.Epoch());axis=pad.Process(.8,0,r.Epoch());
    Check(axis.count==1&&r.Submit(axis.events[0]).status==Status::Dispatch,"neutral then fresh stick rearms");
    held=digital.Process(Source::Keyboard,0,0,Action::Up,Phase::Pressed,r.Epoch());
    Check(held.hasEvent&&r.Submit(held.event).status==Status::Dispatch,"fresh physical edge after platform flush");
    const auto beforeDisconnect=r.Epoch();FenceSources(r,touch,digital,enhanced,pad);
    Check(!r.Held(Action::Up)&&r.Epoch()>beforeDisconnect,"disconnect resets all sources conservatively");
    FenceSources(r,touch,digital,enhanced,pad);
    Check(!r.Held(Action::Right)&&pad.Process(.8,0,r.Epoch()).count==0,"reconnect never restores held stick");
    Check(r.Submit(held.event).status==Status::Stale,"queued pre-disconnect event cannot mutate new session");
    const auto reasons=life.Reasons();
    Check(!life.Set(static_cast<Suspension>(255),true)&&life.Reasons()==reasons,"invalid reason rejected atomically");
    Check(!life.Set(static_cast<Suspension>(3),true),"composite reason cannot clear unrelated masks");
    std::cout<<checks<<" R8 lifecycle checks passed\n";
}
