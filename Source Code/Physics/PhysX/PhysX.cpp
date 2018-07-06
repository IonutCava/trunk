#include "Headers/PhysX.h"
#include "Headers/PhysXSceneInterface.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

#include <Samples/PxToolkit/include/PxToolkit.h>
#include <Samples/PxToolkit/include/PxTkNamespaceMangle.h>
#include <PsFile.h>

namespace Divide {

physx::PxDefaultAllocator PhysX::_gDefaultAllocatorCallback;
physx::PxDefaultErrorCallback PhysX::_gDefaultErrorCallback;

physx::PxProfileZone* PhysX::getOrCreateProfileZone(
    physx::PxFoundation& inFoundation) {
#ifdef PHYSX_PROFILE_SDK
    if (_profileZone == nullptr) {
        _profileZone =
            &physx::PxProfileZone::createProfileZone(
                &inFoundation, "SampleProfileZone", gProfileNameProvider);
    }
#endif
    return _profileZone;
}

PhysX::PhysX()
    : _gPhysicsSDK(nullptr),
      _foundation(nullptr),
      _zoneManager(nullptr),
      _cooking(nullptr),
      _profileZone(nullptr),
      _pvdConnection(nullptr),
      _targetScene(nullptr),
      _accumulator(0.0f),
      _timeStepFactor(0.0f),
      _timeStep(0.0f) {}

ErrorCode PhysX::initPhysicsAPI(U8 targetFrameRate) {
    Console::printfn(Locale::get("START_PHYSX_API"));

    // create foundation object with default error and allocator callbacks.
    _foundation = PxCreateFoundation(PX_PHYSICS_VERSION,
                                     _gDefaultAllocatorCallback,
                                     _gDefaultErrorCallback);
    assert(_foundation != nullptr);

    bool recordMemoryAllocations = false;

#ifdef _DEBUG
    recordMemoryAllocations = true;
    _zoneManager = 
        &physx::PxProfileZoneManager::createProfileZoneManager(_foundation);
    assert(_zoneManager != nullptr);
#endif

    _gPhysicsSDK =
        PxCreatePhysics(PX_PHYSICS_VERSION, 
                        *_foundation, physx::PxTolerancesScale(),
                        recordMemoryAllocations, _zoneManager);

    if (_gPhysicsSDK == nullptr) {
        Console::errorfn(Locale::get("ERROR_START_PHYSX_API"));
        return PHYSX_INIT_ERROR;
    }

#ifdef _DEBUG
    if (getOrCreateProfileZone(*_foundation)) {
        _zoneManager->addProfileZone(*_profileZone);
    }

    _pvdConnection = _gPhysicsSDK->getPvdConnectionManager();
    _gPhysicsSDK->getPvdConnectionManager()->addHandler(*this);

    if (_pvdConnection != nullptr) {
        if (physx::PxVisualDebuggerExt::createConnection(_pvdConnection,
                                                         "localhost",
                                                         5425, 10000) 
                                                         != nullptr) {
            Console::d_printfn(Locale::get("CONNECT_PVD_OK"));
        }
    }
#endif

    if (!PxInitExtensions(*_gPhysicsSDK)) {
        Console::errorfn(Locale::get("ERROR_EXTENSION_PHYSX_API"));
        return PHYSX_EXTENSION_ERROR;
    }

    if (!_cooking) {
        physx::PxCookingParams* cookparams =
            MemoryManager_NEW physx::PxCookingParams(physx::PxTolerancesScale());
        cookparams->targetPlatform = physx::PxPlatform::ePC;
        _cooking =
            PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, *cookparams);
        MemoryManager::DELETE(cookparams);
    }

    updateTimeStep(targetFrameRate);
    Console::printfn(Locale::get("START_PHYSX_API_OK"));

    return NO_ERR;
}

bool PhysX::closePhysicsAPI() {
    if (!_gPhysicsSDK) {
        return false;
    }

    Console::printfn(Locale::get("STOP_PHYSX_API"));

    DIVIDE_ASSERT(_targetScene == nullptr,
                  "PhysX error: target scene not destroyed before calling "
                  "closePhysicsAPI."
                  "Call \"setPhysicsScene( nullptr )\" first");

    if (_cooking) {
        _cooking->release();
    }
    PxCloseExtensions();
    if (_zoneManager) {
        //_zoneManager->release();
    }
    _gPhysicsSDK->release();
    _foundation->release();

    return true;
}

inline void PhysX::updateTimeStep(U8 timeStepFactor) {
    CLAMP<U8>(timeStepFactor, 1, timeStepFactor);
    _timeStepFactor = timeStepFactor;
    _timeStep = 1.0f / _timeStepFactor;
    _timeStep *= ParamHandler::getInstance().getParam<F32>("simSpeed");
}

/// Process results
void PhysX::process(const U64 deltaTime) {
    if (_targetScene && _timeStep > 0.0f) {
        _accumulator +=
            Time::MicrosecondsToMilliseconds<physx::PxReal>(deltaTime);

        if (_accumulator < _timeStep) {
            return;
        }

        _accumulator -= _timeStep;
        _targetScene->process(_timeStep);
    }
}

/// Update actors
void PhysX::update(const U64 deltaTime) {
    if (_targetScene) {
        _targetScene->update(deltaTime);
    }
}

void PhysX::idle() {
    if (_targetScene) {
        _targetScene->idle();
    }
}

PhysicsSceneInterface* PhysX::NewSceneInterface(Scene* scene) {
    return MemoryManager_NEW PhysXSceneInterface(scene);
}

void PhysX::setPhysicsScene(PhysicsSceneInterface* const targetScene) {
    if (_targetScene) {
        MemoryManager::DELETE(_targetScene);
    }
    _targetScene = targetScene;
}

void PhysX::initScene() {
    DIVIDE_ASSERT(
        _targetScene != nullptr,
        "PhysX error: no target scene specified before a call to initScene()");
    _targetScene->init();
}

bool PhysX::createActor(SceneGraphNode& node, const stringImpl& sceneName,
                        PhysicsActorMask mask, PhysicsCollisionGroup group) {
    assert(_targetScene != nullptr);

    Object3D* sNode = node.getNode<Object3D>();

    // Load cached version from file first
    stringImpl nodeName("XML/Scenes/" + sceneName + "/collisionMeshes/node_[_" +
                        sNode->getName() + "_]");
    nodeName.append(".cm");

    FILE* fp = fopen(nodeName.c_str(), "rb");
    bool ok = false;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        physx::PxU32 filesize = ftell(fp);
        fclose(fp);
        ok = (filesize != 0);
    }

    if (group == GROUP_NON_COLLIDABLE) {
        // return true;
    }

    if (!ok) {
        sNode->computeTriangleList();
        const vectorImpl<vec3<U32> >& triangles = sNode->getTriangles();

        if (sNode->getTriangles().empty()) {
            return false;
        }

        VertexBuffer* nodeVB = sNode->getGeometryVB();
        if (sNode->getObjectType() == Object3D::ObjectType::SUBMESH) {
            nodeVB = node.getParent()->getNode<Object3D>()->getGeometryVB();
        }

        physx::PxDefaultFileOutputStream stream(nodeName.c_str());

        physx::PxTriangleMeshDesc meshDesc;
        meshDesc.points.count = (physx::PxU32)nodeVB->getPosition().size();
        meshDesc.points.stride = sizeof(vec3<F32>);
        meshDesc.points.data = &nodeVB->getPosition()[0];
        meshDesc.triangles.count = (physx::PxU32)triangles.size();
        meshDesc.triangles.stride = 3 * sizeof(U32);
        meshDesc.triangles.data = triangles.data();
        if (!nodeVB->usesLargeIndices()) {
            // meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;
        }

        if (!_cooking->cookTriangleMesh(meshDesc, stream)) {
            Console::errorfn(Locale::get("ERROR_COOK_TRIANGLE_MESH"));
            return false;
        }

    } else {
        Console::printfn(Locale::get("COLLISION_MESH_LOADED_FROM_FILE"),
                         nodeName.c_str());
    }

    PhysXSceneInterface* targetScene =
        dynamic_cast<PhysXSceneInterface*>(_targetScene);
    PhysicsComponent* nodePhysics =
        sNode->getObjectType() == Object3D::ObjectType::SUBMESH
            ? node.getParent()->getComponent<PhysicsComponent>()
            : node.getComponent<PhysicsComponent>();

    PhysXActor* tempActor = targetScene->getOrCreateRigidActor(node.getName());
    assert(tempActor != nullptr);
    tempActor->setParent(nodePhysics);

    if (!tempActor->_actor) {
        const vec3<F32>& position = nodePhysics->getPosition();
        const vec4<F32>& orientation = nodePhysics->getOrientation().asVec4();

        physx::PxTransform posePxTransform(
            physx::PxVec3(position.x, position.y, position.z),
            physx::PxQuat(orientation.x,
                          orientation.y,
                          orientation.z,
                          orientation.w).getConjugate());

        if (mask != MASK_RIGID_DYNAMIC) {
            tempActor->_actor =
                _gPhysicsSDK->createRigidStatic(posePxTransform);
        } else {
            tempActor->_actor =
                _gPhysicsSDK->createRigidDynamic(posePxTransform);
            tempActor->_isDynamic = true;
        }

        if (!tempActor->_actor) {
            MemoryManager::DELETE(tempActor);
            return false;
        }

        tempActor->_type = physx::PxGeometryType::eTRIANGLEMESH;
        // If we got here, the new actor was just created (didn't exist
        // previously in the scene), so add it
        targetScene->addRigidActor(tempActor, false);
    }

    assert(tempActor->_actor);

    physx::PxDefaultFileInputData stream(nodeName.c_str());
    physx::PxTriangleMesh* triangleMesh =
        _gPhysicsSDK->createTriangleMesh(stream);

    if (!triangleMesh) {
        Console::errorfn(Locale::get("ERROR_CREATE_TRIANGLE_MESH"));
        return false;
    }

    const vec3<F32>& scale = tempActor->getComponent()->getScale();
    physx::PxTriangleMeshGeometry triangleGeometry(
        triangleMesh,
        physx::PxMeshScale(physx::PxVec3(scale.x, scale.y, scale.z), 
                           physx::PxQuat(physx::PxIdentity)));
    tempActor->_actor->createShape(
        triangleGeometry, *_gPhysicsSDK->createMaterial(0.7f, 0.7f, 1.0f));

    return true;
};
};