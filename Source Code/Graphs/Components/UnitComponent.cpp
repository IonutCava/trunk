#include "Headers/UnitComponent.h"

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
        return true;
    }

    return false;
}

}; //namespace Divide