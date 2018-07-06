/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AI_TENIS_SCENE_AI_ACTION_LIST_H_
#define _AI_TENIS_SCENE_AI_ACTION_LIST_H_
#include "AI/ActionInterface/Headers/ActionList.h"

enum AI_MSG{
	REQUEST_DISTANCE_TO_TARGET = 0,
	RECEIVE_DISTANCE_TO_TARGET = 1,
	ATTACK_BALL = 2,
	DONT_ATTACK_BALL = 3
};

class AITenisSceneAIActionList : public ActionList{
public:
	AITenisSceneAIActionList(SceneGraphNode* target);
	void processData();
	void processInput();
	void update(SceneGraphNode* node = NULL, NPC* unitRef = NULL);
	void addEntityRef(AIEntity* entity);
	void processMessage(AIEntity* sender, AI_MSG msg,const boost::any& msg_content);

private:
	void updatePositions();
	F32 distanceToBall(const vec3<F32>& entityPosition, const vec3<F32> ballPosition);

private:
	SceneGraphNode* _node;
	SceneGraphNode* _target;
	vec3<F32> _ballPosition, _prevBallPosition, _entityPosition, _initialPosition;
	bool _attackBall, _ballToTeam2,_gameStop;
	U16 _tickCount;
};

#endif