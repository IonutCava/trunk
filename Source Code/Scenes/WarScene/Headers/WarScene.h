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

#ifndef _WAR_SCENE_H
#define _WAR_SCENE_H

#include "Scenes/Headers/Scene.h"

class SkinnedSubMesh;
class AICoordination;
class AIEntity;
class NPC;

class WarScene : public Scene {
public:
    WarScene() : Scene(),
        _groundPlaceholder(NULL),
        _faction1(NULL),
        _faction2(NULL),
        _bobNode(NULL),
        _bobNodeBody(NULL),
        _lampLightNode(NULL),
        _lampTransform(NULL),
        _lampTransformNode(NULL),
        _sceneReady(false)
    {
        _scorTeam1 = 0;
        _scorTeam2 = 0;
    }

    void preRender();

    bool load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui);
    bool loadResources(bool continueOnErrors);
    bool initializeAI(bool continueOnErrors);
    bool deinitializeAI(bool continueOnErrors);
    void processInput(const U64 deltaTime);
    void processTasks(const U64 deltaTime);
    void updateSceneState(const U64 deltaTime);

    bool onKeyDown(const OIS::KeyEvent& key);
    bool onKeyUp(const OIS::KeyEvent& key);
    bool onMouseMove(const OIS::MouseEvent& key);
    bool onMouseClickDown(const OIS::MouseEvent& key, OIS::MouseButtonID button);
    bool onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
    void processSimulation(boost::any a, CallbackParam b);
    void startSimulation();
    void resetSimulation();

private:
    I8 _score;
    vec4<F32> _sunvector;
    SceneGraphNode* _groundPlaceholder;

private: //Game
    bool _sceneReady;
    I8 _scorTeam1;
    I8 _scorTeam2;
    ///AIEntities are the "processors" behing the NPC's
    vectorImpl<AIEntity *> _army1;
    vectorImpl<AIEntity *> _army2;
    ///NPC's are the actual game entities
    vectorImpl<NPC *> _army1NPCs;
    vectorImpl<NPC *> _army2NPCs;
    ///Team's are factions for AIEntites so they can manage friend/foe situations
    AICoordination *_faction1, *_faction2;
    SceneGraphNode *_bobNode;
    SceneGraphNode *_bobNodeBody;
    SceneGraphNode *_lampLightNode;
    SceneGraphNode *_lampTransformNode;
    SceneNode      *_lampTransform;
};

#endif