#include "stdafx.h"

#include "Physics/Headers/PXDevice.h"
#include "Headers/RigidBodyComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
    RigidBodyComponent::RigidBodyComponent(SceneGraphNode& parentSGN, PhysicsGroup physicsGroup, PXDevice& context)
        : SGNComponent(parentSGN),
          _physicsCollisionGroup(physicsGroup)
    {
        _rigidBody.reset(context.createRigidActor(parentSGN, *this));
    }

    RigidBodyComponent::~RigidBodyComponent()
    {

    }

    void RigidBodyComponent::cookCollisionMesh(const stringImpl& sceneName) {

    }

    bool RigidBodyComponent::filterCollission(const RigidBodyComponent& collider) {
        // filter by mask, type, etc
        return true;
    }

    void RigidBodyComponent::onCollision(const RigidBodyComponent& collider) {
        //handle collision
        if (_collisionCbk) {
            if (filterCollission(collider)) {
                assert(getSGN().getGUID() != collider.getSGN().getGUID());
                _collisionCbk(collider);
            }
        }
    }
}; //namespace