#include "stdafx.h"

#include "Headers/RenderingSystem.h"

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
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        _componentCache.resize(0);
        _componentCache.reserve(_container->size());

        auto rComp = _container->begin();
        auto rCompEnd = _container->end();
        for (;rComp != rCompEnd; ++rComp)
        {
            _componentCache.push_back(rComp.ptr());
            rComp->PreUpdate(microSec);
        }
    }

    void RenderingSystem::Update(F32 dt) {
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (RenderingComponent* rComp : _componentCache) 
        {
            rComp->Update(microSec);
        }
    }

    void RenderingSystem::PostUpdate(F32 dt) {
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (RenderingComponent* rComp : _componentCache)
        {
            rComp->PostUpdate(microSec);
        }
    }

    bool RenderingSystem::saveCache(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        RenderingComponent* rComp = sgn.GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->saveCache(outputBuffer)) {
            return false;
        }

        return Super::saveCache(sgn, outputBuffer);
    }

    bool RenderingSystem::loadCache(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        RenderingComponent* rComp = sgn.GetComponent<RenderingComponent>();
        if (rComp != nullptr && !rComp->loadCache(inputBuffer)) {
            return false;
        }

        return Super::loadCache(sgn, inputBuffer);
    }
}; //namespace Divide