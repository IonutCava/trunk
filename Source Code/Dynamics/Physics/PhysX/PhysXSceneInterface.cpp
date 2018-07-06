#include "Headers/PhysXSceneInterface.h"

#include "Scenes/Headers/Scene.h"
#include "Graphs/Headers/SceneNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

using namespace physx;

bool PhysXSceneInterface::init(){
    physx::PxPhysics* gPhysicsSDK = PhysX::getInstance().getSDK();
    //Create the scene
    if(!gPhysicsSDK){
        ERROR_FN(Locale::get("ERROR_PHYSX_SDK"));
        return false;
    }

    PxSceneDesc sceneDesc(gPhysicsSDK->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);
    if(!sceneDesc.cpuDispatcher) {
        _cpuDispatcher = PxDefaultCpuDispatcherCreate(3);
        if(!_cpuDispatcher){
            ERROR_FN(Locale::get("ERROR_PHYSX_INTERFACE_CPU_DISPATCH"));
        }
         sceneDesc.cpuDispatcher = _cpuDispatcher;
    }

    if(!sceneDesc.filterShader)
        sceneDesc.filterShader	= PxDefaultSimulationFilterShader;

    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

    _gScene = gPhysicsSDK->createScene(sceneDesc);
    if (!_gScene){
        ERROR_FN(Locale::get("ERROR_PHYSX_INTERFACE_CREATE_SCENE"));
        return false;
    }

    _gScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0);
    _gScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
    PxMaterial* planeMaterial = gPhysicsSDK->createMaterial(0.9f, 0.1f, 1.0f);
    _materials.push_back(planeMaterial);
    PxMaterial* cubeMaterial = gPhysicsSDK->createMaterial(0.1f, 0.4f, 1.0f);
    _materials.push_back(cubeMaterial);
    PxMaterial* sphereMaterial = gPhysicsSDK->createMaterial(0.6f, 0.1f, 0.6f);
    _materials.push_back(sphereMaterial);
    _addedPhysXPlane = false;
    _rigidCount = 0;
    return true;
}

bool PhysXSceneInterface::exit(){
    D_PRINT_FN(Locale::get("STOP_PHYSX_SCENE_INTERFACE"));
    return true;
}

void PhysXSceneInterface::idle(){
    if(_rigidCount == 0)
        return;

    U32 tempCounter = _rigidCount;

    WriteLock w_lock(_queueLock);
    for(U32 i = 0; i < tempCounter; i++){
        _sceneRigidActors.push_back(_sceneRigidQueue[i]);
    }
    _sceneRigidQueue.clear();
    w_lock.unlock();

    _rigidCount = 0;
}

void PhysXSceneInterface::release(){
    if(!_gScene){
        ERROR_FN(Locale::get("ERROR_PHYSX_CLOSE_INVALID_INTERFACE"));
        return;
    }

    for(RigidMap::iterator it = _sceneRigidActors.begin();
                           it != _sceneRigidActors.end();
                           it++){
        (*it)->_actor->release();
        SAFE_DELETE(*it);
     }

    _sceneRigidActors.clear();

    //if(_cpuDispatcher)
    //   _cpuDispatcher->release();

    _gScene->release();
}

void PhysXSceneInterface::update(const D32 deltaTime){
    if(!_gScene) 
        return;

    for(RigidMap::iterator it = _sceneRigidActors.begin();
                           it != _sceneRigidActors.end();
                           it++){
        PhysXActor* crtActor = *it;
        if(!crtActor->_isInScene)
            addToScene(*crtActor);
        updateActor(*crtActor);
    }
}

#pragma("ToDo: Add a better synchronization method between SGN's transform and PhysXActor's pose!! -Ionut")
void PhysXSceneInterface::updateActor(const PhysXActor& actor){
    if(actor._transform->isPhysicsDirty()){
        const vec3<F32>& position = actor._transform->getPosition();
        const vec4<F32>& orientation = actor._transform->getGlobalOrientation().asVec4();

        physx::PxTransform posePxTransform(PxVec3(position.x, position.y, position.z),
                                           PxQuat(orientation.x,orientation.y,orientation.z,orientation.w).getConjugate());
        actor._actor->setGlobalPose(posePxTransform);
        actor._transform->cleanPhysics();
        return;
    }

    PxU32 nShapes = actor._actor->getNbShapes();
    PxShape** shapes=New PxShape*[nShapes];
    actor._actor->getShapes(shapes, nShapes);
    while (nShapes--){
        updateShape(shapes[nShapes], actor._transform);
    }
    SAFE_DELETE_ARRAY(shapes);
}

void PhysXSceneInterface::updateShape(PxShape* const shape, Transform* const t){
    if(!t || !shape)
        return;

    PxTransform pT = PxShapeExt::getGlobalPose(*shape);
    PxQuat pQ = pT.q.getConjugate();
    if(shape->getGeometryType() == PxGeometryType::ePLANE){
        t->scale(shape->getActor().getObjectSize());
    }else{
        if(shape->getGeometryType() == PxGeometryType::eTRIANGLEMESH){
            PxTriangleMeshGeometry triangleGeom;
            shape->getTriangleMeshGeometry(triangleGeom);
        }
    }

    t->rotateQuaternion(Quaternion<F32>(pQ.x,pQ.y,pQ.z,pQ.w));
    t->setPosition(vec3<F32>(pT.p.x,pT.p.y,pT.p.z));
}

void PhysXSceneInterface::process(const D32 deltaTime){
    if(!_gScene)
        return;

    _gScene->simulate((physx::PxReal)deltaTime);

    while(!_gScene->fetchResults()){
        idle();
    }
    
}

void PhysXSceneInterface::addRigidActor(PhysXActor* const actor, bool threaded) {
    if(!actor)
        return;
    if(threaded){
        WriteLock w_lock(_queueLock);
        _sceneRigidQueue.push_back(actor);
        w_lock.unlock();
        _rigidCount++;
    }else{
        _sceneRigidActors.push_back(actor);
    }
}

PhysXActor* PhysXSceneInterface::getOrCreateRigidActor(const std::string& actorName) {
    if(!_gScene)
        return NULL;

    for(RigidMap::iterator it = _sceneRigidActors.begin();
                           it != _sceneRigidActors.end();
                           it++){
        if((*it)->_actorName.compare(actorName) == 0)
            return (*it);
    }

    PhysXActor* newActor = New  PhysXActor();
    newActor->_actorName = actorName;
    return newActor;
}

#pragma message("ToDo: Only 1 shape per actor for now. Fix This! -Ionut")
SceneGraphNode* PhysXSceneInterface::addToScene(PhysXActor& actor){

    SceneNode* sceneNode = NULL;
    SceneGraphNode* tempNode = NULL;

    _gScene->addActor(*(actor._actor));
    U32 nbActors = _gScene->getNbActors(PxActorTypeSelectionFlag::eRIGID_DYNAMIC |
                                        PxActorTypeSelectionFlag::eRIGID_STATIC);
    PxShape** shapes=New PxShape*[actor._actor->getNbShapes()];
    actor._actor->getShapes(shapes, actor._actor->getNbShapes());

    std::string sgnName = "";
    PxGeometryType::Enum geometryType = shapes[0]->getGeometryType();
    switch(geometryType){
        case PxGeometryType::eBOX: {
            PxBoxGeometry box;
            shapes[0]->getBoxGeometry(box);
            std::string meshName = "BoxActor_";
            meshName.append(Util::toString(box.halfExtents.x * 2));
            sgnName = meshName + "_node_";
            sgnName.append(Util::toString(nbActors));
            actor._actorName = sgnName;
            ResourceDescriptor boxDescriptor(meshName);
            boxDescriptor.setPropertyList(Util::toString(box.halfExtents.x * 2));
            sceneNode = CreateResource<Box3D>(boxDescriptor);

            if(sceneNode->getRefCount() == 1){
                Material* boxMaterial = CreateResource<Material>(ResourceDescriptor(meshName+"_material"));
                boxMaterial->setDiffuse(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
                boxMaterial->setAmbient(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
                boxMaterial->setEmissive(vec4<F32>(0.1f,0.1f,0.1f,1.0f));
                boxMaterial->setShininess(2);
                sceneNode->setMaterial(boxMaterial);
            }
        }
        break;

        case PxGeometryType::ePLANE: {
            if(!_addedPhysXPlane){
                sgnName = "PlaneActor_node_";
                sgnName.append(Util::toString(nbActors));
                ResourceDescriptor planeDescriptor("PlaneActor");
                planeDescriptor.setFlag(true);
                sceneNode = CreateResource<Quad3D>(planeDescriptor);
                if(sceneNode->getRefCount() == 1){
                    Material* planeMaterial = CreateResource<Material>(ResourceDescriptor("planeMaterial"));
                    planeMaterial->setDiffuse(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
                    planeMaterial->setAmbient(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
                    planeMaterial->setEmissive(vec4<F32>(0.3f,0.3f,0.3f,1.0f));
                    planeMaterial->setShininess(0);
                    planeMaterial->setCastsShadows(false);
                    sceneNode->setMaterial(planeMaterial);
                }
                actor._actorName = sgnName;
                _addedPhysXPlane = true;
           }
        }break;
    }

    SAFE_DELETE_ARRAY(shapes);
    if(geometryType != PxGeometryType::eTRIANGLEMESH) {
        if(sceneNode){
            tempNode = _parentScene->addGeometry(sceneNode,sgnName);
        }
        if(!tempNode){
            ERROR_FN(Locale::get("ERROR_ACTOR_CAST"),sgnName.c_str());
        }else{
            actor._transform = tempNode->getTransform();
        }
    }

    actor._isInScene = true;
    return tempNode;
}