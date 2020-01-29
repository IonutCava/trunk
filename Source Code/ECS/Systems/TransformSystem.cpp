#include "stdafx.h"

#include "ECS/Events/Headers/TransformEvents.h"

#include "Headers/TransformSystem.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Headers/EngineTaskPool.h"

namespace Divide {

    TransformSystem::TransformSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : ECSSystem(parentEngine),
          PlatformContextComponent(context)
    {
        // Just a random value to start with some mem in place
        _componentCache.reserve(Config::MAX_VISIBLE_NODES);
    }

    TransformSystem::~TransformSystem()
    {

    }

    void TransformSystem::PreUpdate(F32 dt) {
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        // Keep memory in order to avoid mid-frame allocs
        _componentCache.resize(0);

        auto transform = _container->begin();
        auto transformEnd = _container->end();
        for (;transform != transformEnd; ++transform)
        {
            _componentCache.push_back(transform.ptr());
            transform->PreUpdate(microSec);
        }
    }

    void TransformSystem::Update(F32 dt) {
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (TransformComponent* tComp : _componentCache) {
            tComp->Update(microSec);
        }
    }

    void TransformSystem::PostUpdate(F32 dt) {
        OPTICK_EVENT();

        const U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (TransformComponent* tComp : _componentCache) {
            tComp->PostUpdate(microSec);
        }
    }

    void TransformSystem::OnUpdateLoop() {
        
        auto transform = _container->begin();
        auto transformEnd = _container->end();
        for (; transform != transformEnd; ++transform)
        {
            transform->OnUpdateLoop();
        }
    }

    bool TransformSystem::saveCache(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->saveCache(outputBuffer)) {
            return false;
        }

        return Super::saveCache(sgn, outputBuffer);
    }

    bool TransformSystem::loadCache(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->loadCache(inputBuffer)) {
            return false;
        }

        return Super::loadCache(sgn, inputBuffer);
    }
}; //namespace Divide