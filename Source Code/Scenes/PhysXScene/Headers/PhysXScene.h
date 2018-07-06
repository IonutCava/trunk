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

#ifndef _PHYSX_SCENE_H
#define _PHYSX_SCENE_H

#include "Scenes/Headers/Scene.h"

class PhysXScene : public Scene {
public:
    PhysXScene() : Scene()
    {
    }

    void preRender();

    bool load(const std::string& name, CameraManager* const cameraMgr);
    bool loadResources(bool continueOnErrors);
    bool unload();
    void processInput(const D32 deltaTime);
    void processTasks(const D32 deltaTime);
    void onKeyDown(const OIS::KeyEvent& key);
    void onKeyUp(const OIS::KeyEvent& key);
    void onMouseMove(const OIS::MouseEvent& key);
    void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
    void createStack(U32 size = 10);
    void createTower(U32 size = 10);

private:
    vec3<F32> _sunvector;
    boost::atomic<bool> _addingActors;
};

#endif