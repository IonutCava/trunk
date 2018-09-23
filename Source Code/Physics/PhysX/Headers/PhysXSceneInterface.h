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
#ifndef _PHYSX_SCENE_INTERFACE_H_
#define _PHYSX_SCENE_INTERFACE_H_

#include "PhysX.h"
#include "Physics/Headers/PhysicsSceneInterface.h"

namespace Divide {

class Scene;

class PhysXSceneInterface : public PhysicsSceneInterface {
    typedef moodycamel::ConcurrentQueue<PhysXActor*> LoadQueue;

   public:
    PhysXSceneInterface(Scene& parentScene);
    virtual ~PhysXSceneInterface();

    virtual bool init();
    virtual void idle();
    virtual void release();
    virtual void update(const U64 deltaTimeUS);
    virtual void process(const U64 deltaTimeUS);

    void addRigidActor(PhysXActor* const actor, bool threaded = true);
    inline const vector<physx::PxMaterial*> getMaterials() {
        return _materials;
    }
    inline physx::PxScene* getPhysXScene() { return _gScene; }

   protected:
    void updateActor(PhysXActor& actor);
    /// Adds the actor to the PhysX scene
    void addToScene(PhysXActor& actor);

   private:
    typedef vector<PhysXActor*> RigidMap;
    typedef vector<physx::PxMaterial*> MaterialMap;
    physx::PxScene* _gScene;
    physx::PxDefaultCpuDispatcher* _cpuDispatcher;
    MaterialMap _materials;
    RigidMap _sceneRigidActors;
    LoadQueue _sceneRigidQueue;
};

};  // namespace Divide

#endif //_PHYSX_SCENE_INTERFACE_H_
