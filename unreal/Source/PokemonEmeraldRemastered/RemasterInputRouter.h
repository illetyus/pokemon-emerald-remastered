#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

// Platform-neutral input state only. No gameplay/save pointers, UE or Android types.
namespace RemasterControls {

enum class Action : std::uint8_t { Up, Down, Left, Right, Confirm, Cancel,
    Menu, Map, Quest, QuickItem, Count };
enum class Source : std::uint8_t { Keyboard, Gamepad, Touch, Enhanced, Count };
enum class Phase : std::uint8_t { Pressed, Released, Canceled };
enum class Focus : std::uint8_t { Blocked, World, UI, Dialogue, Battle, RecoveryUI };
enum class Target : std::uint8_t { None, UI, WorldStep, FieldInteract, BattleInput };
enum class Status : std::uint8_t {
    Dispatch, Updated, Ignored, Blocked, Unsupported, Invalid, Stale, Full, Exhausted
};

struct Event {
    Source source = Source::Keyboard;
    std::uint16_t device = 0, control = 0;
    Action action = Action::Up;
    Phase phase = Phase::Pressed;
    std::uint64_t epoch = 0;
};
struct Result {
    Status status = Status::Ignored;
    Target target = Target::None;
    Action action = Action::Up;
};
struct Context {
    std::uint8_t suspensionReasons = 0;
    bool ioBusy = false, dialoguePending = false, battleBusy = false, scriptBusy = false;
    bool uiModal = false, saveUsable = false, mapReady = false;
    bool uiAttached = false, fieldAttached = false, battleAttached = false;
    // Owner generation changes on VM/request/view ownership, not UI cursor revision.
    std::uint64_t hostGeneration = 0;
};
inline Focus ResolveFocus(const Context& c) {
    if (c.suspensionReasons || c.ioBusy) return Focus::Blocked;
    if (c.dialoguePending) return c.uiAttached ? Focus::Dialogue : Focus::Blocked;
    if (c.battleBusy) return Focus::Battle;
    if (c.scriptBusy) return Focus::Blocked;
    if (c.uiModal) return c.uiAttached ? Focus::UI : Focus::Blocked;
    if (c.saveUsable && c.mapReady) return Focus::World;
    return c.uiAttached ? Focus::RecoveryUI : Focus::Blocked;
}
struct Axis {
    bool valid = false, active = false;
    Action action = Action::Up;
};
inline Axis Quantize(double x, double y) {
    if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x)>1.0 || std::abs(y)>1.0)
        return {};
    if (std::abs(x)<=.25 && std::abs(y)<=.25) return {true,false,Action::Up};
    if (std::abs(x)>std::abs(y))
        return {true,true,x>0 ? Action::Right : Action::Left};
    return {true,true,y>0 ? Action::Up : Action::Down};
}

class Router {
public:
    static constexpr unsigned Capacity = 64;
    explicit Router(std::uint64_t initialEpoch = 1)
        : epoch(initialEpoch), exhausted(initialEpoch==0) {}
    std::uint64_t Epoch() const { return epoch; }
    Focus CurrentFocus() const { return exhausted ? Focus::Blocked : ResolveFocus(context); }
    unsigned HeldCount(Action action) const {
        return Valid(action) ? counts[Index(action)] : 0;
    }
    bool Held(Action action) const { return HeldCount(action)!=0; }

    // Clear first, then fence. Never wrap a generation into a previously valid epoch.
    bool Reset() {
        for (auto& slot : slots) slot.active=false;
        counts.fill(0);
        if (exhausted || epoch==std::numeric_limits<std::uint64_t>::max()) {
            exhausted=true;
            return false;
        }
        ++epoch;
        return true;
    }
    bool SetContext(const Context& next) {
        const bool changed = ResolveFocus(next)!=ResolveFocus(context)
            || next.hostGeneration!=context.hostGeneration
            || next.suspensionReasons!=context.suspensionReasons
            || next.ioBusy!=context.ioBusy
            || next.uiAttached!=context.uiAttached
            || next.fieldAttached!=context.fieldAttached
            || next.battleAttached!=context.battleAttached;
        context=next;
        return !changed ? !exhausted : Reset();
    }

    Result Submit(const Event& event) {
        if (exhausted) return {Status::Exhausted};
        if (!Valid(event.action) || static_cast<unsigned>(event.source)>=static_cast<unsigned>(Source::Count)
            || static_cast<unsigned>(event.phase)>static_cast<unsigned>(Phase::Canceled))
            return {Status::Invalid};
        if (!event.epoch || event.epoch!=epoch) return {Status::Stale};

        Slot* match=nullptr;
        Slot* free=nullptr;
        for (auto& slot : slots) {
            if (!slot.active) { if (!free) free=&slot; continue; }
            if (slot.source==event.source && slot.device==event.device && slot.control==event.control)
                match=&slot;
        }
        if (match && match->action!=event.action) return {Status::Invalid};
        if (event.phase!=Phase::Pressed) {
            if (!match) return {Status::Ignored};
            --counts[Index(match->action)];
            match->active=false;
            return {Status::Updated,Target::None,event.action};
        }
        if (match) return {Status::Ignored};
        Result routed=Route(event.action);
        if (routed.status!=Status::Dispatch) return routed;
        if (!free) return {Status::Full};

        const bool first=counts[Index(event.action)]==0;
        *free={true,event.source,event.device,event.control,event.action};
        ++counts[Index(event.action)];
        if (!first) return {Status::Updated,Target::None,event.action};
        if (routed.target==Target::WorldStep) {
            // Pinned digital direction priority. A release never dispatches a substitute.
            for (unsigned i=0;i<4;++i) {
                if (!counts[i]) continue;
                if (i!=Index(event.action)) return {Status::Updated,Target::None,event.action};
                break;
            }
        }
        return routed;
    }

private:
    struct Slot {
        bool active = false;
        Source source = Source::Keyboard;
        std::uint16_t device = 0, control = 0;
        Action action = Action::Up;
    };
    static unsigned Index(Action action) { return static_cast<unsigned>(action); }
    static bool Valid(Action action) { return Index(action)<Index(Action::Count); }
    Result Route(Action action) const {
        const Focus focus=CurrentFocus();
        if (focus==Focus::Blocked) return {Status::Blocked};
        if (focus==Focus::UI || focus==Focus::Dialogue || focus==Focus::RecoveryUI)
            return context.uiAttached ? Result{Status::Dispatch,Target::UI,action}
                : Result{Status::Unsupported};
        if (focus==Focus::Battle) {
            if (Index(action)>Index(Action::Cancel)) return {Status::Blocked};
            return context.battleAttached ? Result{Status::Dispatch,Target::BattleInput,action}
                : Result{Status::Unsupported};
        }
        if (Index(action)<4) return {Status::Dispatch,Target::WorldStep,action};
        if (action==Action::Confirm)
            return context.fieldAttached ? Result{Status::Dispatch,Target::FieldInteract,action}
                : Result{Status::Unsupported};
        return context.uiAttached ? Result{Status::Dispatch,Target::UI,action}
            : Result{Status::Unsupported};
    }
    Context context;
    std::array<Slot,Capacity> slots{};
    std::array<unsigned,static_cast<unsigned>(Action::Count)> counts{};
    std::uint64_t epoch;
    bool exhausted;
};
} // namespace RemasterControls

