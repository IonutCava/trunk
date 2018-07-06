#include "stdafx.h"

#include "Headers/RenderingSystem.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {
    RenderingSystem::RenderingSystem(ECS::ECSEngine& parentEngine)
        : ECSSystem(parentEngine)
    {

    }

    RenderingSystem::~RenderingSystem()
    {

    }

    void RenderingSystem::PreUpdate(F32 dt) {
        auto rComp = _engine.GetComponentManager()->begin<RenderingComponent>();
        auto rCompEnd = _engine.GetComponentManager()->end<RenderingComponent>();
        for (;rComp != rCompEnd; ++rComp)
        {
            rComp->update(Time::MillisecondsToMicroseconds(dt));
        }
    }

    void RenderingSystem::Update(F32 dt) {

    }

    void RenderingSystem::PostUpdate(F32 dt) {

    }

    bool RenderingSystem::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        return ECSSystem<RenderingSystem>::save(sgn, outputBuffer);
    }

    bool RenderingSystem::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        return ECSSystem<RenderingSystem>::load(sgn, inputBuffer);
    }
}; //namespace Divide