#include "Headers/PhysicsAPIWrapper.h"

#include "Graphs/Components/Headers/PhysicsComponent.h"

PhysicsAsset::PhysicsAsset() : _resetTransforms(true),
                               _parentComponent(nullptr)
{
}
PhysicsAsset::~PhysicsAsset()
{
    setParent(nullptr);
}

void PhysicsAsset::setParent(PhysicsComponent* parent) {
    if (_parentComponent != nullptr) {
        _parentComponent->physicsAsset(nullptr);
    }
    _parentComponent = parent;
}