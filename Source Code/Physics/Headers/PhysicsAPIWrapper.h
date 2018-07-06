/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _PHYSICS_API_WRAPPER_H_
#define _PHYSICS_API_WRAPPER_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

enum class RigidBodyShape : U32 {
    SHAPE_SPHERE = 0,
    SHAPE_PLANE,
    SHAPE_CAPSULE,
    SHAPE_BOX,
    SHAPE_CONVEXMESH,
    SHAPE_TRIANGLEMESH,
    SHAPE_HEIGHTFIELD,
    SHAPE_COUNT
};

class Scene;
class PhysicsAsset;
class SceneGraphNode;
class PhysicsSceneInterface;
typedef std::shared_ptr<SceneGraphNode> SceneGraphNode_ptr;

enum class ErrorCode : I32;
class NOINITVTABLE PhysicsAPIWrapper {
   protected:
    friend class PXDevice;
    virtual ErrorCode initPhysicsAPI(U8 targetFrameRate) = 0;
    virtual bool closePhysicsAPI() = 0;
    virtual void updateTimeStep(U8 timeStepFactor) = 0;
    virtual void update(const U64 deltaTime) = 0;
    virtual void process(const U64 deltaTime) = 0;
    virtual void idle() = 0;
    virtual PhysicsSceneInterface* NewSceneInterface(Scene* scene) = 0;

    virtual void setPhysicsScene(PhysicsSceneInterface* const targetScene) = 0;
    virtual void initScene() = 0;

    virtual PhysicsAsset* createRigidActor(const SceneGraphNode& node) = 0;
};

};  // namespace Divide

#endif
