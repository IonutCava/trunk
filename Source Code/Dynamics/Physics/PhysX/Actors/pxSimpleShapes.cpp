#include "Core/Math/Headers/Transform.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Dynamics/Physics/PhysX/Headers/PhysXSceneInterface.h"

using namespace physx;

bool PhysX::createPlane(PhysicsSceneInterface* targetScene,const vec3<F32>& position,U32 size){
	assert(targetScene != NULL);
	PxTransform pose = PxTransform(PxVec3(position.x,position.y,position.z),PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
	PxRigidStatic* plane = _gPhysicsSDK->createRigidStatic(pose);
	if (!plane){
		ERROR_FN("PhysX: error creating plane!");
		return false;
	}
	PxShape* shape = plane->createShape(PxPlaneGeometry(), *(_gPhysicsSDK->createMaterial(0.5,0.5,0.5)));
	if (!shape){
		ERROR_FN("PhysX: error creating shape for plane!");
		return false;
	}
	static_cast<PhysXSceneInterface* >(targetScene)->addRigidStaticActor(plane);
	return true;
}

bool PhysX::createBox(PhysicsSceneInterface* targetScene,const vec3<F32>& position, F32 size){
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
		ERROR_FN("PhysX: error creating box!");
		return false;
	}
	static_cast<PhysXSceneInterface* >(targetScene)->addRigidDynamicActor(actor);
	return true;
}