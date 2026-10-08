#pragma once
#include "RemasterInputRouter.h"
#include <array>

namespace RemasterControls {
struct Binding { const char* schemaKey;const char* engineKey;Source source;Action action;std::uint16_t control; };
inline constexpr std::array<Binding,25> Bindings{{
    {"Up","Up",Source::Keyboard,Action::Up,0},{"W","W",Source::Keyboard,Action::Up,1},
    {"Down","Down",Source::Keyboard,Action::Down,2},{"S","S",Source::Keyboard,Action::Down,3},
    {"Left","Left",Source::Keyboard,Action::Left,4},{"A","A",Source::Keyboard,Action::Left,5},
    {"Right","Right",Source::Keyboard,Action::Right,6},{"D","D",Source::Keyboard,Action::Right,7},
    {"Enter","Enter",Source::Keyboard,Action::Confirm,8},{"SpaceBar","SpaceBar",Source::Keyboard,Action::Confirm,9},
    {"Escape","Escape",Source::Keyboard,Action::Cancel,10},{"Tab","Tab",Source::Keyboard,Action::Menu,11},
    {"M","M",Source::Keyboard,Action::Map,12},{"Q","Q",Source::Keyboard,Action::Quest,13},
    {"R","R",Source::Keyboard,Action::QuickItem,14},
    {"DPadUp","Gamepad_DPad_Up",Source::Gamepad,Action::Up,15},
    {"DPadDown","Gamepad_DPad_Down",Source::Gamepad,Action::Down,16},
    {"DPadLeft","Gamepad_DPad_Left",Source::Gamepad,Action::Left,17},
    {"DPadRight","Gamepad_DPad_Right",Source::Gamepad,Action::Right,18},
    {"FaceBottom","Gamepad_FaceButton_Bottom",Source::Gamepad,Action::Confirm,19},
    {"FaceRight","Gamepad_FaceButton_Right",Source::Gamepad,Action::Cancel,20},
    {"SpecialRight","Gamepad_Special_Right",Source::Gamepad,Action::Menu,21},
    {"LeftShoulder","Gamepad_LeftShoulder",Source::Gamepad,Action::Map,22},
    {"RightShoulder","Gamepad_RightShoulder",Source::Gamepad,Action::Quest,23},
    {"FaceTop","Gamepad_FaceButton_Top",Source::Gamepad,Action::QuickItem,24}
}};
inline const char* ActionName(Action action) {
    switch(action) {
    case Action::Up:return "Up";case Action::Down:return "Down";case Action::Left:return "Left";
    case Action::Right:return "Right";case Action::Confirm:return "Confirm";case Action::Cancel:return "Cancel";
    case Action::Menu:return "Menu";case Action::Map:return "Map";case Action::Quest:return "Quest";
    case Action::QuickItem:return "QuickItem";default:return "Invalid";
    }
}
struct InputPacket { Status status=Status::Ignored;bool hasEvent=false;Event event{}; };
class Digital {
public:
    void Reset() { for(auto& record:records) record.active=false; }
    InputPacket Process(Source source,std::uint16_t device,std::uint16_t control,Action action,
                        Phase phase,std::uint64_t epoch) {
        if(static_cast<unsigned>(source)>=static_cast<unsigned>(Source::Count)
            ||static_cast<unsigned>(action)>=static_cast<unsigned>(Action::Count)
            ||static_cast<unsigned>(phase)>static_cast<unsigned>(Phase::Canceled))return{Status::Invalid};
        if(!epoch||epoch<latestEpoch)return{Status::Stale};
        latestEpoch=epoch;
        Record* found=nullptr;Record* free=nullptr;
        for(auto& r:records) {
            if(!r.active){if(!free)free=&r;continue;}
            if(r.source==source&&r.device==device&&r.control==control)found=&r;
        }
        if(found&&found->action!=action)return{Status::Invalid};
        if(phase!=Phase::Pressed) {
            if(!found)return{};
            Event e{source,device,control,action,phase,found->epoch};
            found->active=false;return{Status::Updated,true,e};
        }
        // Keep old press epochs as physical tombstones through focus changes.
        if(found)return{};
        if(!free)return{Status::Full};
        *free={true,source,device,control,action,epoch};
        return{Status::Updated,true,{source,device,control,action,phase,epoch}};
    }
private:
    struct Record {
        bool active=false;Source source=Source::Keyboard;std::uint16_t device=0,control=0;
        Action action=Action::Up;std::uint64_t epoch=0;
    };
    std::array<Record,64> records{};
    std::uint64_t latestEpoch=0;
};
struct InputBatch { Status status=Status::Ignored;unsigned count=0;std::array<Event,2> events{}; };
class Analog {
public:
    explicit Analog(Source source=Source::Enhanced):source(source){}
    void Reset() { active=false;neutralFence=false; }
    void Fence() { active=false;neutralFence=true; }
    InputBatch Process(double x,double y,std::uint64_t epoch) {
        if(!epoch||epoch<latestEpoch)return{Status::Stale};
        latestEpoch=epoch;
        InputBatch out;
        const Axis next=Quantize(x,y);
        if(!next.valid) {
            if(active)out.events[out.count++]=Make(Phase::Canceled);
            active=false;neutralFence=true;out.status=Status::Invalid;return out;
        }
        if(!next.active) {
            if(active)out.events[out.count++]=Make(Phase::Released);
            active=false;neutralFence=false;out.status=Status::Updated;return out;
        }
        if(neutralFence||(active&&capturedEpoch!=epoch)) {
            neutralFence=true;return out;
        }
        if(active&&action==next.action)return out;
        if(active)out.events[out.count++]=Make(Phase::Released);
        action=next.action;capturedEpoch=epoch;active=true;
        out.events[out.count++]=Make(Phase::Pressed);out.status=Status::Updated;return out;
    }
private:
    Event Make(Phase phase) const { return{source,0,60,action,phase,capturedEpoch}; }
    Source source;
    bool active=false,neutralFence=false;
    Action action=Action::Up;
    std::uint64_t capturedEpoch=0,latestEpoch=0;
};
} // namespace RemasterControls

