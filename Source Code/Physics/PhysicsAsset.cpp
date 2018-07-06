#include "Headers/PhysicsAsset.h"

namespace Divide {
PhysicsAsset::PhysicsAsset(PhysicsComponent& parent)
    : TransformInterface(),
      _parentComponent(parent)
{
}

PhysicsAsset::~PhysicsAsset()
{
}

}; //namespace Divide