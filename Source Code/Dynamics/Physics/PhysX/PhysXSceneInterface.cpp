#include "Headers/PhysXSceneInterface.h"

#include "Scenes/Headers/Scene.h"
#include "Graphs/Headers/SceneNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

using namespace physx;

enum PhysXSceneInterfaceState {
    STATE_LOADING_ACTORS
};

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
    for(U32 i = 0; i < tempCounter; ++i){
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

    for(RigidMap::iterator it = _sceneRigidActors.begin(); it != _sceneRigidActors.end(); ++it){
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

    for(RigidMap::iterator it = _sceneRigidActors.begin(); it != _sceneRigidActors.end(); ++it){
        PhysXActor& crtActor = *(*it);

        if(!crtActor._isInScene)
            addToScene(crtActor);
            
        updateActor(crtActor);
    }
}

#pragma message("ToDo: Add a better synchronization method between SGN's transform and PhysXActor's pose!! -Ionut")

void PhysXSceneInterface::updateActor(const PhysXActor& actor){
    if(actor._transform->isPhysicsDirty()){

        const vec3<F32>& position = actor._transform->getPosition();
        const vec4<F32>& orientation = actor._transform->getGlobalOrientation().asVec4();

        physx::PxTransform posePxTransform(PxVec3(position.x, position.y, position.z),
                                           PxQuat(orientation.x,orientation.y,orientation.z,orientation.w).getConjugate());
        actor._actor->setGlobalPose(posePxTransform);
        actor._transform->cleanPhysics();

    }else{

        PxU32 nShapes = actor._actor->getNbShapes();
        PxShape** shapes=New PxShape*[nShapes];
        actor._actor->getShapes(shapes, nShapes);

        while(nShapes--)  
            updateShape(shapes[nShapes], actor);
        
        SAFE_DELETE_ARRAY(shapes);
    }
}

void PhysXSceneInterface::updateShape(PxShape* const shape, const PhysXActor& actor){
    Transform* t = actor._transform;

    if(!t || !shape)
        return;

    PxTransform pT = PxShapeExt::getGlobalPose(*shape);
    PxQuat pQ = pT.q.getConjugate();

    if(actor._type == physx::PxGeometryType::ePLANE)
        std::swap(pQ.x,pQ.z);
       
    t->rotateQuaternion(Quaternion<F32>(pQ.x,pQ.y,pQ.z,pQ.w));
    t->setPosition(vec3<F32>(pT.p.x,pT.p.y,pT.p.z));
    // do not flag dirty for physics
    actor._transform->cleanPhysics();
}

void PhysXSceneInterface::process(const D32 deltaTime){
    if(!_gScene)
        return;

    _gScene->simulate((physx::PxReal)deltaTime);

    while(!_gScene->fetchResults()) idle();
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

    for(RigidMap::iterator it = _sceneRigidActors.begin(); it != _sceneRigidActors.end(); ++it){
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

    switch(actor._type){
        case PxGeometryType::eBOX: {
            ResourceDescriptor boxDescriptor("BoxActor");
            sceneNode = CreateResource<Box3D>(boxDescriptor);

            sgnName = "BoxActor_" + Util::toString(nbActors);

            if(sceneNode->getRefCount() == 1){
                Material* boxMaterial = CreateResource<Material>(ResourceDescriptor("BoxActor_material"));
                boxMaterial->setDiffuse(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
                boxMaterial->setAmbient(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
                boxMaterial->setEmissive(vec4<F32>(0.1f,0.1f,0.1f,1.0f));
                boxMaterial->setShininess(2);
                sceneNode->setMaterial(boxMaterial);
            }
        }
        break;

        case PxGeometryType::ePLANE: {
            sgnName = "PlaneActor";
            if(FindResource<Quad3D>(sgnName)) 
                return GET_ACTIVE_SCENEGRAPH()->findNode(sgnName);

            ResourceDescriptor planeDescriptor(sgnName);
            planeDescriptor.setFlag(true);

            sceneNode = CreateResource<Quad3D>(planeDescriptor);
            Material* planeMaterial = CreateResource<Material>(ResourceDescriptor(sgnName + "_material"));
            planeMaterial->setDiffuse(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
            planeMaterial->setAmbient(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
            planeMaterial->setEmissive(vec4<F32>(0.3f,0.3f,0.3f,1.0f));
            planeMaterial->setShininess(0);
            planeMaterial->setCastsShadows(false);
            sceneNode->setMaterial(planeMaterial);
        }break;
    }

    SAFE_DELETE_ARRAY(shapes);

    if(actor._type != PxGeometryType::eTRIANGLEMESH) {
        if(sceneNode){
            sceneNode->getSceneNodeRenderState().setDrawState(true);
            tempNode = _parentScene->addGeometry(sceneNode,sgnName);
        }
        if(!tempNode){
            ERROR_FN(Locale::get("ERROR_ACTOR_CAST"), sgnName.c_str());
        }else{
            actor._actorName = sgnName;
            actor._transform = tempNode->getTransform();
            actor._transform->scale(actor._userData);
            actor._transform->cleanPhysics();
        }
    }

    actor._isInScene = true;
    return tempNode;
}