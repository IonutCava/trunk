#include "stdafx.h"

#include "Headers/IKComponent.h"

namespace Divide {

IKComponent::IKComponent(SceneGraphNode& parentSGN)
 : SGNComponent(SGNComponent::ComponentType::INVERSE_KINEMATICS, parentSGN)
{
}

};