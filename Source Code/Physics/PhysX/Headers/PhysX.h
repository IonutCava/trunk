/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef PHYSX_H_
#define PHYSX_H_

#ifndef _PHYSICS_API_FOUND_
#define _PHYSICS_API_FOUND_
#endif

#include "PhysXActor.h"
#include "Physics/Headers/PhysicsAPIWrapper.h"

#define MAX_ACTOR_QUEUE 30

namespace Divide {

class PhysX;
class PhysXSceneInterface;


class PxDefaultAllocator : public physx::PxAllocatorCallback {
    void* allocate(size_t size, const char*, const char*, int)  override {
    	return malloc_aligned(size, 16);
    }

    void deallocate(void* ptr)  override {
    	malloc_free(ptr);
    }
};

class PhysicsAsset;
class SceneGraphNode;
class PhysX final : public PhysicsAPIWrapper {

public:
    PhysX();
    ~PhysX();

public:
    ErrorCode initPhysicsAPI(U8 targetFrameRate, F32 simSpeed) final;
    bool closePhysicsAPI() final;
    void update(const U64 deltaTimeUS) final;
    void process(const U64 deltaTimeUS) final;
    void idle() final;

    void updateTimeStep(U8 timeStepFactor, F32 simSpeed) final;

    PhysicsSceneInterface* NewSceneInterface(Scene& scene) final;

    inline physx::PxPhysics* const getSDK() noexcept { return _gPhysicsSDK; }
    void setPhysicsScene(PhysicsSceneInterface* const targetScene) final;

    PhysicsAsset* createRigidActor(const SceneGraphNode* node, RigidBodyComponent& parentComp) final;


    void togglePvdConnection();
    void createPvdConnection(const char* ip, physx::PxU32 port, physx::PxU32 timeout, bool useFullConnection);

protected:
    PhysicsSceneInterface* _targetScene;

private:
    F32 _simulationSpeed;
    physx::PxPhysics* _gPhysicsSDK;
    physx::PxCooking* _cooking;
    physx::PxFoundation* _foundation;
    physx::PxReal _timeStep;
    physx::PxU8   _timeStepFactor;
    physx::PxReal _accumulator;
    physx::PxPvd*                     _pvd;
    physx::PxPvdTransport*            _transport;
    physx::PxPvdInstrumentationFlags  _pvdFlags;

    static physx::PxDefaultAllocator _gDefaultAllocatorCallback;
    static physx::PxDefaultErrorCallback _gDefaultErrorCallback;

    static hashMap<stringImpl, physx::PxTriangleMesh*> _gMeshCache;
};

};  // namespace Divide

#endif
