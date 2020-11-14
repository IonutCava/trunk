#include "stdafx.h"

#include "Headers/RenderingSystem.h"

namespace Divide {
    RenderingSystem::RenderingSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : PlatformContextComponent(context),
          ECSSystem(parentEngine)
    {
    }

    bool RenderingSystem::saveCache(const SceneGraphNode* sgn, ByteBuffer& outputBuffer) {
        RenderingComponent* rComp = sgn->GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->saveCache(outputBuffer)) {
            return false;
        }

        return Super::saveCache(sgn, outputBuffer);
    }

    bool RenderingSystem::loadCache(SceneGraphNode* sgn, ByteBuffer& inputBuffer) {
        RenderingComponent* rComp = sgn->GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->loadCache(inputBuffer)) {
            return false;
        }

        return Super::loadCache(sgn, inputBuffer);
    }
} //namespace Divide