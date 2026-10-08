#include "RemasterTouchInput.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace RemasterControls;
static unsigned checks=0;
static void Check(bool ok,const char* reason) {
    ++checks;
    if (!ok) { std::cerr<<reason<<"\n";std::exit(1); }
}
int main() {
    Touch touch;
    const double centers[10][2]={{.17,.625},{.17,.875},{.07,.75},{.27,.75},
        {.905,.745},{.765,.835},{.93,.14},{.70,.14},{.81,.14},{.59,.14}};
    Router router;Context world;world.mapReady=world.saveUsable=world.uiAttached=world.fieldAttached=true;
    router.SetContext(world);
    for(unsigned i=0;i<10;++i) {
        auto p=touch.Press(i,centers[i][0],centers[i][1],router.Epoch());
        Check(p.valid&&p.hasEvent&&p.event.action==static_cast<Action>(i), "ten independent touch centers");
        Check(p.event.source==Source::Touch&&p.event.control==i&&p.event.epoch==router.Epoch(),
              "finger identity and epoch preserved");
        Check(router.Submit(p.event).status==Status::Dispatch,"touch enters same common router");
        auto repeated=touch.Press(i,centers[i][0],centers[i][1],router.Epoch());
        Check(repeated.valid&&!repeated.hasEvent,"duplicate touch press suppressed");
        auto released=touch.Release(i,router.Epoch());
        Check(released.hasEvent&&released.event.phase==Phase::Released,"matching release");
        Check(router.Submit(released.event).target==Target::None,"touch release never dispatches command");
    }
    auto p=touch.Press(0,.17,.625,router.Epoch());
    router.Submit(p.event);
    Check(!touch.Move(0,.18,.64,router.Epoch()).hasEvent,"move inside capture does not repeat");
    auto canceled=touch.Move(0,.905,.745,router.Epoch());
    Check(canceled.hasEvent&&canceled.event.action==Action::Up&&canceled.event.phase==Phase::Canceled,
          "slide to another control cancels original instead of retargeting");
    router.Submit(canceled.event);
    Check(!router.Held(Action::Up)&&!router.Held(Action::Confirm),"slide has no replacement action");
    Check(!touch.Move(0,.905,.745,router.Epoch()).hasEvent,"drag cannot reacquire");
    Check(!touch.Release(0,router.Epoch()).hasEvent,"release after cancel harmless");
    Check(!touch.Press(10,.17,.625,router.Epoch()).valid,"finger overflow rejected");
    Check(!touch.Press(0,std::numeric_limits<double>::quiet_NaN(),.625,router.Epoch()).valid,
          "nonfinite touch rejected");
    Check(!touch.Press(0,.17,.625,0).valid,"zero epoch rejected");
    Check(!touch.Press(0,.50,.50,router.Epoch()).hasEvent,"empty space no action");
    Check(touch.Press(0,.12,.56,router.Epoch()).hasEvent,"half-open lower edge included");
    touch.Release(0,router.Epoch());
    Check(!touch.Press(0,.22,.625,router.Epoch()).hasEvent,"half-open upper edge excluded");

    p=touch.Press(0,.17,.625,router.Epoch());router.Submit(p.event);
    auto oldEpoch=router.Epoch();router.Reset();
    Check(!touch.Move(0,.17,.625,router.Epoch()).hasEvent,"epoch change discards captured finger");
    Check(!touch.Release(0,router.Epoch()).hasEvent,"stale capture release cannot affect next epoch");
    Check(router.Submit(p.event).status==Status::Stale,"old queued touch press fenced");
    Check(oldEpoch!=router.Epoch(),"actual generation advanced");
    Check(touch.Press(0,.905,.745,router.Epoch()).hasEvent,"fresh physical press can rearm");
    Check(!touch.Press(1,.17,.625,oldEpoch).valid,"older source epoch cannot replace new capture");
    Check(touch.Release(0,router.Epoch()).hasEvent,"stale source event preserves current finger capture");
    touch.Reset();
    Check(!touch.Release(0,router.Epoch()).hasEvent,"explicit reset clears captures");

    auto point=NormalizePixel(220,155,1000,600,{100,50,100,50});
    Check(point.valid&&point.x==.15&&point.y==.21,"pixel/safe-inset normalization oracle");
    Check(!NormalizePixel(1,1,0,600,{}).valid,"zero viewport rejected");
    Check(!NormalizePixel(1,1,100,100,{60,0,60,0}).valid,"invalid usable extent rejected");
    Check(!NormalizePixel(1,1,100,100,{-1,0,0,0}).valid,"negative insets rejected");
    Check(!NormalizePixel(0,0,1000,600,{100,50,100,50}).valid,"safe inset cannot capture touch");

    Layout bad=Layout::Default();bad.rects[1]=bad.rects[0];
    Check(!touch.SetLayout(bad),"overlapping override rejected atomically");
    Check(touch.Press(0,.17,.875,router.Epoch()).event.action==Action::Down,
          "invalid override preserves prior layout");
    touch.Reset();
    bad=Layout::Default();bad.rects[2].x0=std::numeric_limits<double>::infinity();
    Check(!touch.SetLayout(bad),"nonfinite geometry rejected");
    bad=Layout::Default();bad.rects[0].x1=bad.rects[0].x0;
    Check(!touch.SetLayout(bad),"zero-size geometry rejected");
    std::cout<<checks<<" R8 touch input checks passed\n";
}

