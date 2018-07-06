#include "stdafx.h"

#include "Headers/RenderingSystem.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {
    RenderingSystem::RenderingSystem()
    {

    }

    RenderingSystem::~RenderingSystem()
    {

    }

    void RenderingSystem::PreUpdate(F32 dt) {
        auto rComp = ECS::ECS_Engine->GetComponentManager()->begin<RenderingComponent>();
        auto rCompEnd = ECS::ECS_Engine->GetComponentManager()->end<RenderingComponent>();
        for (;rComp != rCompEnd; ++rComp)
        {
            rComp->update(Time::MillisecondsToMicroseconds(dt));
        }
    }

    void RenderingSystem::Update(F32 dt) {

    }

    void RenderingSystem::PostUpdate(F32 dt) {

    }
}; //namespace Divide