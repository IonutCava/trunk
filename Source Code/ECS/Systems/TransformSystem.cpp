#include "stdafx.h"

#include "ECS/Events/Headers/TransformEvents.h"
#include "ECS/Components/Headers/TransformComponent.h"

#include "Headers/TransformSystem.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

    TransformSystem::TransformSystem(ECS::ECSEngine& parentEngine)
        : ECSSystem(parentEngine)
    {

    }

    TransformSystem::~TransformSystem()
    {

    }

    void TransformSystem::PreUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (;transform != transformEnd; ++transform)
        {
            transform->PreUpdate(microSec);
        }
    }

    void TransformSystem::Update(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (; transform != transformEnd; ++transform)
        {
            transform->Update(microSec);
        }
    }

    void TransformSystem::PostUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);
         
        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (;transform != transformEnd; ++transform)
        {
            transform->PostUpdate(microSec);
        }
    }

    bool TransformSystem::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->save(outputBuffer)) {
            return false;
        }
         
        return ECSSystem<TransformSystem>::save(sgn, outputBuffer);
    }

    bool TransformSystem::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->load(inputBuffer)) {
            return false;
        }
           
        return ECSSystem<TransformSystem>::save(sgn, inputBuffer);
    }
}; //namespace Divide