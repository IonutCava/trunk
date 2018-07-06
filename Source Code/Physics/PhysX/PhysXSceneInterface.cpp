#include "Headers/PhysXSceneInterface.h"

#include "Scenes/Headers/Scene.h"
#include "Graphs/Headers/SceneNode.h"
#include "Graphs/Components/Headers/PhysicsComponent.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

using namespace physx;

namespace Divide {

enum class PhysXSceneInterfaceState : U32 {
    STATE_LOADING_ACTORS
};

PhysXSceneInterface::PhysXSceneInterface(Scene* parentScene)
    : PhysicsSceneInterface(parentScene),
      _gScene(nullptr),
      _cpuDispatcher(nullptr) {}

PhysXSceneInterface::~PhysXSceneInterface() { release(); }

bool PhysXSceneInterface::init() {
    physx::PxPhysics* gPhysicsSDK = PhysX::getInstance().getSDK();
    // Create the scene
    if (!gPhysicsSDK) {
        Console::errorfn(Locale::get("ERROR_PHYSX_SDK"));
        return false;
    }

    PxSceneDesc sceneDesc(gPhysicsSDK->getTolerancesScale());
    sceneDesc.gravity =
        PxVec3(DEFAULT_GRAVITY.x, DEFAULT_GRAVITY.y, DEFAULT_GRAVITY.z);
    if (!sceneDesc.cpuDispatcher) {
        _cpuDispatcher = PxDefaultCpuDispatcherCreate(3);
        if (!_cpuDispatcher) {
            Console::errorfn(Locale::get("ERROR_PHYSX_INTERFACE_CPU_DISPATCH"));
        }
        sceneDesc.cpuDispatcher = _cpuDispatcher;
    }

    if (!sceneDesc.filterShader) {
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    }
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

    _gScene = gPhysicsSDK->createScene(sceneDesc);
    if (!_gScene) {
        Console::errorfn(Locale::get("ERROR_PHYSX_INTERFACE_CREATE_SCENE"));
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
        Console::errorfn(Locale::get("ERROR_PHYSX_CLOSE_INVALID_INTERFACE"));
        return;
    }

    Console::d_printfn(Locale::get("STOP_PHYSX_SCENE_INTERFACE"));

    idle();
    for (PhysXActor* actor : _sceneRigidActors) {
        actor->_actor->release();
    }

    MemoryManager::DELETE_VECTOR(_sceneRigidActors);

    if (_cpuDispatcher) {
        _cpuDispatcher->release();
    }
    _gScene->release();
    _gScene = nullptr;
}

void PhysXSceneInterface::idle() {
    if (_sceneRigidQueue.empty()) {
        return;
    }
    PhysXActor* crtActor = nullptr;
    while (_sceneRigidQueue.pop(crtActor)) {
        assert(crtActor != nullptr);
        _sceneRigidActors.push_back(crtActor);
        addToScene(*crtActor);
    }
}

void PhysXSceneInterface::update(const U64 deltaTime) {
    if (!_gScene) {
        return;
    }

    _sceneRigidActors.erase(
        std::remove_if(std::begin(_sceneRigidActors),
                       std::end(_sceneRigidActors),
                       [](PhysXActor* actor)
                           -> bool { return actor->getParent() == nullptr; }),
        std::end(_sceneRigidActors));

    for (PhysXActor* actor : _sceneRigidActors) {
        updateActor(*actor);
    }


    // retrieve array of actors that moved
    PxU32 nbActiveTransforms;
    const PxActiveTransform* activeTransforms = _gScene->getActiveTransforms(nbActiveTransforms);

    // update each SGN with the new transform
    for (PxU32 i = 0; i < nbActiveTransforms; ++i) {
        PxRigidActor* actor = static_cast<PxRigidActor*>(activeTransforms[i].actor);
        PhysicsComponent* pComp = static_cast<PhysicsComponent*>(actor->userData);
        PxTransform pT = actor->getGlobalPose();
        PxQuat pQ = pT.q.getConjugate();
        PxVec3 pP = pT.p;
        pComp->setRotation(Quaternion<F32>(pQ.x, pQ.y, pQ.z, pQ.w));
        pComp->setPosition(vec3<F32>(pP.x, pP.y, pP.z));
    }
}

void PhysXSceneInterface::updateActor(PhysXActor& actor) {
    if (!actor.resetTransforms()) {
        return;
    }

    const vec3<F32>& position = actor.getParent()->getPosition();
    const vec4<F32>& orientation = actor.getParent()->getOrientation().asVec4();
    actor._actor->setGlobalPose(physx::PxTransform(
                                    PxVec3(position.x, position.y, position.z),
                                    PxQuat(orientation.x, orientation.y, orientation.z, orientation.w).getConjugate()));
    actor.resetTransforms(false);
}

void PhysXSceneInterface::process(const U64 deltaTime) {
    if (!_gScene) {
        return;
    }

    physx::PxReal deltaTimeMS = 
        Time::MicrosecondsToMilliseconds<physx::PxReal>(deltaTime);

    _gScene->simulate(deltaTimeMS);

    while (!_gScene->fetchResults()) {
        idle();
    }
}

void PhysXSceneInterface::addRigidActor(PhysXActor* const actor,
                                        bool threaded) {
    assert(actor != nullptr);

    if (threaded) {
        while (!_sceneRigidQueue.push(actor)) {
            idle();
        }
    } else {
        _sceneRigidActors.push_back(actor);
    }
}

PhysXActor* PhysXSceneInterface::getOrCreateRigidActor(const stringImpl& actorName) {
    if (!_gScene) {
        return nullptr;
    }

    for (RigidMap::iterator it = std::begin(_sceneRigidActors);
         it != std::end(_sceneRigidActors); ++it) {
        if ((*it)->_actorName.compare(actorName) == 0) {
            return (*it);
        }
    }

    PhysXActor* newActor = MemoryManager_NEW PhysXActor();
    newActor->_actorName = actorName;
    return newActor;
}

void PhysXSceneInterface::addToScene(PhysXActor& actor) {
    STUBBED("ToDo: Only 1 shape per actor for now. Also, maybe use a factory or something. Fix This! -Ionut")
    SceneNode* sceneNode = nullptr;

    _gScene->addActor(*(actor._actor));
    U32 nbActors =
        _gScene->getNbActors(PxActorTypeSelectionFlag::eRIGID_DYNAMIC |
                             PxActorTypeSelectionFlag::eRIGID_STATIC);
    PxShape** shapes =
        MemoryManager_NEW PxShape * [actor._actor->getNbShapes()];
    actor._actor->getShapes(shapes, actor._actor->getNbShapes());

    SceneGraphNode_ptr targetNode;
    stringImpl sgnName = "";
    bool shadowState = true;
    switch (actor._type) {
        case PxGeometryType::eBOX: {
            ResourceDescriptor boxDescriptor("BoxActor");
            sceneNode = CreateResource<Box3D>(boxDescriptor);

            sgnName = "BoxActor_" + std::to_string(nbActors);

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
            if (FindResourceImpl<Quad3D>(sgnName)) {
                targetNode = GET_ACTIVE_SCENEGRAPH().findNode(sgnName).lock();
                assert(targetNode);
                actor.setParent(targetNode->getComponent<PhysicsComponent>());
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
                _parentScene->getSceneGraph().getRoot()->addNode(*sceneNode, sgnName);
            targetNode->getComponent<RenderingComponent>()->castsShadows(
                shadowState);
        }

        actor._actorName = sgnName;
        actor.getParent()->setScale(actor._userData);
        actor._actor->userData = actor.getParent();
    } 

    assert(targetNode != nullptr);
    actor.setParent(targetNode->getComponent<PhysicsComponent>());
}
};