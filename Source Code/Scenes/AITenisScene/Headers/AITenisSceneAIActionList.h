/*“Copyright 2009-2011 DIVIDE-Studio”*/
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
	LOVESTE_MINGEA = 2,
	NU_LOVI_MINGEA = 3
};

class AITenisSceneAIActionList : public ActionList{
	typedef unordered_map<AIEntity*, F32 > membruDistantaMap;
public:
	AITenisSceneAIActionList(SceneGraphNode* target);
	void processData();
	void processInput();
	void update(SceneGraphNode* node);
	void addEntityRef(AIEntity* entity);
	void processMessage(AIEntity* sender, AI_MSG msg,const boost::any& msg_content);

private:
	void updatePositions();

private:
	SceneGraphNode* _node;
	SceneGraphNode* _target;
	vec3 _pozitieMinge, _pozitieMingeAnterioara, _pozitieEntitate, _pozitieInitiala;
	membruDistantaMap _membruDistanta;
	bool _atacaMingea, _mingeSpreEchipa2,_stopJoc;
	U16 _tickCount;
};

#endif