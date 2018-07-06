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

void BoundsComponent::parseBounds(const BoundingBox& nodeBounds, const bool force, const mat4<F32>& worldTransform) {
    if (_boundingBoxDirty || force) {
        SceneGraphNode_ptr parent = _parentSGN.getParent().lock();
        if (parent) {
            parent->get<BoundsComponent>()->flagBoundingBoxDirty();
        }

        _boundingBox.set(nodeBounds);

        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            _boundingBox.add(_parentSGN.getChild(i, childCount).get<BoundsComponent>()->getBoundingBoxConst());
        }

        if (!_lockBBTransforms) {
            _boundingBox.transform(worldTransform);
        }

        _boundingSphere.fromBoundingBox(_boundingBox);
        _boundingBoxDirty = false;
    }
}

};