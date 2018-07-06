#include "Headers/RagdollComponent.h"

namespace Divide {

RagdollComponent::RagdollComponent(SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::RAGDOLL, parentSGN)
{
}

};