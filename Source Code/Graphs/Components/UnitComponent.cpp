#include "Headers/UnitComponent.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/Unit.h"

namespace Divide {
UnitComponent::UnitComponent(SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::UNIT, parentSGN)
{
}

UnitComponent::~UnitComponent()
{
    _unit.reset();
}

bool UnitComponent::setUnit(Unit_ptr unit) {
    if (!_unit) {
        _unit = unit;
        Attorney::UnitComponent::setParentNode(*_unit, _parentSGN.shared_from_this());
        return true;
    }

    return false;
}

}; //namespace Divide