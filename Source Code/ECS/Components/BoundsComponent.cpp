#include "stdafx.h"

#include "Headers/BoundsComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {

BoundsComponent::BoundsComponent(SceneGraphNode& sgn)
    : SGNComponent(sgn),
    _boundingBoxDirty(true),
    _lockBBTransforms(false)
{
    RegisterEventCallback(&BoundsComponent::OnTransformDirty);
}

BoundsComponent::~BoundsComponent()
{
    UnregisterAllEventCallbacks();
}

void BoundsComponent::OnTransformDirty(const TransformDirty* event) {
    if (GetOwner() == event->ownerID) {
        TransformComponent* tComp = 
            ECS::ECS_Engine->GetComponentManager()->GetComponent<TransformComponent>(event->ownerID);
        if (tComp) {
            onTransform(tComp->getWorldMatrix());
        }
    }
}

void BoundsComponent::onTransform(const mat4<F32>& worldMatrix) {
    _worldMatrix.set(worldMatrix);
    flagBoundingBoxDirty();
}

void BoundsComponent::onBoundsChange(const BoundingBox& nodeBounds) {
    _refBoundingBox.set(nodeBounds);
    flagBoundingBoxDirty();
}

void BoundsComponent::update(const U64 deltaTimeUS) {
    if (_boundingBoxDirty) {
        SceneGraphNode* parent = _parentSGN.getParent();
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