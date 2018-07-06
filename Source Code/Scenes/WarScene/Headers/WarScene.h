/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _WAR_SCENE_H
#define _WAR_SCENE_H

#include "Scenes/Headers/Scene.h"

namespace Divide {

class SkinnedSubMesh;
class NPC;

namespace AI {
class AITeam;
class AIEntity;
class WarSceneOrder;
};

class WarScene : public Scene {
   public:
    WarScene();
    ~WarScene();

    bool load(const stringImpl& name, GUI* const gui);
    bool loadResources(bool continueOnErrors);
    bool initializeAI(bool continueOnErrors);
    bool deinitializeAI(bool continueOnErrors);
    void processTasks(const U64 deltaTime);
    void processGUI(const U64 deltaTime);
    void updateSceneStateInternal(const U64 deltaTime);

   private:
    void startSimulation();
    void toggleCamera();

   private:
    I8 _score;
    DirectionalLight* _sun;
    GUIMessageBox* _infoBox;

   private:  // Game
    bool _sceneReady;
    U64 _lastNavMeshBuildTime;
    /// AIEntities are the "processors" behing the NPC's
    vectorImpl<AI::AIEntity*> _army[2];
    /// NPC's are the actual game entities
    vectorImpl<NPC*> _armyNPCs[2];
    SceneGraphNode* _flag[2];
    /// Teams are factions for AIEntites so they can manage friend/foe situations
    AI::AITeam* _faction[2];
};

};  // namespace Divide

#endif