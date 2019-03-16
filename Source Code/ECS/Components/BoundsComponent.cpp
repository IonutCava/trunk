#include "stdafx.h"

#include "Headers/BoundsComponent.h"
#include "Headers/TransformComponent.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/BoundsEvents.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {

BoundsComponent::BoundsComponent(SceneGraphNode& sgn, PlatformContext& context)
    : BaseComponentType<BoundsComponent, ComponentType::BOUNDS>(sgn, context),
     _ignoreTransform(false)
{
    _refBoundingBox.set(sgn.getNode().getBounds());
    _boundingBox.set(_refBoundingBox);
    _boundingSphere.fromBoundingBox(_boundingBox);

    _boundingBoxNotDirty.clear();

    RegisterEventCallback(&BoundsComponent::onTransformUpdated);
    _editorComponent.registerField("BoundingBox", &_boundingBox, EditorComponentFieldType::BOUNDING_BOX, true);
    _editorComponent.registerField("Ref BoundingBox", &_refBoundingBox, EditorComponentFieldType::BOUNDING_BOX, true);
    _editorComponent.registerField("BoundingSphere", &_boundingSphere, EditorComponentFieldType::BOUNDING_SPHERE, true);
}

BoundsComponent::~BoundsComponent()
{
    UnregisterAllEventCallbacks();
}

void BoundsComponent::flagBoundingBoxDirty(bool recursive) {
    _boundingBoxNotDirty.clear();
    if (recursive) {
        SceneGraphNode* parent = _parentSGN.getParent();
        if (parent != nullptr) {
            BoundsComponent* bounds = parent->get<BoundsComponent>();
            // We stop if the parent sgn doesn't have a bounds component.
            if (bounds != nullptr) {
                bounds->flagBoundingBoxDirty(true);
            }
        }
    }
}

void BoundsComponent::onTransformUpdated(const TransformUpdated* event) {
    if (GetOwner() == event->ownerID) {
        flagBoundingBoxDirty(true);
    }
}

void BoundsComponent::setRefBoundingBox(const BoundingBox& nodeBounds) {
    // All the parents should already be dirty thanks to the bounds system
    _refBoundingBox.set(nodeBounds);
    _boundingBoxNotDirty.clear();
}

const BoundingBox& BoundsComponent::updateAndGetBoundingBox() {
    if (!_boundingBoxNotDirty.test_and_set()) {
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
    }

    return _boundingBox;
}

void BoundsComponent::Update(const U64 deltaTimeUS) {
    BaseComponentType<BoundsComponent, ComponentType::BOUNDS>::Update(deltaTimeUS);
}

void BoundsComponent::PostUpdate(const U64 deltaTimeUS) {
    updateAndGetBoundingBox();

    BaseComponentType<BoundsComponent, ComponentType::BOUNDS>::PostUpdate(deltaTimeUS);
}

};