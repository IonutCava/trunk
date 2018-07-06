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

class Scene;
class SceneGraphNode;
#include "core.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

class PhysicsAsset {
public:
    PhysicsAsset();
    virtual ~PhysicsAsset();

protected:
    friend class PhysicsComponent;
    inline bool resetTransforms()            const { return _resetTransforms; }
    inline void resetTransforms(const bool state)  { _resetTransforms = state; }
    
    void setParent(PhysicsComponent* parent);
    PhysicsComponent* getComponent() { return _parentComponent; }

protected:
    bool _resetTransforms;
    PhysicsComponent* _parentComponent;
};

class PhysicsSceneInterface;
class PhysicsAPIWrapper {
protected:
    friend class PXDevice;
    PhysicsAPIWrapper() : _apiId(PX_PLACEHOLDER){}
    inline void setId(PhysicsAPI api) {_apiId = api;}
    inline PhysicsAPI getId() { return _apiId;}

    virtual ErrorCodes initPhysicsApi(U8 targetFrameRate) = 0;
    virtual bool       closePhysicsApi() = 0;
    virtual void updateTimeStep(U8 timeStepFactor)  = 0;
    virtual void updateTimeStep()  = 0;
    virtual void update(const U64 deltaTime) = 0;
    virtual void process(const U64 deltaTime) = 0;
    virtual void idle() = 0;
    virtual PhysicsSceneInterface* NewSceneInterface(Scene* scene) = 0;

    virtual bool createPlane(const vec3<F32>& position = VECTOR3_ZERO, U32 size = 1) = 0;
    virtual bool createBox(const vec3<F32>& position = VECTOR3_ZERO, F32 size = 1.0f) = 0;
    virtual bool createActor(SceneGraphNode* const node, const std::string& sceneName, PhysicsActorMask mask,PhysicsCollisionGroup group) = 0;
    virtual void setPhysicsScene(PhysicsSceneInterface* const targetScene) = 0;
    virtual void initScene() = 0;

 private:
    PhysicsAPI _apiId;
};

#endif