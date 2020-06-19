#include "stdafx.h"

#include "Headers/PhysXSceneInterface.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/PlatformContext.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "ECS/Components/Headers/TransformComponent.h"

using namespace physx;

namespace Divide {

enum class PhysXSceneInterfaceState : U8 {
    STATE_LOADING_ACTORS
};

PhysXSceneInterface::PhysXSceneInterface(Scene& parentScene)
    : PhysicsSceneInterface(parentScene),
      _gScene(nullptr),
      _cpuDispatcher(nullptr)
{
}

PhysXSceneInterface::~PhysXSceneInterface() {
    release();
}

bool PhysXSceneInterface::init() {
    physx::PxPhysics* gPhysicsSDK = static_cast<PhysX&>(_parentScene.context().pfx().getImpl()).getSDK();
    // Create the scene
    if (!gPhysicsSDK) {
        Console::errorfn(Locale::get(_ID("ERROR_PHYSX_SDK")));
        return false;
    }

    PxSceneDesc sceneDesc(gPhysicsSDK->getTolerancesScale());
    sceneDesc.gravity =
        PxVec3(DEFAULT_GRAVITY.x, DEFAULT_GRAVITY.y, DEFAULT_GRAVITY.z);
    if (!sceneDesc.cpuDispatcher) {
        _cpuDispatcher = PxDefaultCpuDispatcherCreate(3);
        if (!_cpuDispatcher) {
            Console::errorfn(Locale::get(_ID("ERROR_PHYSX_INTERFACE_CPU_DISPATCH")));
        }
        sceneDesc.cpuDispatcher = _cpuDispatcher;
    }

    if (!sceneDesc.filterShader) {
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    }
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

    _gScene = gPhysicsSDK->createScene(sceneDesc);
    if (!_gScene) {
        Console::errorfn(Locale::get(_ID("ERROR_PHYSX_INTERFACE_CREATE_SCENE")));
        return false;
    }

    _gScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0);
    _gScene->setVisualizationParameter(
        PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
    PxMaterial* planeMaterial = gPhysicsSDK->createMaterial(0.9f, 0.1f, 1.0f);
    _materials.push_back(planeMaterial);
    PxMaterial* cubeMaterial = gPhysicsSDK->createMaterial(0.1f, 0.4f, 1.0f);
    _materials.push_back(cubeMaterial);
    PxMaterial* sphereMaterial = gPhysicsSDK->createMaterial(0.6f, 0.1f, 0.6f);
    _materials.push_back(sphereMaterial);
    return true;
}

void PhysXSceneInterface::release() {
    if (!_gScene) {
        return;
    }

    Console::d_printfn(Locale::get(_ID("STOP_PHYSX_SCENE_INTERFACE")));

    idle();
    for (PhysXActor* actor : _sceneRigidActors) {
        actor->_actor->release();
    }

    MemoryManager::DELETE_CONTAINER(_sceneRigidActors);

    if (_cpuDispatcher) {
        _cpuDispatcher->release();
    }
    _gScene->release();
    _gScene = nullptr;
}

void PhysXSceneInterface::idle() {
    PhysXActor* crtActor = nullptr;
    while (_sceneRigidQueue.try_dequeue(crtActor)) {
        assert(crtActor != nullptr);
        _sceneRigidActors.push_back(crtActor);
        addToScene(*crtActor);
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
    PxU32 nbActiveTransforms;
    const PxActiveTransform* activeTransforms = _gScene->getActiveTransforms(nbActiveTransforms);

    // update each SGN with the new transform
    for (PxU32 i = 0; i < nbActiveTransforms; ++i) {
        PxRigidActor* actor = static_cast<PxRigidActor*>(activeTransforms[i].actor);
        TransformComponent* tComp = static_cast<TransformComponent*>(actor->userData);
        PxTransform pT = actor->getGlobalPose();
        PxQuat pQ = pT.q.getConjugate();
        PxVec3 pP = pT.p;
        tComp->setRotation(Quaternion<F32>(pQ.x, pQ.y, pQ.z, pQ.w));
        tComp->setPosition(vec3<F32>(pP.x, pP.y, pP.z));
    }
}

void PhysXSceneInterface::updateActor(PhysXActor& actor) {

}

void PhysXSceneInterface::process(const U64 deltaTimeUS) {
    if (!_gScene) {
        return;
    }

    physx::PxReal deltaTimeMS = 
        Time::MicrosecondsToMilliseconds<physx::PxReal>(deltaTimeUS);

    _gScene->simulate(deltaTimeMS);

    while (!_gScene->fetchResults()) {
        idle();
    }
}

void PhysXSceneInterface::addRigidActor(PhysXActor* const actor,
                                        bool threaded) {
    assert(actor != nullptr);

    if (threaded) {
        _sceneRigidQueue.enqueue(actor);
    } else {
        _sceneRigidActors.push_back(actor);
    }
}

void PhysXSceneInterface::addToScene(PhysXActor& actor) {
    constexpr U32 normalMask = to_base(ComponentType::NAVIGATION) |
                               to_base(ComponentType::TRANSFORM) |
                               to_base(ComponentType::RIGID_BODY) |
                               to_base(ComponentType::BOUNDS) |
                               to_base(ComponentType::RENDERING) |
                               to_base(ComponentType::NETWORKING);

   /* STUBBED("ToDo: Only 1 shape per actor for now. Also, maybe use a factory or something. Fix This! -Ionut")
    SceneNode* sceneNode = nullptr;

    _gScene->addActor(*(actor._actor));
    U32 nbActors =
        _gScene->getNbActors(PxActorTypeSelectionFlag::eRIGID_DYNAMIC |
                             PxActorTypeSelectionFlag::eRIGID_STATIC);
    PxShape** shapes =
        MemoryManager_NEW PxShape * [actor._actor->getNbShapes()];
    actor._actor->getShapes(shapes, actor._actor->getNbShapes());

    SceneGraphNode* targetNode;
    stringImpl sgnName = "";
    bool shadowState = true;
    switch (actor._type) {
        case PxGeometryType::eBOX: {
            ResourceDescriptor boxDescriptor("BoxActor");
            sceneNode = CreateResource<Box3D>(boxDescriptor);

            sgnName = "BoxActor_" + Util::to_string(nbActors);

            if (sceneNode->GetRef() == 1) {
                Material* boxMaterial = CreateResource<Material>(
                    ResourceDescriptor("BoxActor_material"));
                boxMaterial->setDiffuse(vec4<F32>(0.0f, 0.0f, 1.0f, 1.0f));
                boxMaterial->setEmissive(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
                boxMaterial->setShininess(2);
                boxMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
                sceneNode->setMaterialTpl(boxMaterial);
            }
        } break;

        case PxGeometryType::ePLANE: {
            sgnName = "PlaneActor";
            targetNode = parentScene.sceneGraph().findNode(sgnName).lock();
            if (targetNode != nullptr) {
                actor.setParent(targetNode->get<RigidBodyComponent>());
                return;
            }

            ResourceDescriptor planeDescriptor(sgnName);
            planeDescriptor.setFlag(true);
            sceneNode = CreateResource<Quad3D>(planeDescriptor);
            Material* planeMaterial = CreateResource<Material>(
                ResourceDescriptor(sgnName + "_material"));
            planeMaterial->setDiffuse(vec4<F32>(0.4f, 0.1f, 0.1f, 1.0f));
            planeMaterial->setEmissive(vec4<F32>(0.3f, 0.3f, 0.3f, 1.0f));
            planeMaterial->setShininess(0);
            planeMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
            shadowState = false;
            sceneNode->setMaterialTpl(planeMaterial);
        } break;
    }

    MemoryManager::DELETE_ARRAY(shapes);

    if (actor._type != PxGeometryType::eTRIANGLEMESH) {
        if (sceneNode) {
            sceneNode->renderState().setDrawState(true);
            targetNode =
                _parentScene->sceneGraph().getRoot().addChildNode(sceneNode, normalMask, sgnName);
            targetNode->get<RenderingComponent>()->castsShadows(
                shadowState);
        }

        actor._actorName = sgnName;
        actor.getParent()->setScale(actor._userData);
        actor._actor->userData = actor.getParent();
    } 

    assert(targetNode != nullptr);
    actor.setParent(targetNode->get<RigidBodyComponent>());
    */
}
};