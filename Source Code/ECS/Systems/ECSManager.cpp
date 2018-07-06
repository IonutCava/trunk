#include "stdafx.h"

#include "Headers/ECSManager.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

void ECSManager::init(ECS::ECSEngine& engine) 
{
    TransformSystem* tSys = engine.GetSystemManager()->AddSystem<TransformSystem>(engine);
    AnimationSystem* aSys = engine.GetSystemManager()->AddSystem<AnimationSystem>(engine);
    RenderingSystem* rSys = engine.GetSystemManager()->AddSystem<RenderingSystem>(engine);
    BoundsSystem*  bSys = engine.GetSystemManager()->AddSystem<BoundsSystem>(engine);

    aSys->AddDependencies(tSys);
    rSys->AddDependencies(tSys);
    rSys->AddDependencies(bSys);
    engine.GetSystemManager()->UpdateSystemWorkOrder();
}

void ECSManager::destroy(ECS::ECSEngine& engine) {
    ACKNOWLEDGE_UNUSED(engine);
}

}; //namespace Divide