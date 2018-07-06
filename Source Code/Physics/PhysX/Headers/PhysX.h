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

// PhysX includes

#include <PxPhysicsAPI.h>
// Connecting the SDK to Visual Debugger
#include <pvd/PxVisualDebugger.h>
#include <extensions/PxDefaultErrorCallback.h>
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxVisualDebuggerExt.h>
#include <foundation/PxAllocatorCallback.h>
// PhysX includes //

#include "Physics/Headers/PhysicsAPIWrapper.h"

#define MAX_ACTOR_QUEUE 30

namespace Divide {

class Transform;
class PhysX;
class PhysXSceneInterface;
class PhysXActor : public PhysicsAsset {
   public:
    PhysXActor()
        : PhysicsAsset(),
          _actor(nullptr),
          _isDynamic(false),
          _userData(0.0f)
    {
    }

    ~PhysXActor() {}

   protected:
    friend class PhysX;
    friend class PhysXSceneInterface;
    physx::PxRigidActor* _actor;
    physx::PxGeometryType::Enum _type;
    stringImpl _actorName;
    bool _isDynamic;
    F32 _userData;
};

class PxDefaultAllocator : public physx::PxAllocatorCallback {
    void* allocate(size_t size, const char*, const char*, int) {
    	return malloc_aligned(size, 16);
    }

    void deallocate(void* ptr) {
    	malloc_free(ptr);
    }
};

class SceneGraphNode;
DEFINE_SINGLETON_EXT2_W_SPECIFIER(PhysX, PhysicsAPIWrapper,
                      physx::debugger::comm::PvdConnectionHandler,
                      final)

  private:
    PhysX();
    ///////////////////////////////////////////////////////////////////////////////
    // Implements PvdConnectionFactoryHandler
    virtual void onPvdSendClassDescriptions(physx::debugger::comm::PvdConnection&) {
    }
    virtual void onPvdConnected(physx::debugger::comm::PvdConnection& inFactory) {}
    virtual void onPvdDisconnected(
        physx::debugger::comm::PvdConnection& inFactory) {}

  public:
    ErrorCode initPhysicsAPI(U8 targetFrameRate);
    bool closePhysicsAPI();
    void update(const U64 deltaTime);
    void process(const U64 deltaTime);
    void idle();

    void updateTimeStep(U8 timeStepFactor);
    inline void updateTimeStep() {
        updateTimeStep(_timeStepFactor);
    }

    PhysicsSceneInterface* NewSceneInterface(Scene* scene);

    // Default Shapes:
    void createPlane(const vec3<F32>& position = VECTOR3_ZERO, U32 size = 1);
    void createBox(const vec3<F32>& position = VECTOR3_ZERO, F32 size = 1.0f);
    void createActor(SceneGraphNode& node, const stringImpl& sceneName,
                     PhysicsActorMask mask, PhysicsCollisionGroup group);
    inline physx::PxPhysics* const getSDK() { return _gPhysicsSDK; }
    void setPhysicsScene(PhysicsSceneInterface* const targetScene);
    void initScene();

  protected:
    physx::PxProfileZone* getOrCreateProfileZone(physx::PxFoundation& inFoundation);

  protected:
    PhysicsSceneInterface* _targetScene;

  private:
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

END_SINGLETON

};  // namespace Divide

#endif
