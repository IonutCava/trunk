#include "stdafx.h"

#include "Headers/BoundsComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

BoundsComponent::BoundsComponent(SceneGraphNode& sgn)
    : SGNComponent(SGNComponent::ComponentType::BOUNDS, sgn),
    _boundingBoxDirty(true),
    _lockBBTransforms(false)
{
}

BoundsComponent::~BoundsComponent()
{
}

void BoundsComponent::onTransform(const mat4<F32>& worldMatrix) {
    _worldMatrix.set(worldMatrix);
    flagBoundingBoxDirty();
}

void BoundsComponent::onBoundsChange(const BoundingBox& nodeBounds) {
    _refBoundingBox.set(nodeBounds);
    flagBoundingBoxDirty();
}

void BoundsComponent::update(const U64 deltaTime) {
    if (_boundingBoxDirty) {
        SceneGraphNode_ptr parent = _parentSGN.getParent().lock();
        if (parent) {
            parent->get<BoundsComponent>()->flagBoundingBoxDirty();
        }

        _boundingBox.set(_refBoundingBox);

        _parentSGN.forEachChild([this](const SceneGraphNode& child) {
            _boundingBox.add(child.get<BoundsComponent>()->getBoundingBox());
        });

        if (!_lockBBTransforms) {
            _boundingBox.transform(_worldMatrix);
        }

        _boundingSphere.fromBoundingBox(_boundingBox);
        _boundingBoxDirty = false;
    }
}

};