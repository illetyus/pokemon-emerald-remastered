#include "RemasterInputRouter.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using namespace RemasterControls;
static unsigned checks = 0;
static void Check(bool ok, const char* reason) {
    ++checks;
    if (!ok) { std::cerr << reason << "\n"; std::exit(1); }
}
static Context World() {
    Context c;
    c.mapReady = c.saveUsable = c.uiAttached = c.fieldAttached = true;
    return c;
}
static Event E(const Router& r, Action a, Phase p = Phase::Pressed,
               std::uint16_t control = 1, Source s = Source::Keyboard,
               std::uint16_t device = 0) {
    return {s, device, control, a, p, r.Epoch()};
}
static std::vector<Target> Trace(Source source) {
    Router r; r.SetContext(World());
    std::vector<Target> out;
    for (unsigned i = 0; i < 10; ++i) {
        auto a = static_cast<Action>(i);
        auto result = r.Submit(E(r,a,Phase::Pressed,static_cast<std::uint16_t>(i),source));
        Check(result.status == Status::Dispatch, "all world semantics route explicitly");
        out.push_back(result.target);
        auto released = r.Submit(E(r,a,Phase::Released,static_cast<std::uint16_t>(i),source));
        Check(released.target == Target::None && !r.Held(a), "release cannot dispatch");
    }
    return out;
}
int main() {
    const auto keyboard = Trace(Source::Keyboard);
    Check(keyboard == Trace(Source::Gamepad), "gamepad semantic trace equals keyboard");
    Check(keyboard == Trace(Source::Touch), "touch semantic trace equals keyboard");
    Check(keyboard == Trace(Source::Enhanced), "Enhanced semantic trace equals keyboard");
    const std::vector<Target> expected = {Target::WorldStep,Target::WorldStep,
        Target::WorldStep,Target::WorldStep,Target::FieldInteract,
        Target::UI,Target::UI,Target::UI,Target::UI,Target::UI};
    Check(keyboard == expected, "independent action-to-owner oracle");

    Router r; auto c = World(); r.SetContext(c);
    Check(r.Submit(E(r,Action::Up)).status == Status::Dispatch, "first press");
    Check(r.Submit(E(r,Action::Up)).status == Status::Ignored, "duplicate physical press");
    Check(r.Submit(E(r,Action::Up,Phase::Pressed,2,Source::Touch)).status == Status::Updated,
          "second source hold never duplicates command");
    Check(r.HeldCount(Action::Up) == 2, "distinct controls retain ownership");
    Check(r.Submit(E(r,Action::Down,Phase::Released)).status == Status::Invalid,
          "mismatched release rejected");
    Check(r.HeldCount(Action::Up) == 2, "mismatched release does not drop another hold");
    r.Submit(E(r,Action::Up,Phase::Released));
    Check(r.Held(Action::Up), "releasing keyboard retains touch hold");
    r.Submit(E(r,Action::Up,Phase::Canceled,2,Source::Touch));
    Check(!r.Held(Action::Up), "cancel clears final semantic hold");
    Check(r.Submit(E(r,Action::Up,Phase::Released)).status == Status::Ignored,
          "unmatched release is harmless");

    r.Submit(E(r,Action::Up));
    Check(r.Submit(E(r,Action::Down,Phase::Pressed,3)).status == Status::Updated,
          "source priority suppresses opposing lower-priority press");
    auto result = r.Submit(E(r,Action::Up,Phase::Released));
    Check(result.target == Target::None && r.Held(Action::Down),
          "priority release must not synthesize lower-priority movement");
    r.Reset();

    for (unsigned i=0;i<64;++i) {
        result=r.Submit(E(r,Action::Up,Phase::Pressed,static_cast<std::uint16_t>(i)));
        Check(result.status==(i ? Status::Updated:Status::Dispatch), "bounded hold insertion");
    }
    Check(r.Submit(E(r,Action::Confirm,Phase::Pressed,64)).status==Status::Full,
          "overflow rejects instead of evicting held key");
    Check(r.HeldCount(Action::Up)==64 && !r.Held(Action::Confirm), "overflow atomicity");
    r.Submit(E(r,Action::Up,Phase::Released,0));
    Check(r.Submit(E(r,Action::Confirm,Phase::Pressed,64)).status==Status::Dispatch,
          "release frees bounded capacity");
    auto invalid=E(r,Action::Confirm,Phase::Pressed,65);
    invalid.action=static_cast<Action>(255);
    Check(r.Submit(invalid).status==Status::Invalid, "invalid action");
    invalid=E(r,Action::Confirm,Phase::Pressed,65);invalid.source=static_cast<Source>(255);
    Check(r.Submit(invalid).status==Status::Invalid, "invalid source");
    invalid=E(r,Action::Confirm,Phase::Pressed,65);invalid.phase=static_cast<Phase>(255);
    Check(r.Submit(invalid).status==Status::Invalid, "invalid phase");

    const auto old=E(r,Action::Confirm,Phase::Released,64);
    c.uiModal=true; c.hostGeneration=11;
    r.SetContext(c);
    Check(r.CurrentFocus()==Focus::UI && !r.Held(Action::Up), "focus reset clears holds");
    Check(r.Submit(old).status==Status::Stale, "stale release cannot affect next owner");
    auto newer=E(r,Action::Confirm,Phase::Pressed,64);
    Check(r.Submit(newer).target==Target::UI, "fresh modal confirm routes only to UI");
    Check(r.Submit(old).status==Status::Stale && r.Held(Action::Confirm),
          "stale release cannot release new-owner press");
    const auto modalEpoch=r.Epoch();
    c.hostGeneration=12;r.SetContext(c);
    Check(r.Epoch()>modalEpoch && !r.Held(Action::Confirm), "same focus new host is fenced");
    c.suspensionReasons=1;r.SetContext(c);
    Check(r.Submit(E(r,Action::Up)).status==Status::Blocked && !r.Held(Action::Up),
          "suspension rejects without accumulating held commands");
    c.suspensionReasons=3;r.SetContext(c);
    c.suspensionReasons=2;r.SetContext(c);
    Check(r.CurrentFocus()==Focus::Blocked, "partial resume remains blocked");
    c.suspensionReasons=0;r.SetContext(c);
    Check(!r.Held(Action::Up), "resume does not replay hold");

    c=World();c.mapReady=c.saveUsable=false;r.SetContext(c);
    Check(r.CurrentFocus()==Focus::RecoveryUI &&
          r.Submit(E(r,Action::Menu)).target==Target::UI, "missing save retains recovery menu");
    r.Reset();
    Check(r.Submit(E(r,Action::Up)).target!=Target::WorldStep, "recovery cannot move world");
    c=World();c.scriptBusy=true;r.SetContext(c);
    Check(r.Submit(E(r,Action::Menu)).status==Status::Blocked, "script owner blocks menus");
    c.dialoguePending=true;r.SetContext(c);
    Check(r.CurrentFocus()==Focus::Dialogue &&
          r.Submit(E(r,Action::Confirm)).target==Target::UI, "attached dialogue precedes script busy");
    c.ioBusy=true;r.SetContext(c);
    Check(r.Submit(E(r,Action::Confirm)).status==Status::Blocked, "IO blocks even dialogue");
    c=World();c.battleBusy=true;c.battleAttached=false;r.SetContext(c);
    Check(r.Submit(E(r,Action::Confirm)).status==Status::Unsupported &&
          !r.Held(Action::Confirm), "missing battle host is explicit without hold");
    c.battleAttached=true;r.SetContext(c);
    Check(r.Submit(E(r,Action::Confirm)).target==Target::BattleInput, "attached battle owner");
    Check(r.Submit(E(r,Action::Map,Phase::Pressed,7)).status==Status::Blocked, "battle cannot open map");
    c=World();c.fieldAttached=false;r.SetContext(c);
    Check(r.Submit(E(r,Action::Confirm)).status==Status::Unsupported,
          "missing field host cannot masquerade as interaction");

    auto axis=Quantize(.25,0);
    Check(axis.valid&&!axis.active, "dead zone boundary neutral");
    axis=Quantize(.8,.8);Check(axis.active&&axis.action==Action::Up, "analog tie uses Y");
    axis=Quantize(-.9,.4);Check(axis.action==Action::Left, "dominant negative X");
    axis=Quantize(.1,-.9);Check(axis.action==Action::Down, "negative Y");
    Check(!Quantize(std::numeric_limits<double>::quiet_NaN(),0).valid, "NaN rejected");
    Check(!Quantize(0,std::numeric_limits<double>::infinity()).valid, "infinite axis rejected");
    Check(!Quantize(1.01,0).valid, "out-of-range axis rejected");

    Router exhausted(std::numeric_limits<std::uint64_t>::max());
    exhausted.SetContext(World());
    Check(exhausted.Submit(E(exhausted,Action::Up)).status==Status::Exhausted,
          "epoch exhaustion fails closed without wrap");
    Router zero(0);
    Check(zero.Submit(E(zero,Action::Up)).status==Status::Exhausted, "zero epoch fails closed");
    std::cout << checks << " R8 shared input router checks passed\n";
}

