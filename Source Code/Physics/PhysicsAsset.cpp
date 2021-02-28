#include "stdafx.h"

#include "Headers/PhysicsAsset.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"

namespace Divide {
PhysicsAsset::PhysicsAsset(RigidBodyComponent& parent)
    : _parentComponent(parent),
      _physicsGroup(PhysicsGroup::GROUP_COUNT)
{
}

void PhysicsAsset::physicsCollisionGroup(const PhysicsGroup group) {
    _physicsGroup = group;
}

}; //namespace Divide