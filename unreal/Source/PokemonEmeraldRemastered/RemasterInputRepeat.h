#pragma once
#include "RemasterInputRouter.h"
#include "RemasterUiModel.h"

namespace RemasterControls {
struct RepeatFrame{bool up=false,down=false;};
class UiRepeat {
public:
    void Reset(){up.held=down.held=0;epoch=0;}
    bool Sync(const Router& router,bool enabled){
        const auto focus=router.CurrentFocus();
        const bool changed=epoch!=router.Epoch()||focus!=lastFocus||enabled!=lastEnabled;
        if(changed){up.held=down.held=0;epoch=router.Epoch();lastFocus=focus;lastEnabled=enabled;}
        return changed;
    }
    RepeatFrame Tick(const Router& router,bool enabled){
        Sync(router,enabled);
        const auto focus=router.CurrentFocus();
        const bool eligible=focus==Focus::UI||focus==Focus::Dialogue;
        return{Step(up,eligible&&router.Held(Action::Up),enabled),
               Step(down,eligible&&router.Held(Action::Down),enabled)};
    }
private:
    static bool Step(RemasterUi::Repeat& repeat,bool pressed,bool enabled){
        const bool emitted=repeat.Tick(pressed,enabled);
        // Preserve R9's 24/3 cadence without an unbounded multi-year counter.
        if(repeat.held==27)repeat.held=24;
        return emitted;
    }
    RemasterUi::Repeat up,down;
    std::uint64_t epoch=0;
    Focus lastFocus=Focus::Blocked;
    bool lastEnabled=false;
};
} // namespace RemasterControls
