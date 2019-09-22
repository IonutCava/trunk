#include "stdafx.h"

#include "Headers/AnimationSystem.h"

namespace Divide {
    AnimationSystem::AnimationSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : ECSSystem(parentEngine),
          PlatformContextComponent(context)
    {

    }

    AnimationSystem::~AnimationSystem()
    {

    }

    void AnimationSystem::PreUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto anim = _container->begin();
        auto animEnd = _container->end();
        for (;anim != animEnd; ++anim)
        {
            anim->PreUpdate(microSec);
        }
    }

    void AnimationSystem::Update(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto anim = _container->begin();
        auto animEnd = _container->end();
        for (; anim != animEnd; ++anim)
        {
            anim->Update(microSec);
        }
    }

    void AnimationSystem::PostUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto anim = _container->begin();
        auto animEnd = _container->end();
        for (; anim != animEnd; ++anim)
        {
            anim->PostUpdate(microSec);
        }
    }

    void AnimationSystem::FrameEnded() {

        auto comp = _container->begin();
        auto compEnd = _container->end();
        for (; comp != compEnd; ++comp) {
            comp->FrameEnded();
        }
    }

    bool AnimationSystem::saveCache(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        AnimationComponent* aComp = sgn.GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->saveCache(outputBuffer)) {
            return false;
        }

        return Super::saveCache(sgn, outputBuffer);
    }

    bool AnimationSystem::loadCache(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        AnimationComponent* aComp = sgn.GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->loadCache(inputBuffer)) {
            return false;
        }

        return Super::loadCache(sgn, inputBuffer);
    }
};//namespace Divide