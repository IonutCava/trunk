#include "Headers/PXDevice.h"
#include "Headers/PhysicsAsset.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
    
PXDevice::PXDevice() : _API_ID(PhysicsAPI::COUNT),
                       _api(nullptr)
{
}

PXDevice::~PXDevice() 
{
}

ErrorCode PXDevice::initPhysicsAPI(U8 targetFrameRate) {
    DIVIDE_ASSERT(_api == nullptr,
                "PXDevice error: initPhysicsAPI called twice!");
    switch (_API_ID) {
        case PhysicsAPI::PhysX: {
            _api = &PhysX::instance();
        } break;
        case PhysicsAPI::ODE: 
        case PhysicsAPI::Bullet: 
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_PFX_DEVICE_API")));
            return ErrorCode::PFX_NON_SPECIFIED;
        } break;
    };

    return _api->initPhysicsAPI(targetFrameRate);
}

bool PXDevice::closePhysicsAPI() { 
    DIVIDE_ASSERT(_api != nullptr,
            "PXDevice error: closePhysicsAPI called without init!");
        
    bool state = _api->closePhysicsAPI();

    switch (_API_ID) {
        case PhysicsAPI::PhysX: {
            PhysX::destroyInstance();
        } break;
        case PhysicsAPI::ODE:
        case PhysicsAPI::Bullet:
        default: {
        } break; 
    };

    return state;
}

void PXDevice::updateTimeStep(U8 timeStepFactor) {
    _api->updateTimeStep(timeStepFactor);
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

void PXDevice::initScene() {
    _api->initScene();
}

PhysicsSceneInterface* PXDevice::NewSceneInterface(Scene* scene) {
    return _api->NewSceneInterface(scene);
}

PhysicsAsset* PXDevice::createRigidActor(const SceneGraphNode& node,
                                         PhysicsActorMask mask,
                                         PhysicsCollisionGroup group) {
    return _api->createRigidActor(node, mask, group);
}

}; //namespace Divide