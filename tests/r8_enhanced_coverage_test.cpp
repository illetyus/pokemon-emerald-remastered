#include "RemasterEnhancedCoverage.h"
#include <cstdlib>
#include <iostream>
using namespace RemasterControls;
static unsigned checks=0;
static void Check(bool ok,const char* why){++checks;if(!ok){std::cerr<<why<<"\n";std::exit(1);}}
int main(){
    EnhancedCoverage coverage;
    Check(coverage.Add({"W"},Action::Up,true),"partial keyboard Move accepted");
    Check(coverage.Covered(Bindings[1])&&!coverage.Covered(Bindings[0]),"only mapped W suppresses native W");
    for(unsigned i=15;i<25;++i)Check(!coverage.Covered(Bindings[i]),"keyboard Enhanced action cannot remove gamepad equivalents");
    Check(!coverage.StickCovered(),"keyboard axis cannot suppress native left stick");
    Check(coverage.Add({"Enter"},Action::Confirm),"partial confirm accepted");
    Check(coverage.Covered(Bindings[8])&&!coverage.Covered(Bindings[9])&&!coverage.Covered(Bindings[19]),
          "mapped Enter retains Space and gamepad confirm");
    Check(!coverage.Add({"SpaceBar","M"},Action::Confirm),"reserved other-action key rejects entire asset");
    Check(!coverage.Covered(Bindings[9])&&!coverage.Covered(Bindings[12]),"failed mapping validation is atomic");
    Check(!coverage.Add({"Gamepad_LeftX"},Action::Up,true)&&!coverage.StickCovered(),"partial stick asset rejected atomically");
    Check(coverage.Add({"Gamepad_LeftX","Gamepad_LeftY"},Action::Up,true)&&coverage.StickCovered(),"both mapped axes replace paired native stick");
    EnhancedCoverage full;Check(full.Add({"Gamepad_Left2D"},Action::Up,true)&&full.StickCovered(),"2D mapped stick replaces native poll");
    Check(!full.Add({"Gamepad_Left2D"},Action::Confirm),"axis key cannot masquerade as reserved confirm mapping");
    Check(!full.Add({},Action::Confirm),"empty mapping cannot suppress any fallback");
    Check(!full.Add({"Touch1"},Action::Confirm),"Enhanced touch key cannot duplicate native positional capture");
    Check(!full.Add({"Enter"},Action::Count),"invalid semantic fails closed");
    Check(full.Add({"Gamepad_DPad_Up","Gamepad_DPad_Down","Gamepad_DPad_Left","Gamepad_DPad_Right"},Action::Up,true),
          "complete gamepad movement mapping accepted");
    for(unsigned i=0;i<15;++i)Check(!full.Covered(Bindings[i]),"gamepad Enhanced action retains all native keyboard equivalents");
    for(unsigned i=15;i<19;++i)Check(full.Covered(Bindings[i]),"mapped gamepad Dpad has no duplicate native binding");
    std::cout<<checks<<" R8 partial Enhanced coverage checks passed\n";
}
