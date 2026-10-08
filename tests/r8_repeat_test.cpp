#include "RemasterInputRepeat.h"
#include <cstdlib>
#include <iostream>
using namespace RemasterControls;
static unsigned checks=0;
static void Check(bool ok,const char* why){++checks;if(!ok){std::cerr<<why<<"\n";std::exit(1);}}
static Context UI(){Context c;c.uiAttached=c.uiModal=c.mapReady=c.saveUsable=true;return c;}
static void Press(Router& r,Source source,Action action,unsigned control=0){
    Check(r.Submit({source,0,static_cast<std::uint16_t>(control),action,Phase::Pressed,r.Epoch()}).status==Status::Dispatch,
          "fresh common hold");
}
int main(){
    for(auto source:{Source::Keyboard,Source::Gamepad,Source::Touch,Source::Enhanced}){
        Router r;r.SetContext(UI());UiRepeat repeat;Press(r,source,Action::Up);
        for(unsigned tick=1;tick<=200;++tick){
            const auto frame=repeat.Tick(r,true);
            Check(frame.up==(tick>=24&&(tick-24)%3==0)&&!frame.down,"all sources share accepted 24/3 R9 cadence");
        }
        r.Submit({source,0,0,Action::Up,Phase::Released,r.Epoch()});
        Check(!repeat.Tick(r,true).up,"release stops repeat");
    }
    Router r;auto context=UI();r.SetContext(context);UiRepeat repeat;Press(r,Source::Touch,Action::Down);
    for(unsigned i=0;i<23;++i)Check(!repeat.Tick(r,true).down,"no early repeat");
    repeat.Tick(r,false);
    for(unsigned i=0;i<23;++i)Check(!repeat.Tick(r,true).down,"setting disable clears countdown");
    Check(repeat.Tick(r,true).down,"re-enabled repeat retains first 24 ticks");
    ++context.hostGeneration;r.SetContext(context);
    Check(repeat.Sync(r,true)&&!repeat.Tick(r,true).down,"focus/request epoch resets repeat and clock boundary");
    Press(r,Source::Keyboard,Action::Down);
    for(unsigned i=0;i<20;++i)repeat.Tick(r,true);
    Check(r.Submit({Source::Touch,0,1,Action::Down,Phase::Pressed,r.Epoch()}).status==Status::Updated,
          "second source merges same hold");
    r.Submit({Source::Keyboard,0,0,Action::Down,Phase::Released,r.Epoch()});
    for(unsigned i=0;i<3;++i)Check(!repeat.Tick(r,true).down,"source handover preserves semantic countdown");
    Check(repeat.Tick(r,true).down,"held semantic handover reaches original tick 24");
    context.uiModal=false;r.SetContext(context);Press(r,Source::Keyboard,Action::Up);
    for(unsigned i=0;i<60;++i){const auto f=repeat.Tick(r,true);Check(!f.up&&!f.down,"world holds never repeat through UI clock");}
    context.battleBusy=context.battleAttached=true;r.SetContext(context);Press(r,Source::Gamepad,Action::Up);
    for(unsigned i=0;i<60;++i)Check(!repeat.Tick(r,true).up,"battle hold never repeated");
    context.battleBusy=false;context.dialoguePending=true;r.SetContext(context);Press(r,Source::Enhanced,Action::Down);
    for(unsigned i=0;i<23;++i)Check(!repeat.Tick(r,true).down,"dialogue navigation respects first delay");
    Check(repeat.Tick(r,true).down,"dialogue choice direction may repeat");
    r.Reset();Press(r,Source::Enhanced,Action::Confirm);
    for(unsigned i=0;i<60;++i){const auto f=repeat.Tick(r,true);Check(!f.up&&!f.down,"Confirm cannot produce any repeat");}
    context.dialoguePending=false;context.mapReady=context.saveUsable=false;r.SetContext(context);
    Press(r,Source::Touch,Action::Up);
    for(unsigned i=0;i<30;++i)Check(!repeat.Tick(r,true).up,"recovery HUD is outside repeat scope");
    context=UI();r.SetContext(context);Press(r,Source::Keyboard,Action::Up);
    unsigned emitted=0;for(unsigned i=1;i<=10000;++i){const auto f=repeat.Tick(r,true);if(f.up)++emitted;
        Check(f.up==(i>=24&&(i-24)%3==0),"bounded counter preserves long 24/3 cadence");}
    Check(emitted==(10000-24)/3+1,"long hold repeat count");
    std::cout<<checks<<" R8 common-held R9 repeat checks passed\n";
}
