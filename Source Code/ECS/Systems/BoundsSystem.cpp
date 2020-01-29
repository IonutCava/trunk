#include "stdafx.h"

#include "Headers/BoundsSystem.h"
#include "Graphs/Headers/SceneNode.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
    BoundsSystem::BoundsSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : ECSSystem(parentEngine),
          PlatformContextComponent(context)
    {
        _componentCache.reserve(Config::MAX_VISIBLE_NODES);
    }

    BoundsSystem::~BoundsSystem()
    {
    }

    // Recures all the way up to the root
    void BoundsSystem::onBoundsChanged(SceneGraphNode& sgn) const {
        SceneGraphNode* parent = sgn.parent();
        if (parent != nullptr) {
            Attorney::SceneNodeBoundsComponent::setBoundsChanged(parent->getNode());
            onBoundsChanged(*parent);
        }
    }

    // Set all parent nodes' bbs to dirty if any child changed his bb. This step just sets the flags!
    void BoundsSystem::PreUpdate(F32 dt) {
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        _componentCache.resize(0);

        auto bComp = _container->begin();
        auto bCompEnd = _container->end();
        for (;bComp != bCompEnd; ++bComp)
        {
            _componentCache.push_back(bComp.ptr());

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
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (BoundsComponent* bComp : _componentCache)
        {
            const SceneNode& sceneNode = bComp->getSGN().getNode();
            if (Attorney::SceneNodeBoundsComponent::boundsChanged(sceneNode)) {
                bComp->setRefBoundingBox(sceneNode.getBounds());
            }

            bComp->Update(microSec);
        }
    }

    // Everything should be up-to-date, so we could clear all of the flags. 
    void BoundsSystem::PostUpdate(F32 dt) {
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (BoundsComponent* bComp : _componentCache)
        {
            Attorney::SceneNodeBoundsComponent::clearBoundsChanged(bComp->getSGN().getNode());
            bComp->PostUpdate(microSec);
        }
    }

}; //namespace Divide