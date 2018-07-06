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
        for (auto transform = ECS::ECS_Engine->GetComponentManager()->begin<TransformComponent>(); 
                  transform != ECS::ECS_Engine->GetComponentManager()->end<TransformComponent>();
                ++transform)
        {
           
        }
    }

    void TransformSystem::Update(F32 dt) {
        // 
    }

    void TransformSystem::PostUpdate(F32 dt) {
        // If the transform has been modified, notify listeners, 
        for (auto transform = ECS::ECS_Engine->GetComponentManager()->begin<TransformComponent>();
                  transform != ECS::ECS_Engine->GetComponentManager()->end<TransformComponent>();
                ++transform)
        {
            transform->notifyListeners();
            transform->snapshot();
        }
    }
}; //namespace Divide