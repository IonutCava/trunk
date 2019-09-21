#include "stdafx.h"

#include "Headers/AnimationSystem.h"
#include "ECS/Components/Headers/AnimationComponent.h"

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

        auto container = _compManager->GetComponentContainer<AnimationComponent>();
        auto anim = container->begin();
        auto animEnd = container->end();
        for (;anim != animEnd; ++anim)
        {
            anim->PreUpdate(microSec);
        }
    }

    void AnimationSystem::Update(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto container = _compManager->GetComponentContainer<AnimationComponent>();
        auto anim = container->begin();
        auto animEnd = container->end();
        for (; anim != animEnd; ++anim)
        {
            anim->Update(microSec);
        }
    }

    void AnimationSystem::PostUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto container = _compManager->GetComponentContainer<AnimationComponent>();
        auto anim = container->begin();
        auto animEnd = container->end();
        for (; anim != animEnd; ++anim)
        {
            anim->PostUpdate(microSec);
        }
    }

    void AnimationSystem::FrameEnded() {
        auto container = _compManager->GetComponentContainer<AnimationComponent>();
        auto comp = container->begin();
        auto compEnd = container->end();
        for (; comp != compEnd; ++comp) {
            comp->FrameEnded();
        }
    }

    bool AnimationSystem::saveCache(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        AnimationComponent* aComp = sgn.GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->saveCache(outputBuffer)) {
            return false;
        }

        return ECSSystem<AnimationSystem>::saveCache(sgn, outputBuffer);
    }

    bool AnimationSystem::loadCache(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        AnimationComponent* aComp = sgn.GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->loadCache(inputBuffer)) {
            return false;
        }

        return ECSSystem<AnimationSystem>::loadCache(sgn, inputBuffer);
    }
};//namespace Divide