/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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

#include < PxPhysicsAPI.h >
#include < PxDefaultErrorCallback.h >
#include < PxDefaultAllocator.h > 
#include < PxVisualDebuggerExt.h>
//PhysX includes //
#include "core.h"
//PhysX libraries
#pragma comment(lib, "PhysX3CHECKED_x86.lib")
#pragma comment(lib, "PhysX3CommonCHECKED_x86.lib")
#pragma comment(lib, "PhysX3ExtensionsCHECKED.lib")
#pragma comment(lib, "PhysXVisualDebuggerSDKCHECKED.lib")
//PhysX libraries //

#include "Dynamics/Physics/Headers/PhysicsAPIWrapper.h"



class SceneGraphNode;
class PhysXSceneInterface;
DEFINE_SINGLETON_EXT1( PhysX,PhysicsAPIWrapper)

private:
	PhysX(); 

public:
	
   I8   initPhysics(U8 targetFrameRate);  
   bool exitPhysics(); 
   void update();
   void process();
   void idle();

   inline void updateTimeStep(U8 timeStepFactor) {
	   CLAMP<U8>(timeStepFactor,1,timeStepFactor);
	   _timeStep = 1.0f / timeStepFactor;
   }

   PhysicsSceneInterface* NewSceneInterface(Scene* scene);

  //Default Shapes:
   bool createPlane(PhysicsSceneInterface* targetScene,const vec3<F32>& position = vec3<F32>(0,0,0), U32 size = 1);
   bool createBox(PhysicsSceneInterface* targetScene,const vec3<F32>& position = vec3<F32>(0,0,0), F32 size = 1);
   bool createActor(PhysicsSceneInterface* targetScene, SceneGraphNode* node, PhysicsActorMask mask,PhysicsCollisionGroup group);
   inline void registerActiveScene(PhysXSceneInterface* activeScene) {_currentScene = activeScene;}
   inline physx::PxPhysics* const getSDK() {return _gPhysicsSDK;}
   inline const physx::PxSimulationFilterShader& getFilterShader() {return _gDefaultFilterShader;}

private:
	PhysXSceneInterface* _currentScene;
	physx::PxPhysics* _gPhysicsSDK ;
	physx::PxFoundation* _foundation;
	physx::PxDefaultErrorCallback _gDefaultErrorCallback;
	physx::PxDefaultAllocator _gDefaultAllocatorCallback;
	physx::PxSimulationFilterShader _gDefaultFilterShader;
	physx::debugger::comm::PvdConnectionManager* _pvdConnection;
	boost::mutex _physxMutex;
	physx::PxReal _timeStep;

END_SINGLETON
#endif
#endif