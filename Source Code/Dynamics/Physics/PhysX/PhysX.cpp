#include "Headers/PhysX.h"
#include "Headers/PhysXSceneInterface.h"
#include "Graphs/Headers/SceneGraphNode.h"

using namespace physx;

PhysX::PhysX() : _currentScene(NULL), _gPhysicsSDK(NULL), _foundation(NULL), _pvdConnection(NULL){}

I8 PhysX::initPhysics(){
  _gDefaultFilterShader=physx::PxDefaultSimulationFilterShader;
  PRINT_FN(Locale::get("START_PHYSX_API"));
  // create foundation object with default error and allocator callbacks.
  _foundation = PxCreateFoundation(PX_PHYSICS_VERSION,_gDefaultAllocatorCallback,_gDefaultErrorCallback);
  _gPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation , PxTolerancesScale() );
   if(_gPhysicsSDK == NULL) {
	   ERROR_FN(Locale::get("ERROR_START_PHYSX_API"));
	   return PHYSX_INIT_ERROR;
   }
   if(!PxInitExtensions(*_gPhysicsSDK)){
	   ERROR_FN(Locale::get("ERROR_EXTENSION_PHYSX_API"));
	   return PHYSX_EXTENSION_ERROR;
   }
   PRINT_FN(Locale::get("START_PHYSX_API_OK"));
#ifdef _DEBUG
   _pvdConnection = _gPhysicsSDK->getPvdConnectionManager();
   if(_pvdConnection != NULL){
	   PxVisualDebuggerExt::createConnection(_pvdConnection,"localhost",5425, 10000);
   }
#endif
   return NO_ERR;
}

bool PhysX::exitPhysics(){
	if(_gPhysicsSDK != NULL){
		PRINT_FN(Locale::get("STOP_PHYSX_API"));
		 PxCloseExtensions();
       _gPhysicsSDK->release();
	   _foundation->release();
	}else{
		return false;
	}
	return true;
}

///Process results
void PhysX::process(){
	if(_currentScene){
		_currentScene->process(); 
	}
}

///Update actors
void PhysX::update(){
	if(_currentScene){
		_currentScene->update();   
	}
}

void PhysX::idle(){
}

PhysicsSceneInterface* PhysX::NewSceneInterface(Scene* scene) {
	return New PhysXSceneInterface(scene);
}

bool PhysX::createActor(PhysicsSceneInterface* targetScene, SceneGraphNode* node, PhysicsActorMask mask,PhysicsCollisionGroup group){
	assert(node != NULL);
	assert(targetScene != NULL);
	PxTransform transform(PxVec3(node->getTransform()->getPosition().x,
								 node->getTransform()->getPosition().y,
								 node->getTransform()->getPosition().z),
						  PxQuat(node->getTransform()->getOrientation().getX(),
						    	 node->getTransform()->getOrientation().getY(),
								 node->getTransform()->getOrientation().getZ(),
								 node->getTransform()->getOrientation().getW()));
	switch(mask){
		default:
		case MASK_RIGID_STATIC:
			dynamic_cast<PhysXSceneInterface* >(targetScene)->addRigidStaticActor(NULL);
			break;
		case MASK_RIGID_DYNAMIC:
			dynamic_cast<PhysXSceneInterface* >(targetScene)->addRigidDynamicActor(NULL);
			break;
	};
	return true;
};