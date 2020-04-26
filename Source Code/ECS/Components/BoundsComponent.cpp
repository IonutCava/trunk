#include "stdafx.h"

#include "Headers/BoundsComponent.h"
#include "Headers/RenderingComponent.h"
#include "Headers/TransformComponent.h"

#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

BoundsComponent::BoundsComponent(SceneGraphNode& sgn, PlatformContext& context)
    : BaseComponentType<BoundsComponent, ComponentType::BOUNDS>(sgn, context),
     _tCompCache(sgn.get<TransformComponent>())
{
    _refBoundingBox.set(sgn.getNode().getBounds());
    _boundingBox.set(_refBoundingBox);
    _boundingSphere.fromBoundingBox(_boundingBox);

    _boundingBoxDirty.store(true);

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

    EditorComponentField vbbField = {};
    vbbField._name = "Show AABB";
    vbbField._data = &_showAABB;
    vbbField._type = EditorComponentFieldType::PUSH_TYPE;
    vbbField._basicType = GFX::PushConstantType::BOOL;
    vbbField._readOnly = false;

    _editorComponent.registerField(std::move(vbbField));

    EditorComponentField vbsField = {};
    vbsField._name = "Show Bounding Sphere";
    vbsField._data = &_showBS;
    vbsField._type = EditorComponentFieldType::PUSH_TYPE;
    vbsField._basicType = GFX::PushConstantType::BOOL;
    vbsField._readOnly = false;
    _editorComponent.registerField(std::move(vbsField));

    EditorComponentField recomputeBoundsField = {};
    recomputeBoundsField._name = "Recompute Bounds";
    recomputeBoundsField._range = { recomputeBoundsField._name.length() * 10.0f, 20.0f };//dimensions
    recomputeBoundsField._type = EditorComponentFieldType::BUTTON;
    recomputeBoundsField._readOnly = false; //disabled/enabled
    _editorComponent.registerField(std::move(recomputeBoundsField));

    _editorComponent.onChangedCbk([this](const char* field) {
        if (strcmp(field, "Show AABB") == 0 ||
            strcmp(field, "Show Bounding Sphere") == 0) 
        {
            RenderingComponent* const rComp = _parentSGN.get<RenderingComponent>();
            if (rComp != nullptr) {
                rComp->toggleRenderOption(RenderingComponent::RenderOptions::RENDER_BOUNDS_AABB, _showAABB);
                rComp->toggleRenderOption(RenderingComponent::RenderOptions::RENDER_BOUNDS_SPHERE, _showBS);
            }
        } else if (strcmp(field, "Recompute Bounds") == 0) {
            flagBoundingBoxDirty(true);
        }
    });
}

BoundsComponent::~BoundsComponent()
{
}

void BoundsComponent::flagBoundingBoxDirty(bool recursive) {
    OPTICK_EVENT();

    if (_boundingBoxDirty.exchange(true)) {
        // already dirty
        return;
    }

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

void BoundsComponent::OnData(const ECS::CustomEvent& data) {
    if (data._type == ECS::CustomEvent::Type::TransformUpdated) {
        _tCompCache = std::any_cast<TransformComponent*>(data._userData);

        flagBoundingBoxDirty(true);
    }
}

void BoundsComponent::setRefBoundingBox(const BoundingBox& nodeBounds) noexcept {
    // All the parents should already be dirty thanks to the bounds system
    _refBoundingBox.set(nodeBounds);
    _boundingBoxDirty.store(true);
}

const BoundingBox& BoundsComponent::updateAndGetBoundingBox() {
    OPTICK_EVENT();
    if (!_boundingBoxDirty.exchange(false)) {
        // already clean
        return _boundingBox;
    }

    _parentSGN.forEachChild([](const SceneGraphNode* child, I32 /*childIdx*/) {
        child->get<BoundsComponent>()->updateAndGetBoundingBox();
        return true;
    });

    _boundingBox.transform(_refBoundingBox.getMin(), _refBoundingBox.getMax(), _tCompCache->getWorldMatrix());
    _boundingSphere.fromBoundingBox(_boundingBox);

    _parentSGN.SendEvent(
    {
        ECS::CustomEvent::Type::BoundsUpdated,
        this,
        0u
    });
    

    return _boundingBox;
}

F32 BoundsComponent::distanceToBSpehereSQ(const vec3<F32>& pos) const noexcept {
    const BoundingSphere& sphere = getBoundingSphere();
    return sphere.getCenter().distanceSquared(pos) - SQUARED(sphere.getRadius());
}

void BoundsComponent::Update(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    BaseComponentType<BoundsComponent, ComponentType::BOUNDS>::Update(deltaTimeUS);
}

void BoundsComponent::PostUpdate(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    assert(_tCompCache != nullptr);
    updateAndGetBoundingBox();

    BaseComponentType<BoundsComponent, ComponentType::BOUNDS>::PostUpdate(deltaTimeUS);
}

};