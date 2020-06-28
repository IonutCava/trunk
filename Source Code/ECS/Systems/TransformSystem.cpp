#include "stdafx.h"

#include "Headers/TransformSystem.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

    TransformSystem::TransformSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : PlatformContextComponent(context),
          ECSSystem(parentEngine)
    {
    }

    bool TransformSystem::saveCache(const SceneGraphNode* sgn, ByteBuffer& outputBuffer) {
        TransformComponent* tComp = sgn->GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->saveCache(outputBuffer)) {
            return false;
        }

        return Super::saveCache(sgn, outputBuffer);
    }

    bool TransformSystem::loadCache(SceneGraphNode* sgn, ByteBuffer& inputBuffer) {
        TransformComponent* tComp = sgn->GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->loadCache(inputBuffer)) {
            return false;
        }

        return Super::loadCache(sgn, inputBuffer);
    }
}; //namespace Divide