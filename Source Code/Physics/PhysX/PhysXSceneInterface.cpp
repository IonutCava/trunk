#include "stdafx.h"

#include "Headers/PhysXSceneInterface.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/PlatformContext.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "Headers/PhysX.h"
#include "Physics/Headers/PXDevice.h"

using namespace physx;

namespace Divide {

enum class PhysXSceneInterfaceState : U8 {
    STATE_LOADING_ACTORS
};
#ifdef USE_MBP

static void setupMBP(PxScene& scene) {
    const float range = 1000.0f;
    const PxU32 subdiv = 4;
    // const PxU32 subdiv = 1;
    // const PxU32 subdiv = 2;
    // const PxU32 subdiv = 8;

    const PxVec3 min(-range);
    const PxVec3 max(range);
    const PxBounds3 globalBounds(min, max);

    PxBounds3 bounds[256];
    const PxU32 nbRegions = PxBroadPhaseExt::createRegionsFromWorldBounds(bounds, globalBounds, subdiv);

    for (PxU32 i = 0; i < nbRegions; i++) 	{
        PxBroadPhaseRegion region;
        region.bounds = bounds[i];
        region.userData = (void*)i;
        scene.addBroadPhaseRegion(region);
    }
}
#endif //USE_MBP

PhysXSceneInterface::PhysXSceneInterface(Scene& parentScene)
    : PhysicsSceneInterface(parentScene)
{
}

PhysXSceneInterface::~PhysXSceneInterface()
{
    release();
}

bool PhysXSceneInterface::init() {
    const PhysX& physX = static_cast<PhysX&>(_parentScene.context().pfx().getImpl());
    PxPhysics* gPhysicsSDK = physX.getSDK();
    // Create the scene
    if (!gPhysicsSDK) {
        Console::errorfn(Locale::Get(_ID("ERROR_PHYSX_SDK")));
        return false;
    }

    PxSceneDesc sceneDesc(gPhysicsSDK->getTolerancesScale());
    sceneDesc.gravity = PxVec3(DEFAULT_GRAVITY.x, DEFAULT_GRAVITY.y, DEFAULT_GRAVITY.z);
    if (!sceneDesc.cpuDispatcher) {
        _cpuDispatcher = PxDefaultCpuDispatcherCreate(std::max(2u, HardwareThreadCount() - 2u));
        if (!_cpuDispatcher) {
            Console::errorfn(Locale::Get(_ID("ERROR_PHYSX_INTERFACE_CPU_DISPATCH")));
        }
        sceneDesc.cpuDispatcher = _cpuDispatcher;
    }

    if (!sceneDesc.filterShader) {
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    }

#if PX_SUPPORT_GPU_PHYSX
    if (!sceneDesc.cudaContextManager)
        sceneDesc.cudaContextManager = physX.cudaContextManager();
#endif //PX_SUPPORT_GPU_PHYSX

    //sceneDesc.frictionType = PxFrictionType::eTWO_DIRECTIONAL;
    //sceneDesc.frictionType = PxFrictionType::eONE_DIRECTIONAL;
    //sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    //sceneDesc.flags |= PxSceneFlag::eENABLE_AVERAGE_POINT;
    sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
    //sceneDesc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    sceneDesc.sceneQueryUpdateMode = PxSceneQueryUpdateMode::eBUILD_ENABLED_COMMIT_DISABLED;

    //sceneDesc.flags |= PxSceneFlag::eDISABLE_CONTACT_CACHE;
    //sceneDesc.broadPhaseType =  PxBroadPhaseType::eGPU;
    //sceneDesc.broadPhaseType = PxBroadPhaseType::eSAP;
    sceneDesc.gpuMaxNumPartitions = 8;
    //sceneDesc.solverType = PxSolverType::eTGS;
#ifdef USE_MBP
    sceneDesc.broadPhaseType = PxBroadPhaseType::eMBP;
#endif //USE_MBP

    _gScene = gPhysicsSDK->createScene(sceneDesc);
    if (!_gScene) {
        Console::errorfn(Locale::Get(_ID("ERROR_PHYSX_INTERFACE_CREATE_SCENE")));
        return false;
    }

    PxSceneWriteLock scopedLock(*_gScene);
    const PxSceneFlags flag = _gScene->getFlags();
    PX_UNUSED(flag);
    _gScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0);
    _gScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
    PxPvdSceneClient* pvdClient = _gScene->getScenePvdClient();
    if (pvdClient) {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

#ifdef USE_MBP
    setupMBP(*_gScene);
#endif //USE_MBP

    return true;
}

#define SAFE_RELEASE(X) if (X != nullptr) { X->release(); X = nullptr;}
void PhysXSceneInterface::release() {
    Console::d_printfn(Locale::Get(_ID("STOP_PHYSX_SCENE_INTERFACE")));

    idle();

    SAFE_RELEASE(_cpuDispatcher);
    SAFE_RELEASE(_gScene);
}

void PhysXSceneInterface::idle() {
    if (_gScene != nullptr) {
        PhysXActor* crtActor = nullptr;
        while (_sceneRigidQueue.try_dequeue(crtActor)) {
            _sceneRigidActors.push_back(crtActor);
            _gScene->addActor(*(crtActor->_actor));
        }
    }
}

void PhysXSceneInterface::update(const U64 deltaTimeUS) {
    if (!_gScene) {
        return;
    }

    for (PhysXActor* actor : _sceneRigidActors) {
        updateActor(*actor);
    }


    // retrieve array of actors that moved
    PxU32 nbActiveActors;
    PxActor** activeActors = _gScene->getActiveActors(nbActiveActors);

    // update each render object with the new transform
    for (PxU32 i = 0; i < nbActiveActors; ++i) {
        PxRigidActor* actor = static_cast<PxRigidActor*>(activeActors[i]);
        SceneGraphNode* node = static_cast<SceneGraphNode*>(activeActors[i]->userData);
        TransformComponent* tComp = node->get<TransformComponent>();

        PxTransform pT = actor->getGlobalPose();
        const PxQuat pQ = pT.q.getConjugate();
        const PxVec3 pP = pT.p;
        tComp->setRotation(Quaternion<F32>(pQ.x, pQ.y, pQ.z, pQ.w));
        tComp->setPosition(vec3<F32>(pP.x, pP.y, pP.z));
    }
}

void PhysXSceneInterface::updateActor(PhysXActor& actor) {
    ACKNOWLEDGE_UNUSED(actor);
}

void PhysXSceneInterface::process(const U64 deltaTimeUS) {
    if (!_gScene) {
        return;
    }

    const PxReal deltaTimeMS =  Time::MicrosecondsToMilliseconds<PxReal>(deltaTimeUS);

    _gScene->simulate(deltaTimeMS);

    while (!_gScene->fetchResults()) {
        idle();
    }
}

void PhysXSceneInterface::addRigidActor(PhysXActor* const actor) {
    assert(actor != nullptr);
    // We DO NOT take ownership of actors. Ownership remains with RigidBodyComponent
    //_sceneRigidQueue.enqueue(actor);
}

void PhysXSceneInterface::updateRigidActor(PxRigidActor* oldActor, PxRigidActor* newActor) const { 
    if (oldActor != nullptr) {
        _gScene->removeActor(*oldActor);
    }
    if (newActor != nullptr) {
        _gScene->addActor(*newActor);
    }
}
};