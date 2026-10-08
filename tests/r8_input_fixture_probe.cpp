#include "RemasterInputLifecycle.h"
#include "RemasterInputRepeat.h"
#include <iostream>
#include <sstream>
#include <string>
using namespace RemasterControls;
static const char* Name(Status s){
    switch(s){case Status::Dispatch:return "Dispatch";case Status::Updated:return "Updated";
    case Status::Ignored:return "Ignored";case Status::Blocked:return "Blocked";case Status::Unsupported:return "Unsupported";
    case Status::Invalid:return "Invalid";case Status::Stale:return "Stale";case Status::Full:return "Full";
    case Status::Exhausted:return "Exhausted";}return "Invalid";
}
static const char* Name(Target t){switch(t){case Target::None:return "None";case Target::UI:return "UI";
    case Target::WorldStep:return "WorldStep";case Target::FieldInteract:return "FieldInteract";
    case Target::BattleInput:return "BattleInput";}return "None";}
static const char* Name(Focus f){switch(f){case Focus::Blocked:return "Blocked";case Focus::World:return "World";
    case Focus::UI:return "UI";case Focus::Dialogue:return "Dialogue";case Focus::Battle:return "Battle";
    case Focus::RecoveryUI:return "RecoveryUI";}return "Blocked";}
int main(){
    Router router;Touch touch;Digital digital;Analog enhanced,pad(Source::Gamepad);Lifecycle lifecycle;UiRepeat repeat;Context context;
    std::string line;
    while(std::getline(std::cin,line)){
        std::istringstream in(line);std::string command;in>>command;
        Result result;unsigned repeatUp=0,repeatDown=0;
        if(command=="context"){
            unsigned suspension=0;
            in>>context.saveUsable>>context.mapReady>>context.uiModal>>context.dialoguePending>>context.scriptBusy
                >>context.battleBusy>>context.uiAttached>>context.fieldAttached>>context.battleAttached>>context.ioBusy
                >>suspension>>context.hostGeneration;
            context.suspensionReasons=static_cast<std::uint8_t>(suspension)|lifecycle.Reasons();
            result.status=router.SetContext(context)?Status::Updated:Status::Exhausted;
        }else if(command=="event"||command=="physical"){
            unsigned source=0,device=0,control=0,action=0,phase=0;std::uint64_t epoch=0;
            in>>source>>device>>control>>action>>phase>>epoch;
            if(source>255||device>65535||control>65535||action>255||phase>255)return 2;
            const Event event{static_cast<Source>(source),static_cast<std::uint16_t>(device),static_cast<std::uint16_t>(control),
                static_cast<Action>(action),static_cast<Phase>(phase),epoch};
            if(command=="event")result=router.Submit(event);
            else{
                const auto packet=digital.Process(event.source,event.device,event.control,event.action,event.phase,event.epoch);
                result=packet.hasEvent?router.Submit(packet.event):Result{packet.status};
            }
        }else if(command=="touch"){
            unsigned phase=0,finger=0;double x=0,y=0;std::uint64_t epoch=0;in>>phase>>finger>>x>>y>>epoch;
            TouchPacket packet;
            if(phase==0)packet=touch.Press(finger,x,y,epoch);
            else if(phase==1)packet=touch.Release(finger,epoch);
            else if(phase==2)packet=touch.Cancel(finger,epoch);
            else if(phase==3)packet=touch.Move(finger,x,y,epoch);
            else return 2;
            result=packet.hasEvent?router.Submit(packet.event):Result{packet.valid?Status::Ignored:Status::Invalid};
        }else if(command=="axis"){
            unsigned source=0;double x=0,y=0;std::uint64_t epoch=0;in>>source>>x>>y>>epoch;
            if(source!=1&&source!=3)return 2;
            auto& adapter=source==1?pad:enhanced;const auto batch=adapter.Process(x,y,epoch);
            result.status=batch.status;
            for(unsigned i=0;i<batch.count;++i)result=router.Submit(batch.events[i]);
            if(batch.status==Status::Invalid)result.status=Status::Invalid;
        }else if(command=="suspend"){
            unsigned reason=0;bool suspended=false;in>>reason>>suspended;
            if(lifecycle.Set(static_cast<Suspension>(reason),suspended)){
                FenceSources(router,touch,digital,enhanced,pad);
                context.suspensionReasons=lifecycle.Reasons();router.SetContext(context);
            }
            result.status=Status::Updated;
        }else if(command=="fence"){
            FenceSources(router,touch,digital,enhanced,pad);result.status=Status::Updated;
        }else if(command=="tick"){
            unsigned count=0;bool enabled=false;in>>count>>enabled;if(count>20000)return 2;
            for(unsigned i=0;i<count;++i){const auto frame=repeat.Tick(router,enabled);repeatUp+=frame.up;repeatDown+=frame.down;}
            result.status=Status::Updated;
        }else return 2;
        if(!in)return 2;
        std::string extra;if(in>>extra)return 2;
        std::cout<<"{\"status\":\""<<Name(result.status)<<"\",\"target\":\""<<Name(result.target)
            <<"\",\"focus\":\""<<Name(router.CurrentFocus())<<"\",\"epoch\":"<<router.Epoch()
            <<",\"held\":[";
        for(unsigned i=0;i<10;++i){if(i)std::cout<<",";std::cout<<router.HeldCount(static_cast<Action>(i));}
        std::cout<<"],\"repeat_up\":"<<repeatUp<<",\"repeat_down\":"<<repeatDown<<"}\n";
    }
}
