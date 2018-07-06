#include "Headers/PhysX.h"
#include "Headers/PhysXSceneInterface.h"

using namespace physx;

PhysX::PhysX() : _currentScene(NULL), _gPhysicsSDK(NULL){}

bool PhysX::initNx(){
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

bool PhysX::exitNx(){
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