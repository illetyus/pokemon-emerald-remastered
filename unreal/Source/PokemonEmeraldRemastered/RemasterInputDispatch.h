#pragma once
#include "RemasterInputRouter.h"

namespace RemasterControls {
// One selected owner, one response. Rejection never falls through to another owner.
template<class UI,class World,class Field,class Battle>
bool Deliver(Action action,Target target,UI&& ui,World&& world,Field&& field,Battle&& battle) {
    const auto id=static_cast<unsigned>(action);
    if(id>=static_cast<unsigned>(Action::Count))return false;
    switch(target) {
    case Target::UI:return ui(action);
    case Target::WorldStep:return id<4&&world(action);
    case Target::FieldInteract:return action==Action::Confirm&&field(action);
    case Target::BattleInput:return id<=static_cast<unsigned>(Action::Cancel)&&battle(action);
    default:return false;
    }
}
} // namespace RemasterControls
