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

#ifndef _PHYSICS_DEVICE_H_
#define _PHYSICS_DEVICE_H_
#include "Dynamics/Physics/PhysX/Headers/PhysX.h"

#ifndef _PHYSICS_API_FOUND_
#error "No physics library implemented!"
#endif

DEFINE_SINGLETON_EXT1(PXDevice, PhysicsAPIWrapper)

public:
	void setApi(PhysicsAPI api);
	inline I8  getApi(){return _api.getId(); }

	inline I8   initPhysics(U8 targetFrameRate) {return _api.initPhysics(targetFrameRate);}
	inline bool exitPhysics() {return _api.exitPhysics();}
	inline void updateTimeStep(U8 timeStepFactor) {_api.updateTimeStep(timeStepFactor);}
	inline void update() {_api.update();}
	inline void process() {_api.process();}
	inline void idle() {_api.idle();}
    inline void setPhysicsScene(PhysicsSceneInterface* const targetScene) {_api.setPhysicsScene(targetScene);}
    inline void initScene(){_api.initScene();}
	inline PhysicsSceneInterface* NewSceneInterface(Scene* scene) {return _api.NewSceneInterface(scene);}

	inline bool createPlane(const vec3<F32>& position = vec3<F32>(0,0,0), U32 size = 1){return _api.createPlane(position,size);}
	inline bool createBox(const vec3<F32>& position = vec3<F32>(0,0,0), F32 size = 1){return _api.createBox(position,size);}
	inline bool createActor(SceneGraphNode* const node, PhysicsActorMask mask,PhysicsCollisionGroup group){return _api.createActor(node,mask,group);}
private:
	PXDevice() :
	   _api(PhysX::getInstance()) //Defaulting to nothig if no api has been defined
	   {
	   }
	PhysicsAPIWrapper& _api;

END_SINGLETON

#define PHYSICS_DEVICE PXDevice::getInstance()
#endif