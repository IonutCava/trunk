#include "stdafx.h"

#include "Headers/UnitComponent.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/Unit.h"

namespace Divide {
UnitComponent::UnitComponent(SceneGraphNode& parentSGN, PlatformContext& context)
    : BaseComponentType<UnitComponent, ComponentType::UNIT>(parentSGN, context)
{
}

UnitComponent::~UnitComponent()
{
    _unit.reset();
}

bool UnitComponent::setUnit(Unit_ptr unit) {
    if (!_unit) {
        _unit = unit;
        Attorney::UnitComponent::setParentNode(*_unit, &_parentSGN);
        return true;
    }

    return false;
}

}; //namespace Divide