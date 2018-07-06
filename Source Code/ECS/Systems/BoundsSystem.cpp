#include "stdafx.h"

#include "Headers/BoundsSystem.h"
#include "Graphs/Headers/SceneNode.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Components/Headers/BoundsComponent.h"

namespace Divide {
    BoundsSystem::BoundsSystem()
    {

    }


    BoundsSystem::~BoundsSystem()
    {

    }

    void BoundsSystem::PreUpdate(F32 dt) {
        auto bComp = ECS::ECS_Engine->GetComponentManager()->begin<BoundsComponent>();
        auto bCompEnd = ECS::ECS_Engine->GetComponentManager()->end<BoundsComponent>();
        for (;bComp != bCompEnd; ++bComp)
        {
            SceneGraphNode* parent = 
                static_cast<SceneGraphNode*>(ECS::ECS_Engine->GetEntityManager()->GetEntity(bComp->GetOwner()));

            if (parent != nullptr) {
                const SceneNode_ptr& node = parent->getNode();

                vectorImpl<SceneNode::SGNParentData>::iterator it;
                it = node->getSGNData(parent->getGUID());
                assert(it != std::cend(node->_sgnParents));
                SceneNode::SGNParentData& parentData = *it;

                if (parentData.getFlag(SceneNode::UpdateFlag::BOUNDS_CHANGED)) {
                    node->updateBoundsInternal(*parent);
                    bComp->onBoundsChange(node->refBoundingBox());
                    parentData.clearFlag(SceneNode::UpdateFlag::BOUNDS_CHANGED);
                }
            }
        }
    }

    void BoundsSystem::Update(F32 dt) {

    }

    void BoundsSystem::PostUpdate(F32 dt) {

    }
}; //namespace Divide