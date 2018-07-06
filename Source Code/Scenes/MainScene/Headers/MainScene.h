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

#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "Scenes/Headers/Scene.h"

namespace Divide {

class Terrain;
FWD_DECLARE_MANAGED_CLASS(WaterPlane);

class MainScene : public Scene {
   public:
    explicit MainScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name);

    /*General Scene Requirement*/
    bool load(const stringImpl& name);
    bool unload();
    void postLoadMainThread() override;

   private:
    /*Specific Scene Requirement*/
    void updateLights();
    void processInput(PlayerIndex idx, const U64 deltaTimeUS)override;
    void processTasks(const U64 deltaTimeUS)override;
    void processGUI(const U64 deltaTimeUS)override;
    void test(const Task& parentTask, AnyParam a, CallbackParam b);
    U16 registerInputActions() override;

   private:
    vec2<F32> _sunAngle;
    vec4<F32> _sunvector;
    FColour _sunColour;
    F32 _sun_cosy;
    bool _musicPlaying;
    bool _freeflyCamera;
    bool _updateLights;
    AudioDescriptor_ptr _beep;
};

};  // namespace Divide

#endif
