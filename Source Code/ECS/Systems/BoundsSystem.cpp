#include "stdafx.h"

#include "Headers/BoundsSystem.h"
#include "Graphs/Headers/SceneNode.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
    BoundsSystem::BoundsSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : ECSSystem(parentEngine),
          PlatformContextComponent(context)
    {
    }

    BoundsSystem::~BoundsSystem()
    {
    }

    // Recures all the way up to the root
    void BoundsSystem::onBoundsChanged(SceneGraphNode& sgn) const {
        SceneGraphNode* parent = sgn.getParent();
        if (parent != nullptr) {
            Attorney::SceneNodeBoundsComponent::setBoundsChanged(parent->getNode());
            onBoundsChanged(*parent);
        }
    }

    // Set all parent nodes' bbs to dirty if any child changed his bb. This step just sets the flags!
    void BoundsSystem::PreUpdate(F32 dt) {
        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto bComp = _container->begin();
        auto bCompEnd = _container->end();
        for (;bComp != bCompEnd; ++bComp)
        {
            SceneGraphNode& sgn = bComp->getSGN();
            if (Attorney::SceneNodeBoundsComponent::boundsChanged(sgn.getNode())) {
                bComp->flagBoundingBoxDirty(false);
                onBoundsChanged(sgn);
            }

            bComp->PreUpdate(microSec);
        }
    }

    // Grab all of the update bounding boxes where needed. This step does not clear the flags!
    void BoundsSystem::Update(F32 dt) {
        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto bComp = _container->begin();
        auto bCompEnd = _container->end();
        for (;bComp != bCompEnd; ++bComp) {
            const SceneNode& sceneNode = bComp->getSGN().getNode();
            if (Attorney::SceneNodeBoundsComponent::boundsChanged(sceneNode)) {
                bComp->setRefBoundingBox(sceneNode.getBounds());
            }

            bComp->Update(microSec);
        }
    }

    // Everything should be up-to-date, so we could clear all of the flags. 
    void BoundsSystem::PostUpdate(F32 dt) {
        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto bComp = _container->begin();
        auto bCompEnd = _container->end();
        for (; bComp != bCompEnd; ++bComp) {
            Attorney::SceneNodeBoundsComponent::clearBoundsChanged(bComp->getSGN().getNode());
            bComp->PostUpdate(microSec);
        }
    }

    void BoundsSystem::FrameEnded() {

        auto comp = _container->begin();
        auto compEnd = _container->end();
        for (; comp != compEnd; ++comp) {
            comp->FrameEnded();
        }
    }
}; //namespace Divide