#include "stdafx.h"

#include "Headers/BoundsComponent.h"
#include "Headers/TransformComponent.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/BoundsEvents.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {

BoundsComponent::BoundsComponent(SceneGraphNode& sgn, PlatformContext& context)
    : BaseComponentType<BoundsComponent, ComponentType::BOUNDS>(sgn, context),
     _ignoreTransform(false),
     _tCompCache(sgn.get<TransformComponent>())
{
    _refBoundingBox.set(sgn.getNode().getBounds());
    _boundingBox.set(_refBoundingBox);
    _boundingSphere.fromBoundingBox(_boundingBox);

    _boundingBoxDirty.store(true);

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
    _boundingBoxDirty.store(true);

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

void BoundsComponent::onTransformUpdated(const TransformUpdated* evt) {
    if (GetOwner() == evt->ownerID) {
        flagBoundingBoxDirty(true);
    }
}

void BoundsComponent::setRefBoundingBox(const BoundingBox& nodeBounds) {
    // All the parents should already be dirty thanks to the bounds system
    _refBoundingBox.set(nodeBounds);
    _boundingBoxDirty.store(true);
}

const BoundingBox& BoundsComponent::updateAndGetBoundingBox() {
    bool expected = true;
    if (_boundingBoxDirty.compare_exchange_strong(expected, false)) {
        UniqueLock w_lock(_bbLock);

        _boundingBox.set(_refBoundingBox);

        _parentSGN.forEachChild([this](const SceneGraphNode& child) {
            _boundingBox.add(child.get<BoundsComponent>()->updateAndGetBoundingBox());
        });

        if (!_ignoreTransform && _tCompCache != nullptr) {
            _boundingBox.transform(_tCompCache->getWorldMatrix());
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