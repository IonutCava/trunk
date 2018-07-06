#include "stdafx.h"

#include "Headers/BoundsSystem.h"
#include "Graphs/Headers/SceneNode.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Components/Headers/BoundsComponent.h"

namespace Divide {
    BoundsSystem::BoundsSystem(ECS::ECSEngine& parentEngine)
        : ECSSystem(parentEngine)
    {

    }


    BoundsSystem::~BoundsSystem()
    {

    }

    void BoundsSystem::PreUpdate(F32 dt) {
        ECS::EntityManager* entityManager = _engine.GetEntityManager();

        auto bComp = _engine.GetComponentManager()->begin<BoundsComponent>();
        auto bCompEnd = _engine.GetComponentManager()->end<BoundsComponent>();
        for (;bComp != bCompEnd; ++bComp)
        {
            const ECS::EntityId owner = bComp->GetOwner();
            ECS::IEntity* entity = entityManager->GetEntity(owner);

            SceneGraphNode* parent = static_cast<SceneGraphNode*>(entity);

            if (parent != nullptr) {
                const SceneNode_ptr& node = parent->getNode();

                vectorImpl<SceneNode::SGNParentData>::iterator it;
                it = node->getSGNData(parent->getGUID());
                if (it != std::cend(node->_sgnParents)) {
                    SceneNode::SGNParentData& parentData = *it;

                    if (parentData.getFlag(SceneNode::UpdateFlag::BOUNDS_CHANGED)) {
                        node->updateBoundsInternal(*parent);
                        bComp->onBoundsChange(node->refBoundingBox());
                        parentData.clearFlag(SceneNode::UpdateFlag::BOUNDS_CHANGED);
                    }
                }
            }
        }
    }

    void BoundsSystem::Update(F32 dt) {
        auto bComp = _engine.GetComponentManager()->begin<BoundsComponent>();
        auto bCompEnd = _engine.GetComponentManager()->end<BoundsComponent>();
        for (;bComp != bCompEnd; ++bComp)
        {
            bComp->update(Time::MillisecondsToMicroseconds(dt));
        }
    }

    void BoundsSystem::PostUpdate(F32 dt) {

    }
}; //namespace Divide