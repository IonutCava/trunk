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

#ifndef _PHYSX_SCENE_INTERFACE_H_
#define _PHYSX_SCENE_INTERFACE_H_

#include "PhysX.h"
class Scene;
class Transform;
class PhysXSceneInterface{
public:
	PhysXSceneInterface(Scene* parentScene) : _gScene(NULL),
											  _timeStep(1.0f/60.0f),
											  _parentScene(parentScene){}
	~PhysXSceneInterface(){}

	virtual bool init();
	virtual bool exit() = 0;
	virtual void idle() = 0;
	virtual void release();
	virtual void update();
	virtual void process();

	virtual void addRigidStaticActor(physx::PxRigidStatic* actor);
	virtual void addRigidDynamicActor(physx::PxRigidDynamic * actor);

protected:
	virtual void updateActor(physx::PxRigidActor* actor);
	virtual void updateShape(physx::PxShape* shape,Transform* t);
	virtual void addToSceneGraph(physx::PxRigidActor* actor);

private:
	Scene*          _parentScene;
	physx::PxScene* _gScene;
	physx::PxReal   _timeStep;
	std::vector<physx::PxRigidStatic* > _sceneRigidStaticActors;
	std::vector<physx::PxRigidDynamic* > _sceneRigidDynamicActors;
	boost::mutex _creationMutex;
};

#endif