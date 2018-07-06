#include "stdafx.h"

#include "ECS/Events/Headers/TransformEvents.h"
#include "ECS/Components/Headers/TransformComponent.h"

#include "Headers/TransformSystem.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

    TransformSystem::TransformSystem()
    {

    }

    TransformSystem::~TransformSystem()
    {

    }

    void TransformSystem::PreUpdate(F32 dt) {
        // Go over all transforms and make sure we update parent states
        auto transform = ECS::ECS_Engine->GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = ECS::ECS_Engine->GetComponentManager()->end<TransformComponent>();
        for (;transform != transformEnd; ++transform)
        {
           
        }
    }

    void TransformSystem::Update(F32 dt) {
        // 
    }

    void TransformSystem::PostUpdate(F32 dt) {
        // If the transform has been modified, notify listeners, 
        auto transform = ECS::ECS_Engine->GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = ECS::ECS_Engine->GetComponentManager()->end<TransformComponent>();
        for (;transform != transformEnd; ++transform)
        {
            transform->snapshot();
        }
    }
}; //namespace Divide