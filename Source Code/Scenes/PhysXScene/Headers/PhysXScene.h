/*
   Copyright (c) 2017 DIVIDE-Studio
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
    explicit PhysXScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
        : Scene(context, cache, parent, name)
    {
        _hasGroundPlane = false;
    }

    bool load(const stringImpl& name) override;
    void postLoadMainThread() override;
    bool loadResources(bool continueOnErrors) override;
    bool unload() override;
    void processInput(PlayerIndex idx, const U64 deltaTimeUS) override;
    void processGUI(const U64 deltaTimeUS) override;
    U16 registerInputActions() override;

   private:
    void createStack(const Task& parentTask, U32 size = 10);
    void createTower(const Task& parentTask, U32 size = 10);

   private:
    bool _hasGroundPlane;
    vec3<F32> _sunvector;
    SceneGraphNode* _sun;
};

};  // namespace Divide

#endif