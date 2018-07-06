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

#ifndef _AITENIS_SCENE_H
#define _AITENIS_SCENE_H

#include "Scenes/Headers/Scene.h"

namespace Divide {

namespace AI {
class AIEntity;
class AITeam;
};

class Sphere3D;
class NPC;

class TenisScene : public Scene {
   public:
    explicit TenisScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name);

    bool load(const stringImpl& name);
    bool loadResources(bool continueOnErrors);
    void postLoadMainThread() override;
    bool initializeAI(bool continueOnErrors);
    bool deinitializeAI(bool continueOnErrors);
    void processInput(PlayerIndex idx, const U64 deltaTimeUS);
    void processTasks(const U64 deltaTimeUS);
    void processGUI(const U64 deltaTimeUS);

   private:
    // ToDo: replace with Physics system collision detection
    void checkCollisions();
    void playGame(const Task& parentTask, AnyParam a, CallbackParam b);
    void startGame(I64 btnGUID);
    void resetGame();

   private:
    vec3<F32> _sunvector;
    std::shared_ptr<Sphere3D> _ball;
    SceneGraphNode_wptr _ballSGN;
    SceneGraphNode_wptr _net;
    SceneGraphNode_wptr _floor;
    SceneGraphNode_wptr _sun;

   private:  // Game stuff
    mutable SharedLock _gameLock;
    mutable std::atomic_bool _collisionPlayer1;
    mutable std::atomic_bool _collisionPlayer2;
    mutable std::atomic_bool _collisionPlayer3;
    mutable std::atomic_bool _collisionPlayer4;
    mutable std::atomic_bool _collisionNet;
    mutable std::atomic_bool _collisionFloor;
    mutable std::atomic_bool _directionTeam1ToTeam2;
    mutable std::atomic_bool _upwardsDirection;
    mutable std::atomic_bool _touchedTerrainTeam1;
    mutable std::atomic_bool _touchedTerrainTeam2;
    mutable std::atomic_bool _lostTeam1;
    mutable std::atomic_bool _applySideImpulse;
    mutable std::atomic_bool _gamePlaying;
    mutable std::atomic_int _scoreTeam1;
    mutable std::atomic_int _scoreTeam2;
    F32 _sideImpulseFactor;
    
    SceneGraphNode_wptr _aiPlayer[4];
    /// Team's are factions for AIEntites so they can manage friend/foe
    /// situations
    AI::AITeam* _team1, *_team2;
};

};  // namespace Divide

#endif