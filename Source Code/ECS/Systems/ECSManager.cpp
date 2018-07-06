#include "stdafx.h"

#include "Headers/ECSManager.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

ECS::ECSEngine* ECSManager::s_ecsEngine = nullptr;

void ECSManager::init(ECS::ECSEngine& engine) 
{
    if (s_ecsEngine != nullptr) {
        destroy(*s_ecsEngine);
    }
    s_ecsEngine = &engine;

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
    s_ecsEngine = nullptr;
}

bool ECSManager::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
    if (s_ecsEngine != nullptr) {
        TransformSystem* tSys = s_ecsEngine->GetSystemManager()->GetSystem<TransformSystem>();
        if (tSys != nullptr) {
            if (!tSys->save(sgn, outputBuffer)) {
                return false;
            }
        }
        AnimationSystem* aSys = s_ecsEngine->GetSystemManager()->GetSystem<AnimationSystem>();
        if (aSys != nullptr) {
            if (!aSys->save(sgn, outputBuffer)) {
                return false;
            }
        }
        RenderingSystem* rSys = s_ecsEngine->GetSystemManager()->GetSystem<RenderingSystem>();
        if (rSys != nullptr) {
            if (!rSys->save(sgn, outputBuffer)) {
                return false;
            }
        }
        return true;
    }

    return false;
}

bool ECSManager::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
    if (s_ecsEngine != nullptr) {
        TransformSystem* tSys = s_ecsEngine->GetSystemManager()->GetSystem<TransformSystem>();
        if (tSys != nullptr) {
            if (!tSys->load(sgn, inputBuffer)) {
                return false;
            }
        }
        AnimationSystem* aSys = s_ecsEngine->GetSystemManager()->GetSystem<AnimationSystem>();
        if (aSys != nullptr) {
            if (!aSys->load(sgn, inputBuffer)) {
                return false;
            }
        }
        RenderingSystem* rSys = s_ecsEngine->GetSystemManager()->GetSystem<RenderingSystem>();
        if (rSys != nullptr) {
            if (!rSys->load(sgn, inputBuffer)) {
                return false;
            }
        }
        return true;
    }

    return false;
}

}; //namespace Divide