#include "stdafx.h"

#include "Physics/Headers/PXDevice.h"
#include "Headers/RigidBodyComponent.h"

namespace Divide {
    RigidBodyComponent::RigidBodyComponent(PhysicsGroup physicsGroup, PXDevice& context)
        : _physicsCollisionGroup(physicsGroup)
    {
        //_transformInterface.reset(context.createRigidActor(parentSGN));
    }

    RigidBodyComponent::~RigidBodyComponent()
    {

    }

    void RigidBodyComponent::cookCollisionMesh(const stringImpl& sceneName) {

    }
}; //namespace