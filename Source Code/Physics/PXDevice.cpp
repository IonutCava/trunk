#include "Headers/PXDevice.h"

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
            _api = &PhysX::getInstance();
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
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: updateTimeStep(factor) called without init!");

    _api->updateTimeStep(timeStepFactor);
}

void PXDevice::updateTimeStep() {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: updateTimeStep called without init!");

    _api->updateTimeStep(); 
}

void PXDevice::update(const U64 deltaTime) {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: update called without init!");

    _api->update(deltaTime);
}

void PXDevice::process(const U64 deltaTime) {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: process called without init!");

    _api->process(deltaTime);
}

void PXDevice::idle() {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: idle called without init!");

    _api->idle();
}
void PXDevice::setPhysicsScene(PhysicsSceneInterface* const targetScene) {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: setPhysicsScene called without init!");

    _api->setPhysicsScene(targetScene);
}

void PXDevice::initScene() {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: initScene called without init!");

    _api->initScene();
}

PhysicsSceneInterface* PXDevice::NewSceneInterface(Scene* scene) {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: NewSceneInterface called without init!");

    return _api->NewSceneInterface(scene);
}

void PXDevice::createPlane(const vec3<F32>& position, U32 size) {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: createPlane called without init!");

    _api->createPlane(position, size);
}

void PXDevice::createBox(const vec3<F32>& position, F32 size) {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: createBox called without init!");

    _api->createBox(position, size);
}

void PXDevice::createActor(SceneGraphNode& node, const stringImpl& sceneName,
                           PhysicsActorMask mask, PhysicsCollisionGroup group) {
    DIVIDE_ASSERT(_api != nullptr,
        "PXDevice error: createActor called without init!");

    _api->createActor(node, sceneName, mask, group);
}

}; //namespace Divide