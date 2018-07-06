#include "Headers/PhysicsAPIWrapper.h"

#include "Graphs/Components/Headers/PhysicsComponent.h"

namespace Divide {

PhysicsAsset::PhysicsAsset()
    : _resetTransforms(true),
    _parentComponent(nullptr)
{
}

PhysicsAsset::~PhysicsAsset()
{
    setParent(nullptr);
}

void PhysicsAsset::setParent(PhysicsComponent* parent) {
    if (_parentComponent != nullptr) {
        Attorney::PXCompPXAsset::setPhysicsAsset(*_parentComponent, nullptr);
    }
    _parentComponent = parent;

    if (_parentComponent) {
        Attorney::PXCompPXAsset::setPhysicsAsset(*_parentComponent, this);
    }
}

};