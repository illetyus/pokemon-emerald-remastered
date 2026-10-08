#pragma once
#include "RemasterPhysicalInput.h"
#include <string>
#include <vector>

namespace RemasterControls {
class EnhancedCoverage {
public:
    bool Covered(const Binding& binding) const{return binding.control<covered.size()&&covered[binding.control];}
    bool StickCovered() const{return stickCovered;}
    bool Add(const std::vector<std::string>& keys,Action semantic,bool move=false){
        if(keys.empty()||static_cast<unsigned>(semantic)>=static_cast<unsigned>(Action::Count))return false;
        auto next=*this;unsigned axes=0;bool axis2D=false;
        for(const auto& key:keys){
            if(key.empty())return false;
            if(key.compare(0,5,"Touch")==0)return false; // Native positional touch owns capture.
            if(key=="Gamepad_Left2D"||key=="Gamepad_LeftX"||key=="Gamepad_LeftY"){
                if(!move)return false;
                if(key=="Gamepad_Left2D")axis2D=true;
                else axes|=key=="Gamepad_LeftX"?1:2;
            }
            for(const auto& binding:Bindings){
                if(key!=binding.engineKey)continue;
                if(move?static_cast<unsigned>(binding.action)>=4:binding.action!=semantic)return false;
                next.covered[binding.control]=true;
            }
        }
        // An aggregate Enhanced 2D callback cannot safely split one mapped native stick axis.
        if(axes&&!axis2D&&axes!=3)return false;
        next.stickCovered=next.stickCovered||axis2D||axes==3;
        *this=next;return true;
    }
private:
    std::array<bool,Bindings.size()> covered{};
    bool stickCovered=false;
};
} // namespace RemasterControls
