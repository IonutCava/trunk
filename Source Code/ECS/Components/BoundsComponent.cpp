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

    //RegisterEventCallback(&BoundsComponent::onTransformUpdated);
    EditorComponentField bbField = {};
    bbField._name = "Bounding Box";
    bbField._data = &_boundingBox;
    bbField._type = EditorComponentFieldType::BOUNDING_BOX;
    bbField._readOnly = true;
    _editorComponent.registerField(std::move(bbField));

    EditorComponentField rbbField = {};
    rbbField._name = "Ref Bounding Box";
    rbbField._data = &_refBoundingBox;
    rbbField._type = EditorComponentFieldType::BOUNDING_BOX;
    rbbField._readOnly = true;
    _editorComponent.registerField(std::move(rbbField));

    EditorComponentField bsField = {};
    bsField._name = "Bounding Sphere";
    bsField._data = &_boundingSphere;
    bsField._type = EditorComponentFieldType::BOUNDING_SPHERE;
    bsField._readOnly = true;
    _editorComponent.registerField(std::move(bsField));
}

BoundsComponent::~BoundsComponent()
{
    UnregisterAllEventCallbacks();
}

void BoundsComponent::flagBoundingBoxDirty(bool recursive) {
    OPTICK_EVENT();

    bool expected = false;
    if (_boundingBoxDirty.compare_exchange_strong(expected, true)) {

        if (recursive) {
            SceneGraphNode* parent = _parentSGN.parent();
            if (parent != nullptr) {
                BoundsComponent* bounds = parent->get<BoundsComponent>();
                // We stop if the parent sgn doesn't have a bounds component.
                if (bounds != nullptr) {
                    bounds->flagBoundingBoxDirty(true);
                }
            }
        }
    }
}

void BoundsComponent::OnData(const ECS::Data& data) {
    if (data.eventType == ECSCustomEventType::TransformUpdated) {
        flagBoundingBoxDirty(true);
    }
}

/*void BoundsComponent::onTransformUpdated(const TransformUpdated* evt) {
    if (GetOwner() != evt->GetSourceEntityId()) {
        return;
    }

    flagBoundingBoxDirty(true);
}*/

void BoundsComponent::setRefBoundingBox(const BoundingBox& nodeBounds) {
    // All the parents should already be dirty thanks to the bounds system
    _refBoundingBox.set(nodeBounds);
    _boundingBoxDirty.store(true);
}

const BoundingBox& BoundsComponent::updateAndGetBoundingBox() {
    OPTICK_EVENT();

    bool expected = true;
    if (_boundingBoxDirty.compare_exchange_strong(expected, false)) {
        BoundingBox tempBB(_refBoundingBox);
        _parentSGN.forEachChild([&tempBB](const SceneGraphNode* child, I32 /*childIdx*/) {
            tempBB.add(child->get<BoundsComponent>()->updateAndGetBoundingBox());
        });

        if (!_ignoreTransform && _tCompCache != nullptr) {
            tempBB.transform(_tCompCache->getWorldMatrix());
        }

        {
            UniqueLock w_lock(_bbLock);
            _boundingBox.set(tempBB);
            _boundingSphere.fromBoundingBox(_boundingBox);
        }
        _parentSGN.SendEvent<BoundsUpdated>(GetOwner());
    }

    return _boundingBox;
}

void BoundsComponent::Update(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    BaseComponentType<BoundsComponent, ComponentType::BOUNDS>::Update(deltaTimeUS);
}

void BoundsComponent::PostUpdate(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    updateAndGetBoundingBox();

    BaseComponentType<BoundsComponent, ComponentType::BOUNDS>::PostUpdate(deltaTimeUS);
}

};