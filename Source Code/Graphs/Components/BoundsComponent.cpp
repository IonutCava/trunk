#include "Headers/BoundsComponent.h"
#include "Headers/PhysicsComponent.h"

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

void BoundsComponent::update(const U64 deltaTime){
    //if (_boundingBoxDirty) {
        SceneGraphNode_ptr parent = _parentSGN.getParent().lock();
        if (parent) {
            parent->get<BoundsComponent>()->flagBoundingBoxDirty();
        }

        _boundingBox.set(/*_refBoundingBox*/_parentSGN.getNode()->getBoundingBox().first);

        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            _boundingBox.add(_parentSGN.getChild(i, childCount).get<BoundsComponent>()->getBoundingBox());
        }

        if (!_lockBBTransforms) {
            _boundingBox.transform(_worldMatrix);
        }

        _boundingSphere.fromBoundingBox(_boundingBox);
        _boundingBoxDirty = false;
    //}
}

};