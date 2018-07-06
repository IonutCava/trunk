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

#ifndef _PHYSICS_SCENE_INTERFACE_H_
#define _PHYSICS_SCENE_INTERFACE_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class Scene;
class Transform;
class PhysicsAsset;
class SceneGraphNode;
class NOINITVTABLE PhysicsSceneInterface {
   public:
    PhysicsSceneInterface(Scene* parentScene);

    virtual ~PhysicsSceneInterface() {}
    /// Pre PHYSICS_DEVICE initialisation call
    virtual bool init() = 0;
    /// Called on interface destruction
    virtual void release() = 0;
    /// Custom physics idle calls
    virtual void idle() = 0;
    /// Physics update callback for custom behavior
    virtual void update(const U64 deltaTime) = 0;
    /// Custom process step
    virtual void process(const U64 deltaTime) = 0;

    Scene* getParentScene();

   protected:
    Scene* _parentScene;
};

};  // namespace Divide

#endif
