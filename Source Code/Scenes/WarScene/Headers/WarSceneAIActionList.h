/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#ifndef _WAR_SCENE_AI_ACTION_LIST_H_
#define _WAR_SCENE_AI_ACTION_LIST_H_
#include "AI/ActionInterface/Headers/ActionList.h"

enum AIMsg{
	REQUEST_DISTANCE_TO_TARGET = 0,
	RECEIVE_DISTANCE_TO_TARGET = 1,
};

class WarSceneAIActionList : public ActionList{

public:
	WarSceneAIActionList();
	void processData();
	void processInput();
	void update(SceneGraphNode* node = NULL, NPC* unitRef = NULL);
	void addEntityRef(AIEntity* entity);
	void processMessage(AIEntity* sender, AIMsg msg,const boost::any& msg_content);

private:
	void updatePositions();

private:
	SceneGraphNode* _node;
	U16 _tickCount;
};

#endif