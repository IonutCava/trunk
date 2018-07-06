#include "stdafx.h"

#include "Headers/UpdateSystem.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

    UpdateSystem::UpdateSystem(ECS::ECSEngine& parentEngine)
        : ECSSystem(parentEngine)
    {

    }

    UpdateSystem::~UpdateSystem()
    {

    }

    void UpdateSystem::PreUpdate(F32 dt) {
        ACKNOWLEDGE_UNUSED(dt);
    }

    void UpdateSystem::Update(F32 dt) {
        ACKNOWLEDGE_UNUSED(dt);
    }

    void UpdateSystem::PostUpdate(F32 dt) {
        ACKNOWLEDGE_UNUSED(dt);
    }

}; //namespace Divide