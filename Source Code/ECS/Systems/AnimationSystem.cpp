#include "stdafx.h"

#include "Headers/AnimationSystem.h"

namespace Divide {
    AnimationSystem::AnimationSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : PlatformContextComponent(context),
          ECSSystem(parentEngine)
    {
    }

    bool AnimationSystem::saveCache(const SceneGraphNode* sgn, ByteBuffer& outputBuffer) {
        AnimationComponent* aComp = sgn->GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->saveCache(outputBuffer)) {
            return false;
        }

        return Super::saveCache(sgn, outputBuffer);
    }

    bool AnimationSystem::loadCache(SceneGraphNode* sgn, ByteBuffer& inputBuffer) {
        AnimationComponent* aComp = sgn->GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->loadCache(inputBuffer)) {
            return false;
        }

        return Super::loadCache(sgn, inputBuffer);
    }
};//namespace Divide