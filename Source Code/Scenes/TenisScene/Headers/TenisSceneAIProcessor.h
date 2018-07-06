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

#ifndef _AI_TENIS_SCENE_AI_PROCESSOR_H_
#define _AI_TENIS_SCENE_AI_PROCESSOR_H_

#include "AI/ActionInterface/Headers/AIProcessor.h"

namespace Divide {
namespace AI {

enum class AIMsg : U32 {
    REQUEST_DISTANCE_TO_TARGET = 0,
    RECEIVE_DISTANCE_TO_TARGET = 1,
    ATTACK_BALL = 2,
    DONT_ATTACK_BALL = 3
};

class TenisSceneAIProcessor : public AIProcessor {
   public:
    TenisSceneAIProcessor(SceneGraphNode* target, AIManager& parentManager);
    bool processData(const U64 deltaTimeUS);
    bool processInput(const U64 deltaTimeUS);
    bool update(const U64 deltaTimeUS, NPC* unitRef = nullptr);
    void addEntityRef(AIEntity* entity);
    void processMessage(AIEntity& sender, AIMsg msg, const AnyParam& msg_content);

    inline stringImpl toString(bool state = false) const { return ""; }

   private:
    void updatePositions();
    void initInternal() {}
    F32 distanceToBall(const vec3<F32>& entityPosition,
                       const vec3<F32> ballPosition);
    bool performActionStep(GOAPAction::operationsIterator step) { return true; }
    bool performAction(const GOAPAction& planStep) { return true; }

   private:
       SceneGraphNode* _target;
    vec3<F32> _ballPosition, _prevBallPosition, _entityPosition,
        _initialPosition;
    bool _attackBall, _ballToTeam2, _gameStop;
    U16 _tickCount;
};

};  // namespace AI
};  // namespace Divide

#endif //_AI_TENIS_SCENE_AI_PROCESSOR_H_