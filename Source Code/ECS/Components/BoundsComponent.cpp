#include "stdafx.h"

#include "Headers/BoundsComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {

BoundsComponent::BoundsComponent(SceneGraphNode& sgn)
    : SGNComponent(sgn, "BOUNDS"),
     _transformDirty(true),
     _boundingBoxDirty(true),
     _lockBBTransforms(false)
{
    RegisterEventCallback(&BoundsComponent::OnTransformDirty);
    _editorComponent.registerField("BoundingBox", &_boundingBox, EditorComponentFieldType::BOUNDING_BOX);
    _editorComponent.registerField("Ref BoundingBox", &_refBoundingBox, EditorComponentFieldType::BOUNDING_BOX);
    _editorComponent.registerField("BoundingSphere", &_boundingSphere, EditorComponentFieldType::BOUNDING_SPHERE);
    _editorComponent.registerField("Lock BB Transform", &_lockBBTransforms, EditorComponentFieldType::PUSH_TYPE, GFX::PushConstantType::BOOL);
}

BoundsComponent::~BoundsComponent()
{
    UnregisterAllEventCallbacks();
}

void BoundsComponent::OnTransformDirty(const TransformDirty* event) {
    if (GetOwner() == event->ownerID) {
        _transformDirty = true;
    }
}

void BoundsComponent::onBoundsChange(const BoundingBox& nodeBounds) {
    _refBoundingBox.set(nodeBounds);
    flagBoundingBoxDirty();
}

void BoundsComponent::update(const U64 deltaTimeUS) {
    if (_transformDirty) {
        
        TransformComponent* tComp = _parentSGN.GetComponentManager()->GetComponent<TransformComponent>(GetOwner());
        if (tComp) {
            _worldMatrix.set(tComp->getWorldMatrix());
            _boundingBoxDirty = true;
        }
        _transformDirty = false;
    }

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