#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"

#include "Physics/Headers/PXDevice.h"
#include "Headers/RigidBodyComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
    RigidBodyComponent::RigidBodyComponent(SceneGraphNode& parentSGN, PlatformContext& context)
        : BaseComponentType<RigidBodyComponent, ComponentType::RIGID_BODY>(parentSGN, context),
          _physicsCollisionGroup(PhysicsGroup::GROUP_STATIC)
    {
        _rigidBody.reset(context.pfx().createRigidActor(parentSGN, *this));
    }

    RigidBodyComponent::~RigidBodyComponent()
    {

    }

    void RigidBodyComponent::cookCollisionMesh(const Str64& sceneName) {

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