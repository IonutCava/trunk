#include "Headers/PhysXSceneInterface.h"
#include "Scenes/Headers/Scene.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Graphs/Headers/SceneNode.h"

using namespace physx;

bool PhysXSceneInterface::init(){
	//Create the scene
	if(!PhysX::getInstance().getSDK()){
		ERROR_FN(Locale::get("ERROR_PHYSX_SDK"));
		return false;
	}
	PxSceneDesc sceneDesc(PhysX::getInstance().getSDK()->getTolerancesScale());
	sceneDesc.gravity=PxVec3(0.0f, -9.8f, 0.0f);
	if(!sceneDesc.cpuDispatcher) {
		PxDefaultCpuDispatcher* mCpuDispatcher = PxDefaultCpuDispatcherCreate(1);
		if(!mCpuDispatcher){
			ERROR_FN(Locale::get("ERROR_PHYSX_INTERFACE_CPU_DISPATCH"));
		}
		 sceneDesc.cpuDispatcher = mCpuDispatcher;
	}
	if(!sceneDesc.filterShader){
		sceneDesc.filterShader  = PhysX::getInstance().getFilterShader();
	}
    physx::PxPhysics* gPhysicsSDK = PhysX::getInstance().getSDK();
	_gScene = gPhysicsSDK->createScene(sceneDesc);
	if (!_gScene){
		ERROR_FN(Locale::get("ERROR_PHYSX_INTERFACE_CREATE_SCENE"));
		return false;
	}

	_gScene->setVisualizationParameter(PxVisualizationParameter::eSCALE,     1.0);
	_gScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
    PxMaterial* planeMaterial = gPhysicsSDK->createMaterial(0.9f, 0.1f, 1.0f);
    _materials.push_back(planeMaterial);
    PxMaterial* cubeMaterial = gPhysicsSDK->createMaterial(0.1f, 0.4f, 1.0f);
    _materials.push_back(cubeMaterial);
    PxMaterial* sphereMaterial = gPhysicsSDK->createMaterial(0.6f, 0.1f, 0.6f);
    _materials.push_back(sphereMaterial);
    _addedPhysXPlane = false;
    _rigidStaticCount = 0;
    _rigidDynamicCount = 0;
	return true;
}

bool PhysXSceneInterface::exit(){
	D_PRINT_FN(Locale::get("STOP_PHYSX_SCENE_INTERFACE"));
	return true;
}

void PhysXSceneInterface::idle(){
    UpgradableReadLock ur_lock(_queueLock);
    I32 tempDynamic = _rigidDynamicCount;
    for(I32 i = 0; i < tempDynamic; i++){
        PxRigidDynamic* actor = _sceneRigidDynamicQueue[i];
        _sceneRigidDynamicActors.push_back(actor);
        addToScene(actor);
    }

    I32 tempStatic = _rigidStaticCount;
    for(I32 i = 0; i < tempStatic; i++){
        PxRigidStatic* actor = _sceneRigidStaticQueue[i];
        _sceneRigidStaticActors.push_back(actor);
        addToScene(actor);
    }

    UpgradeToWriteLock uw_lock(ur_lock);
    _sceneRigidStaticQueue.clear();
    _sceneRigidDynamicQueue.clear();

    _rigidStaticCount = 0;
    _rigidDynamicCount = 0;
}

void PhysXSceneInterface::release(){
	if(!_gScene){
		ERROR_FN(Locale::get("ERROR_PHYSX_CLOSE_INVALID_INTERFACE"));
		return;
	}
	_gScene->release();
}

void PhysXSceneInterface::update(){
	if(!_gScene) return;
    for(RigidDynamicMap::iterator it = _sceneRigidDynamicActors.begin(); 
                                  it != _sceneRigidDynamicActors.end();
                                  it++){
		updateActor(*it);
	}

    for(RigidStaticMap::iterator it =  _sceneRigidStaticActors.begin();
                                 it != _sceneRigidStaticActors.end(); 
                                 it++){
		updateActor(*it);
	}
}

void PhysXSceneInterface::updateActor(PxRigidActor* const actor){
   PxU32 nShapes = actor->getNbShapes(); 
   PxShape** shapes=New PxShape*[nShapes];
   actor->getShapes(shapes, nShapes);
   while (nShapes--){ 
       updateShape(shapes[nShapes], static_cast<Transform*>(actor->userData)); 
   } 
   SAFE_DELETE_ARRAY(shapes);
}

#pragma message("ToDo: Remove hack! Find out why plane isn't rotating - Ionut")
void PhysXSceneInterface::updateShape(PxShape* const shape, Transform* const t){
	if(!t || !shape) return;
	PxTransform pT = PxShapeExt::getGlobalPose(*shape);
	if(shape->getGeometryType() == PxGeometryType::ePLANE){
		t->scale(shape->getActor().getObjectSize());
		t->rotate(vec3<F32>(1,0,0),90);
	}else{
		t->rotateQuaternion(Quaternion<F32>(pT.q.x,pT.q.y,pT.q.z,pT.q.w));
	}
	t->setPosition(vec3<F32>(pT.p.x,pT.p.y,pT.p.z));
}

void PhysXSceneInterface::process(F32 timeStep){
	if(!_gScene) return;
	_gScene->simulate((physx::PxReal)timeStep);  
    while(!_gScene->fetchResults()){
        idle();   
    }
}

void PhysXSceneInterface::addRigidStaticActor(physx::PxRigidStatic* const actor) {
    WriteLock w_lock(_queueLock);
    _sceneRigidStaticQueue.push_back(actor);
    w_lock.unlock();
    _rigidStaticCount++;
}

void PhysXSceneInterface::addRigidDynamicActor(physx::PxRigidDynamic* const actor) {
    WriteLock w_lock(_queueLock);
   _sceneRigidDynamicQueue.push_back(actor);
   w_lock.unlock();
   _rigidDynamicCount++;
}


#pragma message("ToDo: Only 1 shape per actor for now. Fix This! -Ionut")
SceneGraphNode* PhysXSceneInterface::addToScene(PxRigidActor* const actor){
    if(!actor){
         return NULL;
    }
    SceneNode* sceneNode = NULL;
    SceneGraphNode* tempNode = NULL;

    _gScene->addActor(*actor);
	U32 nbActors = _gScene->getNbActors(PxActorTypeSelectionFlag::eRIGID_DYNAMIC | 
                                        PxActorTypeSelectionFlag::eRIGID_STATIC);
	PxShape** shapes=New PxShape*[actor->getNbShapes()];
	actor->getShapes(shapes, actor->getNbShapes());   

    std::string sgnName = "";
	switch(shapes[0]->getGeometryType()){
		 case PxGeometryType::eBOX: {
            PxBoxGeometry box;
			shapes[0]->getBoxGeometry(box);
            std::string meshName = "BoxActor_";
            meshName.append(Util::toString(box.halfExtents.x * 2));
            sgnName = meshName + "_node_";
            sgnName.append(Util::toString(nbActors));

            ResourceDescriptor boxDescriptor(meshName);
            boxDescriptor.setPropertyList(Util::toString(box.halfExtents.x * 2));
			sceneNode = CreateResource<Box3D>(boxDescriptor);
            Material* boxMaterial = CreateResource<Material>(ResourceDescriptor(meshName+"_material"));
            if(sceneNode->getRefCount() == 1){
			    boxMaterial->setDiffuse(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
			    boxMaterial->setAmbient(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
                boxMaterial->setEmissive(vec4<F32>(0.1f,0.1f,0.1f,1.0f));
                boxMaterial->setShininess(2);
            }
            sceneNode->setMaterial(boxMaterial);
	   }
       break;
		 case PxGeometryType::ePLANE: {
		   if(!_addedPhysXPlane){
                sgnName = "PlaneActor_node_";
                sgnName.append(Util::toString(nbActors));
				Material* planeMaterial = CreateResource<Material>(ResourceDescriptor("planeMaterial"));
				ResourceDescriptor planeDescriptor("PlaneActor");
				planeDescriptor.setFlag(true);
				sceneNode = CreateResource<Quad3D>(planeDescriptor);
                if(planeMaterial->getRefCount() == 1){
				    planeMaterial->setDiffuse(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
				    planeMaterial->setAmbient(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
                    planeMaterial->setEmissive(vec4<F32>(0.3f,0.3f,0.3f,1.0f));
                    planeMaterial->setShininess(0);
                    planeMaterial->setCastsShadows(false);
                }
				sceneNode->setMaterial(planeMaterial);
				_addedPhysXPlane = true;
		   }
		}break;
	} 
    SAFE_DELETE_ARRAY(shapes);

	if(sceneNode){
		tempNode = _parentScene->addGeometry(sceneNode,sgnName);
        actor->userData = static_cast<void* >(tempNode->getTransform());
	}
    if(!tempNode){
        ERROR_FN(Locale::get("ERROR_ACTOR_CAST"),sgnName.c_str());
    }
    return tempNode;
}
