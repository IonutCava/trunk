#include "PhysX.h"
using namespace physx;

PhysX::PhysX(){
	gPhysicsSDK = NULL;
}

bool PhysX::InitNx(){
  Console::getInstance().printfn("Initializing the PhysX engine!");
  gPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback, PxTolerancesScale() );
   if(gPhysicsSDK == NULL) {
	   Console::getInstance().errorfn("Error creating PhysX device!");
	 return false;
   }
   return true;
}

void PhysX::ExitNx(){
	if(gPhysicsSDK != NULL){
		Console::getInstance().printfn("Stopping PhysX device!");
		 gPhysicsSDK->release();
	}
}

void PhysX::idle(){
}