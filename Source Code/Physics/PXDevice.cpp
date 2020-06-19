#include "stdafx.h"

#include "Headers/PXDevice.h"
#include "Headers/PhysicsAsset.h"

#include "Utility/Headers/Localization.h"

#include "Physics/PhysX/Headers/PhysX.h"

#ifndef _PHYSICS_API_FOUND_
#error "No physics library implemented!"
#endif

namespace Divide {
    
PXDevice::PXDevice(Kernel& parent)
    : KernelComponent(parent), 
      PhysicsAPIWrapper(),
      _simulationSpeed(1.0f),
      _API_ID(PhysicsAPI::COUNT),
      _api(nullptr)
{
}

PXDevice::~PXDevice() 
{
    closePhysicsAPI();
}

ErrorCode PXDevice::initPhysicsAPI(U8 targetFrameRate, F32 simSpeed) {
    DIVIDE_ASSERT(_api == nullptr,
                "PXDevice error: initPhysicsAPI called twice!");
    switch (_API_ID) {
        case PhysicsAPI::PhysX: {
            _api = eastl::make_unique<PhysX>();
        } break;
        case PhysicsAPI::ODE: 
        case PhysicsAPI::Bullet: 
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_PFX_DEVICE_API")));
            return ErrorCode::PFX_NON_SPECIFIED;
        };
    };
    _simulationSpeed = simSpeed;
    return _api->initPhysicsAPI(targetFrameRate, _simulationSpeed);
}

bool PXDevice::closePhysicsAPI() { 
    if (_api == nullptr) {
        Console::errorfn(Locale::get(_ID("ERROR_PFX_DEVICE_NO_INIT")));
        return false;
    }

    Console::printfn(Locale::get(_ID("STOP_PHYSICS_INTERFACE")));
    const bool state = _api->closePhysicsAPI();
    _api.reset();

    return state;
}

void PXDevice::updateTimeStep(const U8 timeStepFactor, const F32 simSpeed) {
    _api->updateTimeStep(timeStepFactor, simSpeed);
}

void PXDevice::update(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    _api->update(deltaTimeUS);
}

void PXDevice::process(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    _api->process(deltaTimeUS);
}

void PXDevice::idle() {
    OPTICK_EVENT();

    _api->idle();
}

void PXDevice::setPhysicsScene(PhysicsSceneInterface* const targetScene) {
    _api->setPhysicsScene(targetScene);
}

PhysicsSceneInterface* PXDevice::NewSceneInterface(Scene& scene) {
    return _api->NewSceneInterface(scene);
}

PhysicsAsset* PXDevice::createRigidActor(const SceneGraphNode* node, RigidBodyComponent& parentComp) {
    return _api->createRigidActor(node, parentComp);
}

}; //namespace Divide