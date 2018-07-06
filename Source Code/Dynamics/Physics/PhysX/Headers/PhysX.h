/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef PHYSX_H_
#define PHYSX_H_
#include "config.h"

#ifdef _USE_PHYSX_API_

#ifndef _PHYSICS_API_FOUND_
#define _PHYSICS_API_FOUND_
#endif

//PhysX includes
#ifdef _PLATFORM_DEFINES_H_
#undef U8
#undef U16
#undef U32
#undef U64
#undef _PLATFORM_DEFINES_H_
#define _P_D_TYPES_ONLY_
#endif

#if defined(_MSC_VER)
#	pragma warning( push )
#		pragma warning( disable: 4503 ) //<decorated name length exceeded, name was truncated
#elif defined(__GNUC__)
#	pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wall"
#endif

#include < PxPhysicsAPI.h >
#include < PxDefaultErrorCallback.h >
#include < PxDefaultAllocator.h >
#include < PxVisualDebuggerExt.h >
#include < PxAllocatorCallback.h >
//Connecting the SDK to Visual Debugger
#include < pvd/PxVisualDebugger.h >
#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

//PhysX includes //
#include "core.h"
//PhysX libraries
#ifdef _DEBUG
#pragma comment(lib, "PhysXProfileSDKDEBUG.lib")
#pragma comment(lib, "PhysX3CookingDEBUG_x86.lib")
#pragma comment(lib, "PhysX3DEBUG_x86.lib")
#pragma comment(lib, "PhysX3CommonDEBUG_x86.lib")
#pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")
#pragma comment(lib, "PhysXVisualDebuggerSDKDEBUG.lib")
#else
#pragma comment(lib, "PhysXProfileSDKCHECKED.lib")
#pragma comment(lib, "PhysX3CookingCHECKED_x86.lib")
#pragma comment(lib, "PhysX3CHECKED_x86.lib")
#pragma comment(lib, "PhysX3CommonCHECKED_x86.lib")
#pragma comment(lib, "PhysX3ExtensionsCHECKED.lib")
#pragma comment(lib, "PhysXVisualDebuggerSDKCHECKED.lib")
#endif
//PhysX libraries //

#include "Dynamics/Physics/Headers/PhysicsAPIWrapper.h"

#define MAX_ACTOR_QUEUE 30

class Transform;
class PhysXActor {
public:
    PhysXActor() : _actor(NULL),
                   _transform(NULL),
                   _isDynamic(false),
                   _isInScene(false)
    {
    }

protected:
    friend class PhysX;
    friend class PhysXSceneInterface;
    std::string          _actorName;
    physx::PxRigidActor* _actor;
    Transform*           _transform;
    bool                 _isDynamic;
    bool                 _isInScene;

};

class PxDefaultAllocator : public physx::PxAllocatorCallback
{
    void* allocate(size_t size, const char*, const char*, int)
    {
        return _aligned_malloc(size, 16);
    }

    void deallocate(void* ptr)
    {
        _aligned_free(ptr);
    }
};

class SceneGraphNode;
class PhysXSceneInterface;
DEFINE_SINGLETON_EXT2( PhysX,PhysicsAPIWrapper,PVD::PvdConnectionHandler)

private:
    PhysX();
    ///////////////////////////////////////////////////////////////////////////////
    // Implements PvdConnectionFactoryHandler
    virtual	void onPvdSendClassDescriptions(PVD::PvdConnection&) {}
    virtual	void onPvdConnected(PVD::PvdConnection& inFactory)   {}
    virtual	void onPvdDisconnected(PVD::PvdConnection& inFactory){}

public:

   I8   initPhysics(U8 targetFrameRate);
   bool exitPhysics();
   void update(const D32 deltaTime);
   void process(const D32 deltaTime);
   void idle();

   void updateTimeStep(U8 timeStepFactor);

   PhysicsSceneInterface* NewSceneInterface(Scene* scene);

  //Default Shapes:
   bool createPlane(const vec3<F32>& position = vec3<F32>(0.0f), U32 size = 1);
   bool createBox(const vec3<F32>& position = vec3<F32>(0.0f), F32 size = 1.0f);
   bool createActor(SceneGraphNode* const node, const std::string& sceneName, PhysicsActorMask mask,PhysicsCollisionGroup group);
   inline physx::PxPhysics* const getSDK() {return _gPhysicsSDK;}
   inline void setPhysicsScene(PhysicsSceneInterface* const targetScene) {assert(targetScene); _targetScene = targetScene;}
          void initScene();
protected:
    physx::PxProfileZone* getOrCreateProfileZone(physx::PxFoundation& inFoundation); 

protected:
    PhysicsSceneInterface* _targetScene;

private:
    physx::PxPhysics*            _gPhysicsSDK ;
    physx::PxCooking*            _cooking;
    physx::PxFoundation*         _foundation;
    physx::PxProfileZoneManager* _zoneManager;         
    physx::PxProfileZone*		 _profileZone;    
    physx::debugger::comm::PvdConnectionManager* _pvdConnection;
    boost::mutex _physxMutex;
    physx::PxReal _timeStep;
    physx::PxReal _accumulator;
    static physx::PxDefaultAllocator              _gDefaultAllocatorCallback;
    static physx::PxDefaultErrorCallback          _gDefaultErrorCallback;

END_SINGLETON
#endif
#endif