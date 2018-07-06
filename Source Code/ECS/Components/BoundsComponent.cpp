#include "stdafx.h"

#include "Headers/BoundsComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/BoundsEvents.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {

BoundsComponent::BoundsComponent(SceneGraphNode& sgn)
    : SGNComponent(sgn, "BOUNDS"),
     _boundingBoxDirty(true),
     _ignoreTransform(false)
{
    RegisterEventCallback(&BoundsComponent::onTransformUpdated);
    _editorComponent.registerField("BoundingBox", &_boundingBox, EditorComponentFieldType::BOUNDING_BOX, true);
    _editorComponent.registerField("Ref BoundingBox", &_refBoundingBox, EditorComponentFieldType::BOUNDING_BOX, true);
    _editorComponent.registerField("BoundingSphere", &_boundingSphere, EditorComponentFieldType::BOUNDING_SPHERE, true);
}

BoundsComponent::~BoundsComponent()
{
    UnregisterAllEventCallbacks();
}

void BoundsComponent::flagBoundingBoxDirty() {
    _boundingBoxDirty = true;
    SceneGraphNode* parent = _parentSGN.getParent();
    if (parent != nullptr) {
        BoundsComponent* bounds = parent->get<BoundsComponent>();
        // We stop if the parent sgn doesn't have a bounds component.
        if (bounds != nullptr) {
            bounds->flagBoundingBoxDirty();
        }
    }
}

void BoundsComponent::onTransformUpdated(const TransformUpdated* event) {
    if (GetOwner() == event->ownerID) {
        flagBoundingBoxDirty();
    }
}

void BoundsComponent::onBoundsChange(const BoundingBox& nodeBounds) {
    _refBoundingBox.set(nodeBounds);
    _boundsChanged = false;
    flagBoundingBoxDirty();
}

const BoundingBox& BoundsComponent::updateAndGetBoundingBox() {
    if (_boundingBoxDirty) {
        _boundingBox.set(_refBoundingBox);

        _parentSGN.forEachChild([this](const SceneGraphNode& child) {
            _boundingBox.add(child.get<BoundsComponent>()->updateAndGetBoundingBox());
        });

        if (!_ignoreTransform) {
            TransformComponent* tComp = _parentSGN.get<TransformComponent>();
            if (tComp) {
                _boundingBox.transform(tComp->getWorldMatrix());
            }
        }

        _boundingSphere.fromBoundingBox(_boundingBox);
        _parentSGN.SendEvent<BoundsUpdated>(GetOwner());
        _boundingBoxDirty = false;
    }
    return _boundingBox;
}

void BoundsComponent::Update(const U64 deltaTimeUS) {
    updateAndGetBoundingBox();

    SGNComponent<BoundsComponent>::Update(deltaTimeUS);
}

};