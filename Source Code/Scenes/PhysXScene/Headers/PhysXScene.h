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

#ifndef _PHYSX_SCENE_H
#define _PHYSX_SCENE_H

#include "Scenes/Headers/Scene.h"

namespace Divide {

class PhysXScene : public Scene {
   public:
    PhysXScene() : Scene() { _hasGroundPlane = false; }

    void preRender();

    bool load(const stringImpl& name, GUI* const gui);
    bool loadResources(bool continueOnErrors);
    bool unload();
    void processInput(const U64 deltaTime);
    void processGUI(const U64 deltaTime);

   private:
    void createStack(U32 size = 10);
    void createTower(U32 size = 10);

   private:
    bool _hasGroundPlane;
    vec3<F32> _sunvector;
    SceneGraphNode_wptr _sun;
};

};  // namespace Divide

#endif