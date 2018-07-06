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

#ifndef _PHYSX_SCENE_INTERFACE_H_
#define _PHYSX_SCENE_INTERFACE_H_

#include "PhysX.h"
#include "Dynamics/Physics/Headers/PhysicsSceneInterface.h"
#include <boost/thread/locks.hpp>
#include <boost/atomic.hpp>
#include <boost/lockfree/stack.hpp>

class Scene;
class Transform;
class PhysXSceneInterface : public PhysicsSceneInterface {

public:
	PhysXSceneInterface(Scene* parentScene) : PhysicsSceneInterface(parentScene),
											  _gScene(NULL){}
	virtual ~PhysXSceneInterface(){exit();}

	virtual bool init();
	virtual bool exit();
	virtual void idle();
	virtual void release();
	virtual void update();
	virtual void process(F32 timeStep);

	void addRigidStaticActor(physx::PxRigidStatic* const actor);
	void addRigidDynamicActor(physx::PxRigidDynamic* const actor);
    inline const vectorImpl<physx::PxMaterial* > getMaterials() {return _materials;}
    inline physx::PxScene* getPhysXScene() {return _gScene;}

protected:
    void updateActor(physx::PxRigidActor* const actor);
	void updateShape(physx::PxShape* const shape,Transform* const t);
    ///Adds the actor to the PhysX scene and the SceneGraph. Returns a pointer to the new SceneGraph node created;
    SceneGraphNode* addToScene(physx::PxRigidActor* const actor);
private:
    typedef vectorImpl<physx::PxRigidStatic*  > RigidStaticMap;
    typedef vectorImpl<physx::PxRigidDynamic* > RigidDynamicMap;
    typedef vectorImpl<physx::PxRigidStatic*  > RigidStaticQueue;
    typedef vectorImpl<physx::PxRigidDynamic* > RigidDynamicQueue;

    bool _addedPhysXPlane;
	physx::PxScene* _gScene;
    vectorImpl<physx::PxMaterial* > _materials;
    RigidStaticMap _sceneRigidStaticActors;
    RigidDynamicMap _sceneRigidDynamicActors;
    RigidStaticQueue _sceneRigidStaticQueue;
    RigidDynamicQueue _sceneRigidDynamicQueue;
    boost::atomic<I32> _rigidStaticCount;
    boost::atomic<I32> _rigidDynamicCount;
    mutable SharedLock _queueLock;
};

#endif
