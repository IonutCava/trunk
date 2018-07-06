#include "stdafx.h"

#include "Headers/PXDevice.h"
#include "Headers/PhysicsAsset.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
    
PXDevice::PXDevice(Kernel& parent)
    : KernelComponent(parent), 
      PhysicsAPIWrapper(),
      _API_ID(PhysicsAPI::COUNT),
      _api(nullptr),
      _simulationSpeed(1.0f)
{
}

PXDevice::~PXDevice() 
{
    assert(_api == nullptr);
}

ErrorCode PXDevice::initPhysicsAPI(U8 targetFrameRate, F32 simSpeed) {
    DIVIDE_ASSERT(_api == nullptr,
                "PXDevice error: initPhysicsAPI called twice!");
    switch (_API_ID) {
        case PhysicsAPI::PhysX: {
            _api = std::make_unique<PhysX>();
        } break;
        case PhysicsAPI::ODE: 
        case PhysicsAPI::Bullet: 
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_PFX_DEVICE_API")));
            return ErrorCode::PFX_NON_SPECIFIED;
        } break;
    };
    _simulationSpeed = simSpeed;
    return _api->initPhysicsAPI(targetFrameRate, _simulationSpeed);
}

bool PXDevice::closePhysicsAPI() { 
    if (_api == nullptr) {
        Console::errorfn(Locale::get(_ID("ERROR_PFX_DEVICE_NO_INIT")));
        return false;
    }

    bool state = _api->closePhysicsAPI();
    _api.reset();

    return state;
}

void PXDevice::updateTimeStep(U8 timeStepFactor, F32 simSpeed) {
    _api->updateTimeStep(timeStepFactor, simSpeed);
}

void PXDevice::update(const U64 deltaTime) {
    _api->update(deltaTime);
}

void PXDevice::process(const U64 deltaTime) {
    _api->process(deltaTime);
}

void PXDevice::idle() {
    _api->idle();
}

void PXDevice::setPhysicsScene(PhysicsSceneInterface* const targetScene) {
    _api->setPhysicsScene(targetScene);
}

PhysicsSceneInterface* PXDevice::NewSceneInterface(Scene& scene) {
    return _api->NewSceneInterface(scene);
}

PhysicsAsset* PXDevice::createRigidActor(const SceneGraphNode& node) {
    return _api->createRigidActor(node);
}

}; //namespace Divide