/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _PHYSICS_DEVICE_H_
#define _PHYSICS_DEVICE_H_
#include "Dynamics/Physics/PhysX/Headers/PhysX.h"

DEFINE_SINGLETON_EXT1(PXDevice, PhysicsAPIWrapper)

public:
	void setApi(PhysicsAPI api);
	I8  getApi(){return _api.getId(); }

	inline bool initPhysics() {return _api.initPhysics();}
	inline bool exitPhysics() {return _api.exitPhysics();}
	inline void update() {_api.update();}
	inline void process() {_api.process();}
	inline void idle() {_api.idle();}
	inline PhysicsSceneInterface* NewSceneInterface(Scene* scene) {return _api.NewSceneInterface(scene);}

	inline bool createPlane(PhysicsSceneInterface* targetScene,const vec3& position = vec3(0,0,0), U32 size = 1){return _api.createPlane(targetScene,position,size);}
	inline bool createBox(PhysicsSceneInterface* targetScene,const vec3& position = vec3(0,0,0), F32 size = 1){return _api.createBox(targetScene,position,size);}
	inline bool createActor(PhysicsSceneInterface* targetScene, SceneGraphNode* node, PhysicsActorMask mask,PhysicsCollisionGroup group){return _api.createActor(targetScene, node,mask,group);}
private:
	PXDevice() :
	   _api(PhysX::getInstance()) //Defaulting to OpenGL if no api has been defined
	   {
	   }
	PhysicsAPIWrapper& _api;

END_SINGLETON

#define PHYSICS_DEVICE PXDevice::getInstance()
#endif