#include "stdafx.h"

#include "Headers/ECSManager.h"

namespace Divide {

void ECSManager::init() {
    TransformSystem* tSys = ECS::ECS_Engine->GetSystemManager()->AddSystem<TransformSystem>();
    AnimationSystem* aSys = ECS::ECS_Engine->GetSystemManager()->AddSystem<AnimationSystem>();
    RenderingSystem* rSys = ECS::ECS_Engine->GetSystemManager()->AddSystem<RenderingSystem>();

    aSys->AddDependencies(tSys);
    rSys->AddDependencies(tSys);
    ECS::ECS_Engine->GetSystemManager()->UpdateSystemWorkOrder();
}

void ECSManager::destroy() {

}

}; //namespace Divide