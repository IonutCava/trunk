#include "stdafx.h"

#include "Headers/SelectionComponent.h"

namespace Divide {

    SelectionComponent::SelectionComponent(SceneGraphNode& parentSGN)
        : SGNComponent(parentSGN, getComponentTypeName(ComponentType::SELECTION))
    {
    }

};