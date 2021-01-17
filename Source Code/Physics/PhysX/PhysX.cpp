#include "stdafx.h"

#include "Headers/PhysX.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Headers/PhysXSceneInterface.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Utility/Headers/Localization.h"

// Connecting the SDK to Visual Debugger
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultErrorCallback.h>
#include <pvd/PxPvd.h>
// PhysX includes //

namespace Divide {

namespace {
    const char* g_pvd_target_ip = "127.0.0.1";
    physx::PxU32 g_pvd_target_port = 5425;
    physx::PxU32 g_pvd_target_timeout_ms = 10;
};

physx::PxDefaultAllocator PhysX::_gDefaultAllocatorCallback;
physx::PxDefaultErrorCallback PhysX::_gDefaultErrorCallback;
hashMap<stringImpl, physx::PxTriangleMesh*> PhysX::_gMeshCache;


PhysX::~PhysX()
{
    assert(_gPhysicsSDK == nullptr);
}

ErrorCode PhysX::initPhysicsAPI(const U8 targetFrameRate, const F32 simSpeed) {
    Console::printfn(Locale::get(_ID("START_PHYSX_API")));

    _simulationSpeed = simSpeed;
    // create foundation object with default error and allocator callbacks.
    _foundation = PxCreateFoundation(PX_PHYSICS_VERSION,
                                     _gDefaultAllocatorCallback,
                                     _gDefaultErrorCallback);
    assert(_foundation != nullptr);

    constexpr bool recordMemoryAllocations = (Config::Build::IS_DEBUG_BUILD || Config::Build::IS_PROFILE_BUILD);

    if_constexpr(recordMemoryAllocations) {
        createPvdConnection(g_pvd_target_ip,
                            g_pvd_target_port,
                            g_pvd_target_timeout_ms,
                            Config::Build::IS_DEBUG_BUILD);
    }

    _gPhysicsSDK =
        PxCreatePhysics(PX_PHYSICS_VERSION, 
                        *_foundation, physx::PxTolerancesScale(),
                        recordMemoryAllocations, _pvd);

    if (_gPhysicsSDK == nullptr) {
        Console::errorfn(Locale::get(_ID("ERROR_START_PHYSX_API")));
        return ErrorCode::PHYSX_INIT_ERROR;
    }

    if (!PxInitExtensions(*_gPhysicsSDK, _pvd)) {
        Console::errorfn(Locale::get(_ID("ERROR_EXTENSION_PHYSX_API")));
        return ErrorCode::PHYSX_EXTENSION_ERROR;
    }

    if (!_cooking) {
        physx::PxCookingParams* cookparams = MemoryManager_NEW physx::PxCookingParams(physx::PxTolerancesScale());
        _cooking = PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, *cookparams);
        MemoryManager::DELETE(cookparams);
    }

    updateTimeStep(targetFrameRate, _simulationSpeed);
    Console::printfn(Locale::get(_ID("START_PHYSX_API_OK")));

    return ErrorCode::NO_ERR;
}

bool PhysX::closePhysicsAPI() {
    if (!_gPhysicsSDK) {
        return false;
    }

    Console::printfn(Locale::get(_ID("STOP_PHYSX_API")));

    DIVIDE_ASSERT(_targetScene == nullptr,
                  "PhysX error: target scene not destroyed before calling "
                  "closePhysicsAPI."
                  "Call \"setPhysicsScene( nullptr )\" first");

    if (_cooking != nullptr) {
        _cooking->release();
    }
    PxCloseExtensions();
    _gPhysicsSDK->release();
    if (_pvd != nullptr) {
        _pvd->release();
        _pvd = nullptr;
    }
    if (_transport != nullptr) {
        _transport->release();
        _transport = nullptr;
    }
    _foundation->release();

    _gPhysicsSDK = nullptr;
    _foundation = nullptr;

    return true;
}

void PhysX::togglePvdConnection() const {
    if (_pvd == nullptr) {
        return;
    }

    if (_pvd->isConnected()) {
        _pvd->disconnect();
    } else {
        if (_pvd->connect(*_transport, _pvdFlags)) {
            Console::d_printfn(Locale::get(_ID("CONNECT_PVD_OK")));
        }
    }
}


void PhysX::createPvdConnection(const char* ip, const physx::PxU32 port, const physx::PxU32 timeout, const bool useFullConnection) {
    //Create a pvd connection that writes data straight to the filesystem.  This is
    //the fastest connection on windows for various reasons.  First, the transport is quite fast as
    //pvd writes data in blocks and filesystems work well with that abstraction.
    //Second, you don't have the PVD application parsing data and using CPU and memory bandwidth
    //while your application is running.
    //physx::PxPvdTransport* transport = physx::PxDefaultPvdFileTransportCreate( "c:\\mywork\\sample.pxd2" );

    //The normal way to connect to pvd.  PVD needs to be running at the time this function is called.
    //We don't worry about the return value because we are already registered as a listener for connections
    //and thus our onPvdConnected call will take care of setting up our basic connection state.
    _transport = physx::PxDefaultPvdSocketTransportCreate(ip, port, timeout);
    if (_transport == nullptr) {
        return;
    }

    //The connection flags state overall what data is to be sent to PVD.  Currently
    //the Debug connection flag requires support from the implementation (don't send
    //the data when debug isn't set) but the other two flags, profile and memory
    //are taken care of by the PVD SDK.

    //Use these flags for a clean profile trace with minimal overhead
    _pvdFlags = physx::PxPvdInstrumentationFlag::eALL;
    if (!useFullConnection)
    {
        _pvdFlags = physx::PxPvdInstrumentationFlag::ePROFILE;
    }
    _pvd = physx::PxCreatePvd(*_foundation);
    //if (_pvd->connect(*_transport, _pvdFlags)) {
    //    Console::d_printfn(Locale::get(_ID("CONNECT_PVD_OK")));
    //}
}

inline void PhysX::updateTimeStep(U8 timeStepFactor, const F32 simSpeed) {
    timeStepFactor = to_U8(std::max(1u, timeStepFactor * 1u));
    _timeStepFactor = timeStepFactor;
    _timeStep = 1.0f / _timeStepFactor;
    _timeStep *= simSpeed;
}

/// Process results
void PhysX::process(const U64 deltaTimeUS) {
    if (_targetScene && _timeStep > 0.0f) {
        _accumulator +=
            Time::MicrosecondsToMilliseconds<physx::PxReal>(deltaTimeUS);

        if (_accumulator < _timeStep) {
            return;
        }

        _accumulator -= _timeStep;
        _targetScene->process(Time::SecondsToMicroseconds(_timeStep));
    }
}

/// Update actors
void PhysX::update(const U64 deltaTimeUS) {
    if (_targetScene) {
        _targetScene->update(deltaTimeUS);
    }
}

void PhysX::idle() {
    if (_targetScene) {
        _targetScene->idle();
    }
}

PhysicsSceneInterface* PhysX::NewSceneInterface(Scene& scene) {
    return MemoryManager_NEW PhysXSceneInterface(scene);
}

void PhysX::setPhysicsScene(PhysicsSceneInterface* const targetScene) {
    if (_targetScene) {
        if (targetScene) {
            if (_targetScene->getGUID() == targetScene->getGUID()) {
                return;
            }
        }
    }
    _targetScene = targetScene;
}

/*void PhysX::createActor(SceneGraphNode* node, const stringImpl& sceneName,
                        PhysicsActorMask mask, PhysicsCollisionGroup group) {
    assert(_targetScene != nullptr);

    Object3D* sNode = node.getNode<Object3D>();

    // Load cached version from file first
    stringImpl nodeName("XML/Scenes/" + sceneName + "/collisionMeshes/node_[_" + sNode->name() + "_]");
    nodeName.append(".cm");

    hashMap<stringImpl, physx::PxTriangleMesh*>::iterator it;
    it = _gMeshCache.find(nodeName);

    // -1 = not available
    //  0 = loaded from RAM cache
    //  1 = loaded from file cache
    I8 loadState = -1;
    if (it != std::cend(_gMeshCache)) {
        loadState = 0;
    } else {
        if (fileExists(nodeName) {
            loadState = 1;
        }
    }

    if (group == PhysicsCollisionGroup::GROUP_NON_COLLIDABLE) {
        // return true;
    }

    if (loadState == -1) {
        sNode->computeTriangleList();
        const vectorEASTL<vec3<U32> >& triangles = sNode->getTriangles();

        if (sNode->getTriangles().empty()) {
            return;
        }

        VertexBuffer* nodeVB = sNode->getGeometryVB();
        if (sNode->getObjectType() == ObjectType::SUBMESH) {
            SceneGraphNode* grandParent = node.getParent().lock();
            nodeVB = grandParent->getNode<Object3D>()->getGeometryVB();
        }

        physx::PxDefaultFileOutputStream stream(nodeName.c_str());

        physx::PxTriangleMeshDesc meshDesc;
        meshDesc.points.count = (physx::PxU32)nodeVB->getVertexCount();
        meshDesc.points.stride = offsetof(VertexBuffer::Vertex, _position);
        meshDesc.points.data = nodeVB->getVertices().data();
        meshDesc.triangles.count = (physx::PxU32)triangles.size();
        meshDesc.triangles.stride = 3 * sizeof(U32);
        meshDesc.triangles.data = triangles.data();
        if (!nodeVB->usesLargeIndices()) {
            // meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;
        }

        if (!_cooking->cookTriangleMesh(meshDesc, stream)) {
            Console::errorfn(Locale::get(_ID("ERROR_COOK_TRIANGLE_MESH")));
            return;
        }
    } else if (loadState == 0) {
        Console::printfn(Locale::get(_ID("COLLISION_MESH_LOADED_FROM_RAM")), nodeName.c_str());
    } else if (loadState == 1) {
        Console::printfn(Locale::get(_ID("COLLISION_MESH_LOADED_FROM_FILE")), nodeName.c_str());
    }

    PhysXSceneInterface* targetScene =  dynamic_cast<PhysXSceneInterface*>(_targetScene);
    RigidBodyComponent* nodePhysics = node.get<RigidBodyComponent>();

    PhysXActor* tempActor = targetScene->getOrCreateRigidActor(node.name(), *nodePhysics);
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

        if (mask != PhysicsActorMask::MASK_RIGID_DYNAMIC) {
            tempActor->_actor =
                _gPhysicsSDK->createRigidStatic(posePxTransform);
        } else {
            tempActor->_actor =
                _gPhysicsSDK->createRigidDynamic(posePxTransform);
            tempActor->_isDynamic = true;
        }

        if (!tempActor->_actor) {
            MemoryManager::DELETE(tempActor);
            return;
        }

        tempActor->_type = physx::PxGeometryType::eTRIANGLEMESH;
        // If we got here, the new actor was just created (didn't exist
        // previously in the scene), so add it
        targetScene->addRigidActor(tempActor, false);
    }

    assert(tempActor->_actor);

    physx::PxTriangleMesh* triangleMesh = nullptr;
    if (loadState == 0) {
        triangleMesh = it->second;
    } else {
        physx::PxDefaultFileInputData inData(nodeName.c_str());
        triangleMesh = _gPhysicsSDK->createTriangleMesh(inData);
        hashAlg::insert(_gMeshCache, std::make_pair(nodeName, triangleMesh));
    }

    if (!triangleMesh) {
        Console::errorfn(Locale::get(_ID("ERROR_CREATE_TRIANGLE_MESH")));
        return;
    }

    const vec3<F32>& scale = tempActor->getParent()->getScale();
    STUBBED("PhysX implementation only uses one shape per actor for now! -Ionut")
    tempActor->_actor->createShape(physx::PxTriangleMeshGeometry(
                                       triangleMesh,
                                        physx::PxMeshScale(
                                            physx::PxVec3(scale.x, scale.y, scale.z),
                                            physx::PxQuat(physx::PxIdentity))),
                                   *_gPhysicsSDK->createMaterial(0.7f, 0.7f, 1.0f));

    return;
};
*/

PhysicsAsset* PhysX::createRigidActor(const SceneGraphNode* node, RigidBodyComponent& parentComp)
{
    PhysXActor* newActor = new PhysXActor(parentComp);

    // get node Geometry
    // create Shape from Geometry
    // create Actor
    // attach Shape to Actor
    // register actor

    return newActor;
}

};
