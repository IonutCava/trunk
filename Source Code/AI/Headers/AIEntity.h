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

#ifndef _AI_ENTITY_H_
#define _AI_ENTITY_H_

#include "core.h"
#include "CommunicationInterface.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/Coordination.h"

class ActionList;
class SceneGraphNode;
class NPC;

class AIEntity{
	friend class AIManager;

public:
	AIEntity(const std::string& name);
	
	void processInput();
	void processData();
	void update();

	SceneGraphNode* getBoundNode() {return _node;}
	bool attachNode(SceneGraphNode* const sgn) {_node = sgn; return true;}
	bool addSensor(SENSOR_TYPE type, Sensor* sensor);
	bool setComInterface() {SAFE_UPDATE(_comInterface, New CommunicationInterface(this)); return true;}
	bool addActionProcessor(ActionList* actionProcessor);
	Sensor* getSensor(SENSOR_TYPE type);
	CommunicationInterface* getCommunicationInterface() {return _comInterface;}
	inline AICoordination* getTeam() {return _coordination; }
	inline U32  getTeamID() const    {if(_coordination != NULL) { return _coordination->getTeamID();} return -1; }
	inline U32  getGUID()   const    {return _GUID;}

	///Set a team for this Entity. If the enitity belonged to a different team, remove it from that team first
	void setTeam(AICoordination* const coordination);
	///Add a friend to our team
	bool addFriend(AIEntity* const friendEntity);

	void sendMessage(AIEntity* receiver, AI_MSG msg,const boost::any& msg_content);
	void receiveMessage(AIEntity* sender, AI_MSG msg,const boost::any& msg_content);
	void processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content);
	const std::string& getName() {return _name;}

	inline void addUnitRef(NPC* const npc) {_unitRef = npc;}
	inline NPC* getUnitRef()               {return _unitRef;}

private:
	std::string     _name;
	U32             _GUID;
	SceneGraphNode* _node;
	AICoordination* _coordination;
	ActionList*     _actionProcessor;
	mutable Lock    _updateMutex;
	mutable Lock    _managerQueryMutex;

	CommunicationInterface*             _comInterface;
	unordered_map<SENSOR_TYPE, Sensor*> _sensorList;
	NPC* _unitRef;
	
};

#endif