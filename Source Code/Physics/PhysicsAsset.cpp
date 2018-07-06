#include "stdafx.h"

#include "Headers/PhysicsAsset.h"

namespace Divide {
PhysicsAsset::PhysicsAsset(RigidBodyComponent& parent)
    : TransformInterface(),
      _parentComponent(parent)
{
}

PhysicsAsset::~PhysicsAsset()
{
}

}; //namespace Divide