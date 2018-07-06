#include "stdafx.h"

#include "Headers/RenderingSystem.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {
    RenderingSystem::RenderingSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : ECSSystem(parentEngine),
          PlatformContextComponent(context)
    {

    }

    RenderingSystem::~RenderingSystem()
    {

    }

    void RenderingSystem::PreUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto rComp = _engine.GetComponentManager()->begin<RenderingComponent>();
        auto rCompEnd = _engine.GetComponentManager()->end<RenderingComponent>();
        for (;rComp != rCompEnd; ++rComp)
        {
            rComp->PreUpdate(microSec);
        }
    }

    void RenderingSystem::Update(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto rComp = _engine.GetComponentManager()->begin<RenderingComponent>();
        auto rCompEnd = _engine.GetComponentManager()->end<RenderingComponent>();
        for (; rComp != rCompEnd; ++rComp)
        {
            rComp->Update(microSec);
        }
    }

    void RenderingSystem::PostUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto rComp = _engine.GetComponentManager()->begin<RenderingComponent>();
        auto rCompEnd = _engine.GetComponentManager()->end<RenderingComponent>();
        for (; rComp != rCompEnd; ++rComp)
        {
            rComp->PostUpdate(microSec);
        }
    }

    bool RenderingSystem::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        RenderingComponent* rComp = sgn.GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->save(outputBuffer)) {
            return false;
        }

        return ECSSystem<RenderingSystem>::save(sgn, outputBuffer);
    }

    bool RenderingSystem::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        RenderingComponent* rComp = sgn.GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->load(inputBuffer)) {
            return false;
        }

        return ECSSystem<RenderingSystem>::load(sgn, inputBuffer);
    }
}; //namespace Divide