/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef PHYSX_H_
#define PHYSX_H_

#ifndef _PHYSICS_API_FOUND_
#define _PHYSICS_API_FOUND_
#endif

#include "PhysXActor.h"
#include "Physics/Headers/PhysicsAPIWrapper.h"

#define MAX_ACTOR_QUEUE 30

namespace Divide {

class Transform;
class PhysX;
class PhysXSceneInterface;


class PxDefaultAllocator : public physx::PxAllocatorCallback {
    void* allocate(size_t size, const char*, const char*, int) {
    	return malloc_aligned(size, 16);
    }

    void deallocate(void* ptr) {
    	malloc_free(ptr);
    }
};

class PhysicsAsset;
class SceneGraphNode;
class PhysX final : public PhysicsAPIWrapper,
                    private physx::debugger::comm::PvdConnectionHandler {

public:
    PhysX();
    ~PhysX();
private:
    ///////////////////////////////////////////////////////////////////////////////
    // Implements PvdConnectionFactoryHandler
    virtual void onPvdSendClassDescriptions(physx::debugger::comm::PvdConnection&) {
    }
    virtual void onPvdConnected(physx::debugger::comm::PvdConnection& inFactory) {}
    virtual void onPvdDisconnected(
        physx::debugger::comm::PvdConnection& inFactory) {}

public:
    ErrorCode initPhysicsAPI(U8 targetFrameRate, F32 simSpeed)  override;
    bool closePhysicsAPI()  override;
    void update(const U64 deltaTime)  override;
    void process(const U64 deltaTime)  override;
    void idle()  override;

    void updateTimeStep(U8 timeStepFactor, F32 simSpeed)  override;

    PhysicsSceneInterface* NewSceneInterface(Scene& scene)  override;

    inline physx::PxPhysics* const getSDK() { return _gPhysicsSDK; }
    void setPhysicsScene(PhysicsSceneInterface* const targetScene);

    PhysicsAsset* createRigidActor(const SceneGraphNode& node) override;
protected:
    physx::PxProfileZone* getOrCreateProfileZone(physx::PxFoundation& inFoundation);

protected:
    PhysicsSceneInterface* _targetScene;

private:
    F32 _simulationSpeed;
    physx::PxPhysics* _gPhysicsSDK;
    physx::PxCooking* _cooking;
    physx::PxFoundation* _foundation;
    physx::PxProfileZoneManager* _zoneManager;
    physx::PxProfileZone* _profileZone;
    physx::debugger::comm::PvdConnectionManager* _pvdConnection;
    physx::PxReal _timeStep;
    physx::PxU8   _timeStepFactor;
    physx::PxReal _accumulator;
    static physx::PxDefaultAllocator _gDefaultAllocatorCallback;
    static physx::PxDefaultErrorCallback _gDefaultErrorCallback;

    static hashMapImpl<stringImpl, physx::PxTriangleMesh*> _gMeshCache;
};

};  // namespace Divide

#endif
