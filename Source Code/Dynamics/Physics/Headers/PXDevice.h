/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _PHYSICS_DEVICE_H_
#define _PHYSICS_DEVICE_H_
#include "Dynamics/Physics/PhysX/Headers/PhysX.h"

#ifndef _PHYSICS_API_FOUND_
#error "No physics library implemented!"
#endif

DEFINE_SINGLETON_EXT1(PXDevice, PhysicsAPIWrapper)

public:
    void setApi(PhysicsAPI api);
    inline PhysicsAPI getApi() const {return _api.getId(); }

    inline ErrorCode initPhysicsApi(U8 targetFrameRate) { return _api.initPhysicsApi(targetFrameRate); }
    inline bool      closePhysicsApi() { return _api.closePhysicsApi(); }
    inline void updateTimeStep(U8 timeStepFactor) {_api.updateTimeStep(timeStepFactor);}
    inline void updateTimeStep()                  {_api.updateTimeStep();}
    inline void update(const U64 deltaTime) {_api.update(deltaTime);}
    inline void process(const U64 deltaTime) {_api.process(deltaTime);}
    inline void idle() {_api.idle();}
    inline void setPhysicsScene(PhysicsSceneInterface* const targetScene) {_api.setPhysicsScene(targetScene);}
    inline void initScene(){_api.initScene();}

    inline PhysicsSceneInterface* NewSceneInterface(Scene* scene) {
        return _api.NewSceneInterface(scene);
    }

    inline bool createPlane(const vec3<F32>& position = VECTOR3_ZERO, U32 size = 1){
        return _api.createPlane(position,size);
    }

    inline bool createBox(const vec3<F32>& position = VECTOR3_ZERO, F32 size = 1.0f){
        return _api.createBox(position,size);
    }

    inline bool createActor(SceneGraphNode* const node, const std::string& sceneName, PhysicsActorMask mask,PhysicsCollisionGroup group){
        return _api.createActor(node,sceneName,mask,group);
    }

private:
    PXDevice() :
       _api(PhysX::getOrCreateInstance()) //Defaulting to nothing if no api has been defined
       {
       }
    PhysicsAPIWrapper& _api;

END_SINGLETON

#define PHYSICS_DEVICE PXDevice::getInstance()
#endif