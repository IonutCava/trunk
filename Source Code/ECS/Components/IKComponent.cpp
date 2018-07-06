#include "stdafx.h"

#include "Headers/IKComponent.h"

namespace Divide {

IKComponent::IKComponent(SceneGraphNode& parentSGN)
 : SGNComponent(parentSGN, getComponentTypeName(ComponentType::INVERSE_KINEMATICS))
{
}

};