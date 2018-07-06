#include "Headers/PhysX.h"
#include "Headers/PhysXSceneInterface.h"
#include "Graphs/Headers/SceneGraphNode.h"

using namespace physx;

PhysX::PhysX() : _currentScene(NULL), _gPhysicsSDK(NULL){}

bool PhysX::initPhysics(){
  _gDefaultFilterShader=physx::PxDefaultSimulationFilterShader;
  Console::getInstance().printfn("Initializing the PhysX engine!");
  _gPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, _gDefaultAllocatorCallback, _gDefaultErrorCallback, PxTolerancesScale() );
   if(_gPhysicsSDK == NULL) {
	   Console::getInstance().errorfn("Error creating PhysX device!");
	 return false;
   }
   if(!PxInitExtensions(*_gPhysicsSDK)){
	   Console::getInstance().errorfn("PhysX: PxInitExtensions failed!");
	   return false;
   }
  Console::getInstance().printfn("PhysX engine initialized!");
#ifdef _DEBUG
  PxExtensionVisualDebugger::connect(_gPhysicsSDK->getPvdConnectionManager(),"localhost",5425, 10000, true);
#endif
   return true;
}

bool PhysX::exitPhysics(){
	if(_gPhysicsSDK != NULL){
		Console::getInstance().printfn("Stopping PhysX device!");
		 PxCloseExtensions();
       _gPhysicsSDK->release();
	}else{
		return false;
	}
	return true;
}

//Process results
void PhysX::process(){
	if(_currentScene){
		_currentScene->process(); 
	}
}

//Update actors
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