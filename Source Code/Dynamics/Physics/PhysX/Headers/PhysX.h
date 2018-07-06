/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#include "resource.h"

//PhysX includes
#include < PxPhysicsAPI.h> 
#include < PxExtensionsAPI.h>
#include < PxDefaultErrorCallback.h>
#include < PxDefaultAllocator.h>
#include < PxDefaultSimulationFilterShader.h>
#include < PxDefaultCpuDispatcher.h>
#include < PxShapeExt.h>
#include < PxMat33Legacy.h> 
#include < PxSimpleFactory.h>
//PhysX includes //

#include "Dynamics/Physics/Headers/PhysicsAPIWrapper.h"

//PhysX libraries
#pragma comment(lib, "PhysX3_x86.lib")
#pragma comment(lib, "PxTask.lib")
#pragma comment(lib, "Foundation.lib")
#pragma comment(lib, "PhysX3Extensions.lib")
#pragma comment(lib, "GeomUtils.lib") 
//PhysX libraries //

class SceneGraphNode;
class PhysXSceneInterface;
DEFINE_SINGLETON_EXT1( PhysX,PhysicsAPIWrapper)

private:
	PhysX(); 

public:
	
   bool initPhysics();  
   bool exitPhysics(); 
   void update();
   void process();
   void idle();

   PhysicsSceneInterface* NewSceneInterface(Scene* scene);

  //Default Shapes:
   bool createPlane(PhysicsSceneInterface* targetScene,const vec3& position = vec3(0,0,0), U32 size = 1);
   bool createBox(PhysicsSceneInterface* targetScene,const vec3& position = vec3(0,0,0), F32 size = 1);
   bool createActor(PhysicsSceneInterface* targetScene, SceneGraphNode* node, PhysicsActorMask mask,PhysicsCollisionGroup group);
   void registerActiveScene(PhysXSceneInterface* activeScene) {_currentScene = activeScene;}
   physx::PxPhysics* const getSDK() {return _gPhysicsSDK;}
   const physx::PxSimulationFilterShader& getFilterShader() {return _gDefaultFilterShader;}

private:
	PhysXSceneInterface* _currentScene;
	physx::PxPhysics* _gPhysicsSDK ;
	physx::PxDefaultErrorCallback _gDefaultErrorCallback;
	physx::PxDefaultAllocator _gDefaultAllocatorCallback;
	physx::PxSimulationFilterShader _gDefaultFilterShader;
	boost::mutex _physxMutex;

END_SINGLETON

#endif