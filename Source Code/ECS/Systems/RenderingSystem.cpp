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

        auto compManager = _engine.GetComponentManager();
        auto rComp = compManager->begin<RenderingComponent>();
        auto rCompEnd = compManager->end<RenderingComponent>();
        for (;rComp != rCompEnd; ++rComp)
        {
            rComp->PreUpdate(microSec);
        }
    }

    void RenderingSystem::Update(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto compManager = _engine.GetComponentManager();
        auto rComp = compManager->begin<RenderingComponent>();
        auto rCompEnd = compManager->end<RenderingComponent>();
        for (; rComp != rCompEnd; ++rComp)
        {
            rComp->Update(microSec);
        }
    }

    void RenderingSystem::PostUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto compManager = _engine.GetComponentManager();
        auto rComp = compManager->begin<RenderingComponent>();
        auto rCompEnd = compManager->end<RenderingComponent>();
        for (; rComp != rCompEnd; ++rComp)
        {
            rComp->PostUpdate(microSec);
        }
    }

    bool RenderingSystem::saveCache(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        RenderingComponent* rComp = sgn.GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->saveCache(outputBuffer)) {
            return false;
        }

        return ECSSystem<RenderingSystem>::saveCache(sgn, outputBuffer);
    }

    bool RenderingSystem::loadCache(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        RenderingComponent* rComp = sgn.GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->loadCache(inputBuffer)) {
            return false;
        }

        return ECSSystem<RenderingSystem>::loadCache(sgn, inputBuffer);
    }
}; //namespace Divide