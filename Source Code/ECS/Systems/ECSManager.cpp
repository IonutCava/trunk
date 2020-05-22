#include "stdafx.h"

#include "Headers/ECSManager.h"
#include "Platform/Headers/PlatformDefines.h"

#include "ECS/Systems/Headers/TransformSystem.h"
#include "ECS/Systems/Headers/AnimationSystem.h"
#include "ECS/Systems/Headers/RenderingSystem.h"
#include "ECS/Systems/Headers/BoundsSystem.h"

#include "ECS/Components/Headers/IKComponent.h"
#include "ECS/Components/Headers/NavigationComponent.h"
#include "ECS/Components/Headers/NetworkingComponent.h"
#include "ECS/Components/Headers/RagdollComponent.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/ScriptComponent.h"
#include "ECS/Components/Headers/SelectionComponent.h"
#include "ECS/Components/Headers/UnitComponent.h"
#include "ECS/Components/Headers/PointLightComponent.h"
#include "ECS/Components/Headers/SpotLightComponent.h"
#include "ECS/Components/Headers/DirectionalLightComponent.h"
#include "ECS/Components/Headers/EnvironmentProbeComponent.h"

namespace Divide {

namespace {
    struct PointLightSystem : public ECSSystem<PointLightSystem, PointLightComponent> {
        PointLightSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct SpotLightSystem : public ECSSystem<SpotLightSystem, SpotLightComponent>
    {
        SpotLightSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct DirectionalLightSystem : public ECSSystem<DirectionalLightSystem, DirectionalLightComponent>
    {
        DirectionalLightSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct IKSystem : public ECSSystem<IKSystem, IKComponent>
    {
        IKSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct NavigationSystem : public ECSSystem<NavigationSystem, NavigationComponent>
    {
        NavigationSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct NetworkingSystem : public ECSSystem<NetworkingSystem, NetworkingComponent>
    {
        NetworkingSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct RagdollSystem : public ECSSystem<RagdollSystem, RagdollComponent>
    {
        RagdollSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct RigidBodySystem : public ECSSystem<RigidBodySystem, RigidBodyComponent>
    {
        RigidBodySystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct ScriptSystem : public ECSSystem<ScriptSystem, ScriptComponent>
    {
        ScriptSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct SelectionSystem : public ECSSystem<SelectionSystem, SelectionComponent>
    {
        SelectionSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct UnitSystem : public ECSSystem<UnitSystem, UnitComponent>
    {
        UnitSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
    struct EnvProbeSystem : public ECSSystem<EnvProbeSystem, EnvironmentProbeComponent>
    {
        EnvProbeSystem(ECS::ECSEngine& parentEngine) : ECSSystem(parentEngine) {}
    };
};

ECSManager::ECSManager(PlatformContext& context, ECS::ECSEngine& engine)
    : PlatformContextComponent(context),
      _ecsEngine(engine)
{
    auto TSys = _ecsEngine.GetSystemManager()->AddSystem<TransformSystem>(_ecsEngine, _context);
    auto ASys = _ecsEngine.GetSystemManager()->AddSystem<AnimationSystem>(_ecsEngine, _context);
    auto RSys = _ecsEngine.GetSystemManager()->AddSystem<RenderingSystem>(_ecsEngine, _context);
    auto BSys = _ecsEngine.GetSystemManager()->AddSystem<BoundsSystem>(_ecsEngine, _context);
    auto PlSys = _ecsEngine.GetSystemManager()->AddSystem<PointLightSystem>(_ecsEngine);
    auto SlSys = _ecsEngine.GetSystemManager()->AddSystem<SpotLightSystem>(_ecsEngine);
    auto DlSys = _ecsEngine.GetSystemManager()->AddSystem<DirectionalLightSystem>(_ecsEngine);

    auto IKSys = _ecsEngine.GetSystemManager()->AddSystem<IKSystem>(_ecsEngine);
    auto NavSys = _ecsEngine.GetSystemManager()->AddSystem<NavigationSystem>(_ecsEngine);
    auto NetSys = _ecsEngine.GetSystemManager()->AddSystem<NetworkingSystem>(_ecsEngine);
    auto RagSys = _ecsEngine.GetSystemManager()->AddSystem<RagdollSystem>(_ecsEngine);
    auto RBSys = _ecsEngine.GetSystemManager()->AddSystem<RigidBodySystem>(_ecsEngine);
    auto ScpSys = _ecsEngine.GetSystemManager()->AddSystem<ScriptSystem>(_ecsEngine);
    auto SelSys = _ecsEngine.GetSystemManager()->AddSystem<SelectionSystem>(_ecsEngine);
    auto UnitSys = _ecsEngine.GetSystemManager()->AddSystem<UnitSystem>(_ecsEngine);
    auto ProbeSys = _ecsEngine.GetSystemManager()->AddSystem<EnvProbeSystem>(_ecsEngine);
    
    ASys->AddDependencies(TSys);
    BSys->AddDependencies(ASys);
    RSys->AddDependencies(TSys);
    RSys->AddDependencies(BSys);
    DlSys->AddDependencies(BSys);
    PlSys->AddDependencies(DlSys);
    SlSys->AddDependencies(PlSys);
    IKSys->AddDependencies(ASys);
    UnitSys->AddDependencies(TSys);
    NavSys->AddDependencies(UnitSys);
    RagSys->AddDependencies(ASys);
    RBSys->AddDependencies(RagSys);
    NetSys->AddDependencies(RBSys);
    ScpSys->AddDependencies(UnitSys);
    SelSys->AddDependencies(UnitSys);
    ProbeSys->AddDependencies(UnitSys);

    _ecsEngine.GetSystemManager()->UpdateSystemWorkOrder();
}

ECSManager::~ECSManager()
{

}

bool ECSManager::saveCache(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) const {
    TransformSystem* tSys = _ecsEngine.GetSystemManager()->GetSystem<TransformSystem>();
    if (tSys != nullptr) {
        if (!tSys->saveCache(sgn, outputBuffer)) {
            return false;
        }
    }
    AnimationSystem* aSys = _ecsEngine.GetSystemManager()->GetSystem<AnimationSystem>();
    if (aSys != nullptr) {
        if (!aSys->saveCache(sgn, outputBuffer)) {
            return false;
        }
    }
    RenderingSystem* rSys = _ecsEngine.GetSystemManager()->GetSystem<RenderingSystem>();
    if (rSys != nullptr) {
        if (!rSys->saveCache(sgn, outputBuffer)) {
            return false;
        }
    }
    return true;
}

bool ECSManager::loadCache(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
    TransformSystem* tSys = _ecsEngine.GetSystemManager()->GetSystem<TransformSystem>();
    if (tSys != nullptr) {
        if (!tSys->loadCache(sgn, inputBuffer)) {
            return false;
        }
    }
    AnimationSystem* aSys = _ecsEngine.GetSystemManager()->GetSystem<AnimationSystem>();
    if (aSys != nullptr) {
        if (!aSys->loadCache(sgn, inputBuffer)) {
            return false;
        }
    }
    RenderingSystem* rSys = _ecsEngine.GetSystemManager()->GetSystem<RenderingSystem>();
    if (rSys != nullptr) {
        if (!rSys->loadCache(sgn, inputBuffer)) {
            return false;
        }
    }
    return true;
}

}; //namespace Divide