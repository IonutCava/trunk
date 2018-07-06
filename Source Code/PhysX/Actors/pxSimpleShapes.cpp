#include "PhysX/Headers/PhysX.h"
#include "PhysX/Headers/PhysXSceneInterface.h"
#include "Core/Math/Headers/Transform.h"

using namespace physx;

bool PhysX::createPlane(PhysXSceneInterface* targetScene,const vec3& position,U32 size){
	PxTransform pose = PxTransform(PxVec3(position.x,position.y,position.z),PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
	PxRigidStatic* plane = _gPhysicsSDK->createRigidStatic(pose);
	if (!plane){
		Console::getInstance().errorfn("PhysX: error creating plane!");
		return false;
	}
	PxShape* shape = plane->createShape(PxPlaneGeometry(), *(_gPhysicsSDK->createMaterial(0.5,0.5,0.5)));
	if (!shape){
		Console::getInstance().errorfn("PhysX: error creating shape for plane!");
		return false;
	}
	targetScene->addRigidStaticActor(plane);
	return true;
}

bool PhysX::createBox(PhysXSceneInterface* targetScene,const vec3& position, F32 size){
	PxReal density = 1.0f;
	PxTransform transform(PxVec3(position.x,position.y,position.z), PxQuat::createIdentity());
	PxVec3 dimensions(size ,size ,size );
	PxBoxGeometry geometry(dimensions);
	PxRigidDynamic *actor = PxCreateDynamic(*_gPhysicsSDK,
											 transform, 
											 geometry,
											 *(_gPhysicsSDK->createMaterial(0.5,0.5,0.5)),
											 density);
    actor->setAngularDamping(0.75);
    actor->setLinearVelocity(PxVec3(0,0,0)); 
	if (!actor){
		Console::getInstance().errorfn("PhysX: error creating box!");
		return false;
	}
	targetScene->addRigidDynamicActor(actor);
	return true;
}