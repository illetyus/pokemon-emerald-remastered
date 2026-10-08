#include "RemasterInputDispatch.h"
#include "RemasterUiModel.h"
#include <cstdlib>
#include <iostream>
#include <memory>
using namespace RemasterControls;
static unsigned checks=0;
static void Check(bool ok,const char* why) {
    ++checks;if(!ok){std::cerr<<why<<"\n";std::exit(1);}
}
int main() {
    auto save=std::make_unique<RemasterEmeraldSave>();
    save->status=REMASTER_EMERALD_SAVE_OK;
    RemasterUi::Model ui;RemasterUi::Context uiContext;uiContext.mapReady=true;
    ui.Build(save.get(),uiContext);
    Router r;Context context;context.saveUsable=context.mapReady=context.uiAttached=true;
    r.SetContext(context);
    unsigned world=0,field=0,battle=0,uiCalls=0;
    auto uiOwner=[&](Action action){++uiCalls;const bool consumed=ui.Input(
        static_cast<RemasterUi::Action>(action),save.get(),uiContext);ui.Build(save.get(),uiContext);return consumed;};
    auto worldOwner=[&](Action){++world;return true;};
    auto fieldOwner=[&](Action){++field;return false;};
    auto battleOwner=[&](Action){++battle;return false;};
    auto send=[&](Action action){
        const auto result=r.Submit({Source::Keyboard,0,static_cast<std::uint16_t>(action),action,Phase::Pressed,r.Epoch()});
        return result.status==Status::Dispatch&&Deliver(result.action,result.target,
            uiOwner,worldOwner,fieldOwner,battleOwner);
    };
    const auto fingerprint=RemasterUi::Fingerprint(save.get());
    Check(send(Action::Menu)&&ui.Modal(),"R9 menu receives world menu action");
    context.uiModal=ui.Modal();r.SetContext(context);
    const auto oldEpoch=r.Epoch();
    Check(send(Action::Down)&&world==0,"modal direction reaches real R9 instead of world");
    Check(send(Action::Cancel)&&!ui.Modal(),"cancel reaches same R9 route");
    context.uiModal=false;r.SetContext(context);
    Check(r.Submit({Source::Touch,0,0,Action::Down,Phase::Pressed,oldEpoch}).status==Status::Stale,
          "queued old menu action cannot reach world after cancel");
    Check(send(Action::Down)&&world==1,"fresh world direction uses one world owner");
    Check(!send(Action::Confirm)&&field==0,"missing field owner rejected before delivery");
    context.fieldAttached=true;r.SetContext(context);
    Check(!send(Action::Confirm)&&field==1&&world==1,"rejecting field owner never falls through");
    context.battleBusy=true;context.battleAttached=true;r.SetContext(context);
    Check(!send(Action::Confirm)&&battle==1&&world==1,"battle rejection never falls through to world");
    Check(!send(Action::Menu)&&uiCalls==3,"battle menu action blocked before R9 delivery");
    context.battleBusy=false;context.scriptBusy=true;r.SetContext(context);
    Check(!send(Action::Up)&&world==1,"script-busy cannot move world");
    context.dialoguePending=true;r.SetContext(context);
    Check(r.CurrentFocus()==Focus::Dialogue,"pending dialogue precedes script busy");
    context.dialoguePending=false;context.scriptBusy=false;context.saveUsable=context.mapReady=false;
    save->status=REMASTER_EMERALD_SAVE_CORRUPT;uiContext.mapReady=false;r.SetContext(context);
    Check(send(Action::Menu)&&ui.Modal(),"corrupt save retains actual R9 recovery menu");
    Check(send(Action::Up)&&world==1,"recovery navigation cannot move world");
    Check(!Deliver(Action::Confirm,Target::WorldStep,uiOwner,worldOwner,fieldOwner,battleOwner),
          "malformed direction target cannot call world owner");
    Check(!Deliver(Action::Menu,Target::BattleInput,uiOwner,worldOwner,fieldOwner,battleOwner),
          "malformed battle target cannot call battle owner");
    save->status=REMASTER_EMERALD_SAVE_OK;
    Check(RemasterUi::Fingerprint(save.get())==fingerprint,"input routing does not mutate Emerald state");
    context.uiModal=false;context.saveUsable=context.mapReady=true;r.SetContext(context);
    Check(send(Action::Up),"world hold established");
    const auto before=r.Epoch();++context.hostGeneration;r.SetContext(context);
    Check(r.Epoch()>before&&!r.Held(Action::Up),"same-focus load/owner boundary fences holds");
    std::cout<<checks<<" R8 focus/real-R9 dispatch checks passed\n";
}
