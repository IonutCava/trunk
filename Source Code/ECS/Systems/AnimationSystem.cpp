#include "stdafx.h"

#include "Headers/AnimationSystem.h"
#include "ECS/Components/Headers/AnimationComponent.h"

namespace Divide {
    AnimationSystem::AnimationSystem()
    {

    }

    AnimationSystem::~AnimationSystem()
    {

    }

    void AnimationSystem::PreUpdate(F32 dt) {
        auto anim = ECS::ECS_Engine->GetComponentManager()->begin<AnimationComponent>();
        auto animEnd = ECS::ECS_Engine->GetComponentManager()->end<AnimationComponent>();
        for (;anim != animEnd; ++anim)
        {
            anim->update(Time::MillisecondsToMicroseconds(dt));
        }
    }

    void AnimationSystem::Update(F32 dt) {

    }

    void AnimationSystem::PostUpdate(F32 dt) {

    }
};//namespace Divide