#include "stdafx.h"

#include "Headers/BoundsSystem.h"
#include "Graphs/Headers/SceneNode.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Components/Headers/BoundsComponent.h"

namespace Divide {
    BoundsSystem::BoundsSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : ECSSystem(parentEngine),
          PlatformContextComponent(context)
    {

    }


    BoundsSystem::~BoundsSystem()
    {
    }

    void BoundsSystem::onBoundsChanged(BoundsComponent* bComp) const {
        ECS::EntityManager* entityManager = _engine.GetEntityManager();
        const ECS::EntityId owner = bComp->GetOwner();
        ECS::IEntity* entity = entityManager->GetEntity(owner);
        SceneGraphNode* parent = static_cast<SceneGraphNode*>(entity);

        if (parent != nullptr) {
            const SceneNode_ptr& node = parent->getNode();
            node->updateBoundsInternal();
            bComp->onBoundsChange(node->getBoundsInternal());
        }
    }

    void BoundsSystem::PreUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto bComp = _engine.GetComponentManager()->begin<BoundsComponent>();
        auto bCompEnd = _engine.GetComponentManager()->end<BoundsComponent>();
        for (;bComp != bCompEnd; ++bComp)
        {
            if (bComp->isBoundsChanged()) {
                onBoundsChanged(bComp.operator->());
            }

            bComp->PreUpdate(microSec);
        }
    }

    void BoundsSystem::Update(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto bComp = _engine.GetComponentManager()->begin<BoundsComponent>();
        auto bCompEnd = _engine.GetComponentManager()->end<BoundsComponent>();
        for (;bComp != bCompEnd; ++bComp)
        {
            bComp->Update(microSec);
        }
    }

    void BoundsSystem::PostUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto bComp = _engine.GetComponentManager()->begin<BoundsComponent>();
        auto bCompEnd = _engine.GetComponentManager()->end<BoundsComponent>();
        for (; bComp != bCompEnd; ++bComp)
        {
            bComp->PostUpdate(microSec);
        }
    }
}; //namespace Divide