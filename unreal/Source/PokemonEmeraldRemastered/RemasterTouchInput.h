#pragma once
#include "RemasterInputRouter.h"
#include <array>

namespace RemasterControls {
struct Rect {
    double x0=0,y0=0,x1=0,y1=0;
    bool Valid() const {
        return std::isfinite(x0)&&std::isfinite(y0)&&std::isfinite(x1)&&std::isfinite(y1)
            && x0>=0&&y0>=0&&x1<=1&&y1<=1&&x0<x1&&y0<y1;
    }
    bool Contains(double x,double y) const { return x>=x0&&x<x1&&y>=y0&&y<y1; }
};
struct Layout {
    std::array<Rect,10> rects{};
    static Layout Default() {
        return {{{{.12,.56,.22,.69},{.12,.81,.22,.94},{.02,.69,.12,.81},
            {.22,.69,.32,.81},{.84,.66,.97,.83},{.70,.75,.83,.92},
            {.88,.08,.98,.20},{.65,.08,.75,.20},{.76,.08,.86,.20},{.54,.08,.64,.20}}}};
    }
    bool Valid() const {
        for (unsigned i=0;i<rects.size();++i) {
            const auto& a=rects[i];
            if (!a.Valid()) return false;
            for (unsigned j=0;j<i;++j) {
                const auto& b=rects[j];
                if (a.x0<b.x1&&b.x0<a.x1&&a.y0<b.y1&&b.y0<a.y1) return false;
            }
        }
        return true;
    }
};
struct Insets { double left=0,top=0,right=0,bottom=0; };
struct Point { bool valid=false;double x=0,y=0; };
inline Point NormalizePixel(double x,double y,double width,double height,Insets insets) {
    for (double v : {x,y,width,height,insets.left,insets.top,insets.right,insets.bottom})
        if (!std::isfinite(v)) return {};
    if (width<=0||height<=0||insets.left<0||insets.top<0||insets.right<0||insets.bottom<0)
        return {};
    const double usableW=width-insets.left-insets.right,usableH=height-insets.top-insets.bottom;
    if (usableW<=0||usableH<=0) return {};
    x=(x-insets.left)/usableW;y=(y-insets.top)/usableH;
    return {x>=0&&x<=1&&y>=0&&y<=1,x,y};
}
struct TouchPacket { bool valid=true,hasEvent=false;Event event{}; };
class Touch {
public:
    static constexpr unsigned FingerCapacity=10;
    const Layout& GetLayout() const { return layout; }
    bool SetLayout(const Layout& next) {
        if (!next.Valid()) return false;
        layout=next;Reset();return true;
    }
    void Reset() { for(auto& capture:captures) capture.active=false; }
    TouchPacket Press(unsigned finger,double x,double y,std::uint64_t epoch) {
        if (!Sync(epoch)||finger>=FingerCapacity||!FinitePoint(x,y)) return {false};
        if (captures[finger].active) return {};
        for (unsigned i=0;i<layout.rects.size();++i) if(layout.rects[i].Contains(x,y)) {
            captures[finger]={true,static_cast<Action>(i)};
            return Packet(finger,Phase::Pressed);
        }
        return {};
    }
    TouchPacket Move(unsigned finger,double x,double y,std::uint64_t epoch) {
        if (!Sync(epoch)||finger>=FingerCapacity) return {false};
        if (!captures[finger].active) return {};
        if (FinitePoint(x,y)&&layout.rects[static_cast<unsigned>(captures[finger].action)].Contains(x,y))
            return {};
        return Finish(finger,Phase::Canceled);
    }
    TouchPacket Release(unsigned finger,std::uint64_t epoch) {
        if (!Sync(epoch)||finger>=FingerCapacity) return {false};
        return Finish(finger,Phase::Released);
    }
    TouchPacket Cancel(unsigned finger,std::uint64_t epoch) {
        if (!Sync(epoch)||finger>=FingerCapacity) return {false};
        return Finish(finger,Phase::Canceled);
    }
private:
    struct Capture { bool active=false;Action action=Action::Up; };
    static bool FinitePoint(double x,double y) {
        return std::isfinite(x)&&std::isfinite(y)&&x>=0&&x<=1&&y>=0&&y<=1;
    }
    bool Sync(std::uint64_t epoch) {
        if (!epoch || epoch<currentEpoch) return false;
        if (epoch!=currentEpoch) { Reset();currentEpoch=epoch; }
        return true;
    }
    TouchPacket Packet(unsigned finger,Phase phase) const {
        return {true,true,{Source::Touch,0,static_cast<std::uint16_t>(finger),
            captures[finger].action,phase,currentEpoch}};
    }
    TouchPacket Finish(unsigned finger,Phase phase) {
        if (!captures[finger].active) return {};
        const auto packet=Packet(finger,phase);
        captures[finger].active=false;
        return packet;
    }
    Layout layout=Layout::Default();
    std::array<Capture,FingerCapacity> captures{};
    std::uint64_t currentEpoch=0;
};
} // namespace RemasterControls

