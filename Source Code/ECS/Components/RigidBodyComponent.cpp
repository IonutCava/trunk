#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"

#include "Physics/Headers/PXDevice.h"
#include "Headers/RigidBodyComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
    RigidBodyComponent::RigidBodyComponent(SceneGraphNode* parentSGN, PlatformContext& context)
        : Parent(parentSGN, context),
          _physicsCollisionGroup(parentSGN->usageContext() == NodeUsageContext::NODE_STATIC ? PhysicsGroup::GROUP_STATIC : PhysicsGroup::GROUP_DYNAMIC)
    {
    }

    RigidBodyComponent::~RigidBodyComponent()
    {
    }

    void RigidBodyComponent::physicsCollisionGroup(const PhysicsGroup group) {
        _physicsCollisionGroup = group;
        if (_rigidBody != nullptr) {
            _rigidBody->physicsCollisionGroup(group);
        }
    }

    bool RigidBodyComponent::filterCollision(const RigidBodyComponent& collider) {
        ACKNOWLEDGE_UNUSED(collider);
        // filter by mask, type, etc
        return true;
    }

    void RigidBodyComponent::onCollision(const RigidBodyComponent& collider) {
        //handle collision
        if (_collisionCbk) {
            if (filterCollision(collider)) {
                assert(getSGN()->getGUID() != collider.getSGN()->getGUID());
                _collisionCbk(collider);
            }
        }
    }
} //namespace