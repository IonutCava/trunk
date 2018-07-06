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
                                              _gScene(NULL),
                                              _cpuDispatcher(NULL)
    {
    }

    virtual ~PhysXSceneInterface(){exit();}

    virtual bool init();
    virtual bool exit();
    virtual void idle();
    virtual void release();
    virtual void update();
    virtual void process(F32 timeStep);

    PhysXActor* getOrCreateRigidActor(const std::string& actorName);
    void addRigidActor(PhysXActor* const actor, bool threaded = true);
    inline const vectorImpl<physx::PxMaterial* > getMaterials() {return _materials;}
    inline physx::PxScene* getPhysXScene() {return _gScene;}

protected:
    void updateActor(const PhysXActor& actor);
    void updateShape(physx::PxShape* const shape,Transform* const t);
    ///Adds the actor to the PhysX scene and the SceneGraph. Returns a pointer to the new SceneGraph node created;
    SceneGraphNode* addToScene(PhysXActor& actor);

private:
    typedef vectorImpl<PhysXActor* >        RigidMap;
    typedef vectorImpl<physx::PxMaterial* > MaterialMap;
    bool _addedPhysXPlane;
    physx::PxScene* _gScene;
    physx::PxDefaultCpuDispatcher*	_cpuDispatcher;
    MaterialMap _materials;
    RigidMap    _sceneRigidActors;
    RigidMap    _sceneRigidQueue;
    boost::atomic<U32> _rigidCount;
    mutable SharedLock _queueLock;
};

#endif
