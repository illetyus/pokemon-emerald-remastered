#pragma once
#include "RemasterPhysicalInput.h"
#include "RemasterTouchInput.h"

namespace RemasterControls {
enum class Suspension : std::uint8_t { Background=1,Inactive=2,Paused=4 };
class Lifecycle {
public:
    std::uint8_t Reasons() const {return reasons;}
    bool Set(Suspension reason,bool suspended) {
        const auto mask=static_cast<std::uint8_t>(reason);
        if(mask!=1&&mask!=2&&mask!=4)return false;
        const auto next=static_cast<std::uint8_t>(suspended ? reasons|mask : reasons&~mask);
        if(next==reasons)return false;
        reasons=next;return true;
    }
private:
    std::uint8_t reasons=0;
};
inline void FenceSources(Router& router,Touch& touch,Digital& digital,Analog& enhanced,Analog& gamepad) {
    router.Reset();touch.Reset();digital.Reset();enhanced.Fence();gamepad.Fence();
}
} // namespace RemasterControls
