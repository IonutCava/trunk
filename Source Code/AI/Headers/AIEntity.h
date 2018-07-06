/*
   Copyright (c) 2013 DIVIDE-Studio
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
#ifndef _AI_ENTITY_H_
#define _AI_ENTITY_H_

#include "core.h"
#include "CommunicationInterface.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/Coordination.h"
#include "Utility/Headers/GUIDWrapper.h"

class ActionList;
class SceneGraphNode;
class NPC;

class AIEntity : public GUIDWrapper {
	friend class AIManager;

public:
	AIEntity(const std::string& name);
	~AIEntity();
	void processInput();
	void processData();
	void update();

	SceneGraphNode* getBoundNode() {return _node;}
	bool attachNode(SceneGraphNode* const sgn) {_node = sgn; return true;}
	bool addSensor(SensorType type, Sensor* sensor);
	bool setComInterface() {SAFE_UPDATE(_comInterface, New CommunicationInterface(this)); return true;}
	bool addActionProcessor(ActionList* actionProcessor);
	Sensor* getSensor(SensorType type);
	CommunicationInterface* getCommunicationInterface() {return _comInterface;}
	inline AICoordination* getTeam() {return _coordination; }
	inline U32  getTeamID() const    {if(_coordination != NULL) { return _coordination->getTeamID();} return 0; }

	///Set a team for this Entity. If the enitity belonged to a different team, remove it from that team first
	void setTeam(AICoordination* const coordination);
	///Add a friend to our team
	bool addFriend(AIEntity* const friendEntity);

	void sendMessage(AIEntity* receiver, AIMsg msg,const boost::any& msg_content);
	void receiveMessage(AIEntity* sender, AIMsg msg,const boost::any& msg_content);
	void processMessage(AIEntity* sender, AIMsg msg, const boost::any& msg_content);
	const std::string& getName() {return _name;}

	inline void addUnitRef(NPC* const npc) {_unitRef = npc;}
	inline NPC* getUnitRef()               {return _unitRef;}

private:
	std::string     _name;
	SceneGraphNode* _node;
	AICoordination* _coordination;
	ActionList*     _actionProcessor;
	mutable SharedLock    _updateMutex;
	mutable SharedLock    _managerQueryMutex;

	typedef Unordered_map<SensorType, Sensor*> sensorMap;
	CommunicationInterface* _comInterface;
	sensorMap               _sensorList;
	NPC*                    _unitRef;
};

#endif