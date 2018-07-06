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

#ifndef _WAR_SCENE_AI_ACTION_LIST_H_
#define _WAR_SCENE_AI_ACTION_LIST_H_

#include "AI/ActionInterface/Headers/ActionList.h"

enum AIMsg{
    REQUEST_DISTANCE_TO_TARGET = 0,
    RECEIVE_DISTANCE_TO_TARGET = 1,
    CHANGE_DESTINATION_POINT = 2
};

class WarSceneAIActionList : public ActionList{
public:
    WarSceneAIActionList();
    void processData(const U64 deltaTime);
    void processInput(const U64 deltaTime);
    void update(NPC* unitRef = nullptr);
    void addEntityRef(AIEntity* entity);
    void processMessage(AIEntity* sender, AIMsg msg,const boost::any& msg_content);

private:
    void updatePositions();

private:
    U16       _tickCount;
    I32       _indexInMap;
    U64       _deltaTime;
    AIEntity* _currentEnemyTarget;
    Navigation::NavigationMesh* _navMesh;
};

#endif