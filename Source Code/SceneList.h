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
#ifndef _DIVIDE_SCENE_LIST_H_
#define _DIVIDE_SCENE_LIST_H_

// Initial application scene (menu, scene transition, etc) :
#include "Scenes/DefaultScene/Headers/DefaultScene.h"

// Factory scenes:
#include "Scenes/MainScene/Headers/MainScene.h"
#include "Scenes/PingPongScene/Headers/PingPongScene.h"
#include "Scenes/WarScene/Headers/WarScene.h"

#define STRUCT_NAME(M) BOOST_PP_CAT(M, RegisterStruct)
#define VAR_NAME(M) BOOST_PP_CAT(M, RegisterVariable)

 /// usage: REGISTER_SCENE(A) where: - A is the scene's class name scene in the XML files
 /// Call this function after each scene declaration
#define REGISTER_SCENE(scene)                           \
static struct STRUCT_NAME(scene) {                      \
  STRUCT_NAME(scene)();                                 \
} VAR_NAME(scene);                                      \
                                                        \
STRUCT_NAME(scene)::STRUCT_NAME(scene)()  {             \
    g_sceneFactory[_ID(#scene)]  = boost::factory<scene*>(); \
    g_sceneNameMap[_ID(#scene)]  = #scene; \
}

#define INIT_SCENE_FACTORY \
    namespace { \
        using SceneConstructor = std::function<Scene*(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const Str128& name)>; \
        using SceneFactoryMap = hashMap<U64, SceneConstructor>; \
        using SceneNameMap = hashMap<U64, Str128>; \
        SceneFactoryMap g_sceneFactory; \
        SceneNameMap g_sceneNameMap; \
    };\
    REGISTER_SCENE(DefaultScene)\
    REGISTER_SCENE(MainScene)\
    REGISTER_SCENE(PingPongScene)\
    REGISTER_SCENE(WarScene)
    

#endif