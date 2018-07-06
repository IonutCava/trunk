#include "stdafx.h"

#include "Headers/AnimationSystem.h"
#include "ECS/Components/Headers/AnimationComponent.h"

namespace Divide {
    AnimationSystem::AnimationSystem(ECS::ECSEngine& parentEngine)
        : ECSSystem(parentEngine)
    {

    }

    AnimationSystem::~AnimationSystem()
    {

    }

    void AnimationSystem::PreUpdate(F32 dt) {
        auto anim = _engine.GetComponentManager()->begin<AnimationComponent>();
        auto animEnd = _engine.GetComponentManager()->end<AnimationComponent>();
        for (;anim != animEnd; ++anim)
        {
            anim->update(Time::MillisecondsToMicroseconds(dt));
        }
    }

    void AnimationSystem::Update(F32 dt) {

    }

    void AnimationSystem::PostUpdate(F32 dt) {

    }

    bool AnimationSystem::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        AnimationComponent* aComp = sgn.GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->save(outputBuffer)) {
            return false;
        }

        return ECSSystem<AnimationSystem>::save(sgn, outputBuffer);
    }

    bool AnimationSystem::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        AnimationComponent* aComp = sgn.GetComponent<AnimationComponent>();
        if (aComp != nullptr && !aComp->load(inputBuffer)) {
            return false;
        }

        return ECSSystem<AnimationSystem>::load(sgn, inputBuffer);
    }
};//namespace Divide