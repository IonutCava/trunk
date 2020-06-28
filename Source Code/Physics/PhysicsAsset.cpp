#include "stdafx.h"

#include "Headers/PhysicsAsset.h"

namespace Divide {
PhysicsAsset::PhysicsAsset(RigidBodyComponent& parent)
    : _parentComponent(parent)
{
}

}; //namespace Divide