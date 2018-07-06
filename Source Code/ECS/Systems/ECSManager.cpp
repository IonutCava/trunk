#include "stdafx.h"

#include "Headers/ECSManager.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

ECSManager::ECSManager(PlatformContext& context, ECS::ECSEngine& engine)
    : PlatformContextComponent(context),
      _ecsEngine(engine)
{
    TransformSystem* tSys = _ecsEngine.GetSystemManager()->AddSystem<TransformSystem>(_ecsEngine, _context);
    AnimationSystem* aSys = _ecsEngine.GetSystemManager()->AddSystem<AnimationSystem>(_ecsEngine, _context);
    RenderingSystem* rSys = _ecsEngine.GetSystemManager()->AddSystem<RenderingSystem>(_ecsEngine, _context);
    BoundsSystem*  bSys = _ecsEngine.GetSystemManager()->AddSystem<BoundsSystem>(_ecsEngine, _context);

    aSys->AddDependencies(tSys);
    bSys->AddDependencies(aSys);
    rSys->AddDependencies(tSys);
    rSys->AddDependencies(bSys);
    _ecsEngine.GetSystemManager()->UpdateSystemWorkOrder();
}

ECSManager::~ECSManager()
{

}

bool ECSManager::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) const {
    TransformSystem* tSys = _ecsEngine.GetSystemManager()->GetSystem<TransformSystem>();
    if (tSys != nullptr) {
        if (!tSys->save(sgn, outputBuffer)) {
            return false;
        }
    }
    AnimationSystem* aSys = _ecsEngine.GetSystemManager()->GetSystem<AnimationSystem>();
    if (aSys != nullptr) {
        if (!aSys->save(sgn, outputBuffer)) {
            return false;
        }
    }
    RenderingSystem* rSys = _ecsEngine.GetSystemManager()->GetSystem<RenderingSystem>();
    if (rSys != nullptr) {
        if (!rSys->save(sgn, outputBuffer)) {
            return false;
        }
    }
    return true;
}

bool ECSManager::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
    TransformSystem* tSys = _ecsEngine.GetSystemManager()->GetSystem<TransformSystem>();
    if (tSys != nullptr) {
        if (!tSys->load(sgn, inputBuffer)) {
            return false;
        }
    }
    AnimationSystem* aSys = _ecsEngine.GetSystemManager()->GetSystem<AnimationSystem>();
    if (aSys != nullptr) {
        if (!aSys->load(sgn, inputBuffer)) {
            return false;
        }
    }
    RenderingSystem* rSys = _ecsEngine.GetSystemManager()->GetSystem<RenderingSystem>();
    if (rSys != nullptr) {
        if (!rSys->load(sgn, inputBuffer)) {
            return false;
        }
    }
    return true;
}

}; //namespace Divide