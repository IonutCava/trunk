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

#ifndef _AI_ENTITY_H_
#define _AI_ENTITY_H_

#include "resource.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/Sensors/Headers/CommunicationSensor.h"
#include "AI/ActionInterface/Headers/Coordination.h"

class ActionList;
class SceneGraphNode;

class AIEntity{
	friend class AIManager;
public:
	AIEntity(const std::string& name);
	
	void processInput();
	void processData();
	void update();

	bool attachNode(SceneGraphNode* const sgn) {_node = sgn; return true;}
	bool addSensor(SENSOR_TYPE type, Sensor* sensor);
	bool addFriend(AIEntity* entity);
	bool addEnemyTeam(AICoordination::teamMap& enemyTeam);
	bool addActionProcessor(ActionList* actionProcessor);
	Sensor* getSensor(SENSOR_TYPE type);
	inline AICoordination::teamMap& getTeam()  const {return _coordination->getTeam();}
	inline void       setTeamID(U32 ID) {_coordination->setTeamID(ID);}
	inline U32 const& getTeamID() const {return _coordination->getTeamID();}
	inline U32 const& getGUID()   const {return _GUID;}

	void sendMessage(AIEntity* receiver, AI_MSG msg,const boost::any& msg_content);
	void receiveMessage(AIEntity* sender, AI_MSG msg,const boost::any& msg_content);
	void processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content);
	const std::string& getName() {return _name;}
private:
	std::string     _name;
	U32             _GUID;
	SceneGraphNode* _node;
	AICoordination* _coordination;
	ActionList*     _actionProcessor;
	boost::mutex    _updateMutex;

	unordered_map<SENSOR_TYPE, Sensor*> _sensorList;
	
};

#endif