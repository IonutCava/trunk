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

#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "Scenes/Headers/Scene.h"

namespace Divide {

class Terrain;
class WaterPlane;

class MainScene : public Scene {
   public:
    MainScene()
        : Scene(),
          _water(nullptr),
          _beep(nullptr),
          _freeflyCamera(false),
          _updateLights(true),
          _musicPlaying(false)
    {
    }

    /*General Scene Requirement*/
    bool load(const stringImpl& name, GUI* const gui);
    bool unload();
    bool loadResources(bool continueOnErrors);

   private:
    /*Specific Scene Requirement*/
    void updateLights();
    void processInput(const U64 deltaTime);
    void processTasks(const U64 deltaTime);
    void processGUI(const U64 deltaTime);
    void test(cdiggins::any a, CallbackParam b);
  
   private:
    vec2<F32> _sunAngle;
    vec4<F32> _sunvector, _sunColor;
    F32 _sun_cosy;
    bool _musicPlaying;
    bool _freeflyCamera;
    bool _updateLights;
    AudioDescriptor* _beep;
    vectorImpl<SceneGraphNode_wptr> _visibleTerrains;
    WaterPlane* _water;
    SceneGraphNode_wptr _sun;
    SceneGraphNode_wptr _waterGraphNode;
};

};  // namespace Divide

#endif
