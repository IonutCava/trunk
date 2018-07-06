#include "Core/Math/Headers/Transform.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Dynamics/Physics/PhysX/Headers/PhysXSceneInterface.h"

using namespace physx;

bool PhysX::createPlane(const vec3<F32>& position,U32 size){
    assert(_targetScene != NULL);
    PxTransform pose = PxTransform(PxVec3(position.x,position.y,position.z),PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
    PxRigidStatic* plane = _gPhysicsSDK->createRigidStatic(pose);

    if (!plane){
        ERROR_FN(Locale::get("ERROR_PHYSX_CREATE_PLANE"));
        return false;
    }
    PxShape* shape = plane->createShape(PxPlaneGeometry(),
                                        *(static_cast<PhysXSceneInterface* >(_targetScene)->getMaterials()[1]));
    if (!shape){
        ERROR_FN(Locale::get("ERROR_PHYSX_CREATE_PLANE_SHAPE"));
        return false;
    }
    static_cast<PhysXSceneInterface* >(_targetScene)->addRigidStaticActor(plane);
    return true;
}

bool PhysX::createBox(const vec3<F32>& position, F32 size){
    assert(_targetScene != NULL);
    PxReal density = 1.0f;
    PxTransform transform(PxVec3(position.x,position.y,position.z), PxQuat::createIdentity());
    PxVec3 dimensions(size);
    PxBoxGeometry geometry(dimensions);
    PxRigidDynamic *actor = PxCreateDynamic(*_gPhysicsSDK,
                                             transform,
                                             geometry,
                                             *(static_cast<PhysXSceneInterface* >(_targetScene)->getMaterials()[1]),
                                             density);
    actor->setAngularDamping(0.75);
    actor->setLinearVelocity(PxVec3(0));

    if (!actor){
        ERROR_FN(Locale::get("ERROR_PHYSX_CREATE_BOX"));
        return false;
    }
    static_cast<PhysXSceneInterface* >(_targetScene)->addRigidDynamicActor(actor);
    return true;
}