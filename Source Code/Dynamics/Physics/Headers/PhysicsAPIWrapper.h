#ifndef _PHYSICS_API_WRAPPER_H_
#define _PHYSICS_API_WRAPPER_H_

enum PhysicsCollisionGroup{
    GROUP_NON_COLLIDABLE,
    GROUP_COLLIDABLE_NON_PUSHABLE,
    GROUP_COLLIDABLE_PUSHABLE,
};

enum PhysicsActorMask{
	MASK_RIGID_STATIC,
	MASK_RIGID_DYNAMIC
};

enum PhysicsAPI {

	PHYSX,
	ODE,
	BULLET,
	PX_PLACEHOLDER
};
#include "Hardware/Platform/Headers/PlatformDefines.h"
class Scene;
class SceneGraphNode;
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
#include "Hardware/Platform/Headers/PlatformDefines.h"
class PhysicsSceneInterface;
class PhysicsAPIWrapper {

protected:
	friend class PXDevice;
	PhysicsAPIWrapper() : _apiId(PX_PLACEHOLDER){}
	inline void setId(PhysicsAPI api) {_apiId = api;}
	inline PhysicsAPI getId() { return _apiId;}

    virtual I8 initPhysics(U8 targetFrameRate) = 0;  
	virtual bool exitPhysics() = 0;
	virtual void updateTimeStep(U8 timeStepFactor)  = 0;
    virtual void update() = 0;
    virtual void process() = 0;
    virtual void idle() = 0;
	virtual PhysicsSceneInterface* NewSceneInterface(Scene* scene) = 0;
	virtual bool createPlane(PhysicsSceneInterface* targetScene,const vec3<F32>& position = vec3<F32>(0,0,0), U32 size = 1) = 0;
	virtual bool createBox(PhysicsSceneInterface* targetScene,const vec3<F32>& position = vec3<F32>(0,0,0), F32 size = 1) = 0;
	virtual bool createActor(PhysicsSceneInterface* targetScene,SceneGraphNode* node, PhysicsActorMask mask,PhysicsCollisionGroup group) = 0;
private:
	PhysicsAPI _apiId;

};

#endif