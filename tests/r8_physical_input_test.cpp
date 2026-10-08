#include "RemasterPhysicalInput.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
using namespace RemasterControls;
static unsigned checks=0;
static void Check(bool ok,const char* why) {
    ++checks;if(!ok){std::cerr<<why<<"\n";std::exit(1);}
}
static Context World() {
    Context c;c.mapReady=c.saveUsable=c.uiAttached=c.fieldAttached=true;return c;
}
static void Apply(Router& router,const InputPacket& p) {
    Check(p.status!=Status::Invalid&&p.status!=Status::Full&&p.status!=Status::Stale,"valid adapter event");
    if(p.hasEvent) router.Submit(p.event);
}
static void Apply(Router& router,const InputBatch& batch) {
    for(unsigned i=0;i<batch.count;++i) router.Submit(batch.events[i]);
}
int main(int argc,char**argv) {
    if(argc==2&&std::string(argv[1])=="--bindings") {
        std::cout<<"[";
        bool first=true;
        for(const auto& b:Bindings) {
            if(!first)std::cout<<",";
            first=false;
            std::cout<<"{\"key\":\""<<b.schemaKey<<"\",\"engine_key\":\""<<b.engineKey
                <<"\",\"source\":\""<<(b.source==Source::Keyboard?"keyboard":"gamepad")
                <<"\",\"action\":\""<<ActionName(b.action)<<"\"}";
        }
        std::cout<<"]\n";return 0;
    }
    Router r;r.SetContext(World());Digital digital;
    for(const auto& b:Bindings) {
        auto pressed=digital.Process(b.source,0,b.control,b.action,Phase::Pressed,r.Epoch());
        Check(pressed.hasEvent&&pressed.event.action==b.action,"physical key normalized");
        Check(r.Submit(pressed.event).status==Status::Dispatch,"fresh mapped key dispatches");
        Check(!digital.Process(b.source,0,b.control,b.action,Phase::Pressed,r.Epoch()).hasEvent,
              "OS duplicate press does not generate second command");
        auto released=digital.Process(b.source,0,b.control,b.action,Phase::Released,r.Epoch());
        Check(released.hasEvent&&released.event.phase==Phase::Released,"physical release normalized");
        Check(r.Submit(released.event).target==Target::None&&!r.Held(b.action),"release cannot command gameplay");
    }
    Apply(r,digital.Process(Source::Keyboard,0,0,Action::Up,Phase::Pressed,r.Epoch()));
    Apply(r,digital.Process(Source::Keyboard,0,1,Action::Up,Phase::Pressed,r.Epoch()));
    Check(r.HeldCount(Action::Up)==2,"alias controls remain distinct");
    Apply(r,digital.Process(Source::Keyboard,0,0,Action::Up,Phase::Released,r.Epoch()));
    Check(r.Held(Action::Up),"releasing arrow retains W");
    const auto oldEpoch=r.Epoch();r.Reset();
    Check(!digital.Process(Source::Keyboard,0,1,Action::Up,Phase::Pressed,r.Epoch()).hasEvent,
          "held physical key cannot rearm across focus reset");
    auto released=digital.Process(Source::Keyboard,0,1,Action::Up,Phase::Released,r.Epoch());
    Check(released.event.epoch==oldEpoch&&r.Submit(released.event).status==Status::Stale,
          "physical release retains captured press epoch");
    Apply(r,digital.Process(Source::Keyboard,0,1,Action::Up,Phase::Pressed,r.Epoch()));
    Check(r.Held(Action::Up),"release then fresh press rearms");
    Check(digital.Process(Source::Keyboard,0,1,Action::Down,Phase::Canceled,r.Epoch()).status==Status::Invalid,
          "action mismatch preserves capture");
    Check(digital.Process(static_cast<Source>(255),0,1,Action::Up,Phase::Pressed,r.Epoch()).status==Status::Invalid,
          "invalid source cannot index cache");
    Check(digital.Process(Source::Keyboard,0,2,Action::Up,Phase::Pressed,oldEpoch).status==Status::Stale,
          "old normalized source epoch cannot roll back cache");

    r.Reset();digital.Reset();
    for(unsigned i=0;i<64;++i)
        Check(digital.Process(Source::Gamepad,0,static_cast<std::uint16_t>(i),Action::Up,Phase::Pressed,r.Epoch()).hasEvent,
              "digital capture capacity");
    Check(digital.Process(Source::Gamepad,0,64,Action::Up,Phase::Pressed,r.Epoch()).status==Status::Full,
          "digital overflow explicit");

    Analog analog;r.Reset();
    auto batch=analog.Process(.8,0,r.Epoch());
    Check(batch.count==1&&batch.events[0].action==Action::Right,"analog rising direction");
    Apply(r,batch);
    Check(r.Held(Action::Right),"analog shares common held state");
    Check(analog.Process(.9,.1,r.Epoch()).count==0,"same-direction Triggered callback never steps again");
    batch=analog.Process(0,.9,r.Epoch());
    Check(batch.count==2&&batch.events[0].phase==Phase::Released
        &&batch.events[1].phase==Phase::Pressed&&batch.events[1].action==Action::Up,"changed axis release before press");
    Apply(r,batch);
    Check(!r.Held(Action::Right)&&r.Held(Action::Up),"old axis control cleared");
    r.Reset();
    Check(analog.Process(0,.9,r.Epoch()).count==0,"held stick quarantined across focus");
    batch=analog.Process(0,0,r.Epoch());Apply(r,batch);
    batch=analog.Process(0,.9,r.Epoch());
    Check(batch.count==1,"neutral then fresh direction rearms");
    Apply(r,batch);
    batch=analog.Process(std::numeric_limits<double>::quiet_NaN(),0,r.Epoch());Apply(r,batch);
    Check(batch.status==Status::Invalid&&batch.count==1&&batch.events[0].phase==Phase::Canceled,
          "invalid axis cancels prior hold explicitly");
    Check(!r.Held(Action::Up),"invalid axis cannot leave common hold stuck");
    Check(analog.Process(.8,0,r.Epoch()).count==0,"invalid stream requires neutral before rearming");
    analog.Process(0,0,r.Epoch());
    batch=analog.Process(.8,0,r.Epoch());Apply(r,batch);
    Check(r.Held(Action::Right),"fresh neutral boundary restores axis");
    Analog gamepad(Source::Gamepad);r.Reset();analog.Reset();
    auto enhanced=analog.Process(.8,0,r.Epoch());
    auto pad=gamepad.Process(.8,0,r.Epoch());
    Check(enhanced.count==1&&pad.count==1&&pad.events[0].source==Source::Gamepad,
          "gamepad analog retains physical source identity");
    Check(r.Submit(enhanced.events[0]).status==Status::Dispatch,"first analog source dispatches");
    Check(r.Submit(pad.events[0]).status==Status::Updated&&r.HeldCount(Action::Right)==2,
          "second analog source merges hold without duplicate command");
    Apply(r,gamepad.Process(0,0,r.Epoch()));
    Check(r.HeldCount(Action::Right)==1,"gamepad release retains enhanced hold");
    Apply(r,analog.Process(0,0,r.Epoch()));
    Check(!r.Held(Action::Right),"last analog source release clears hold");
    std::cout<<checks<<" R8 physical input checks passed\n";
}

