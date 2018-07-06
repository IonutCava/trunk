#include "Headers/pxShapeScaling.h"

#include "Core/Math/Headers/Transform.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Dynamics/Physics/PhysX/Headers/PhysXSceneInterface.h"

using namespace physx;

namespace Divide {

bool PhysX::createPlane(const vec3<F32>& position, U32 size){
    assert(_targetScene != nullptr);

    PxRigidStatic* plane = PxCreatePlane(*_gPhysicsSDK,PxPlane(PxVec3(0,1,0),position.y),
                                         *(static_cast<PhysXSceneInterface* >(_targetScene)->getMaterials()[1]));
    /*plane->setGlobalPose(PxTransform(PxVec3(position.x,position.y,position.z), 
                                     PxQuat(RADIANS(90), PxVec3(1,0,0))));*/

    if (!plane){
        ERROR_FN(Locale::get("ERROR_PHYSX_CREATE_PLANE"));
        return false;
    }

    PhysXActor* actorWrapper = New PhysXActor();
    actorWrapper->_type = PxGeometryType::ePLANE;
    actorWrapper->_actor = plane;
    actorWrapper->_isDynamic = false;
    actorWrapper->_userData = (F32)size;
    static_cast<PhysXSceneInterface* >(_targetScene)->addRigidActor(actorWrapper);
    return true;
}

bool PhysX::createBox(const vec3<F32>& position, F32 size){
    assert(_targetScene != nullptr);
    PxReal density = 1.0f;
    PxTransform transform(PxVec3(position.x, position.y, position.z), PxQuat(PxIdentity));
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

    PhysXActor* actorWrapper = New PhysXActor();
    actorWrapper->_type = PxGeometryType::eBOX;
    actorWrapper->_actor = actor;
    actorWrapper->_isDynamic = true;
    actorWrapper->_userData = size * 2;
    static_cast<PhysXSceneInterface* >(_targetScene)->addRigidActor(actorWrapper);
    return true;
}

};