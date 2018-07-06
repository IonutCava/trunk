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

#ifndef _WAR_SCENE_H
#define _WAR_SCENE_H

#include "Scenes/Headers/Scene.h"

class AICoordination;
class AIEntity;
class NPC;

class WarScene : public Scene {

public:
	WarScene() : Scene(),
		_groundPlaceholder(NULL),
		_faction1(NULL),
		_faction2(NULL){
		_scorTeam1 = 0;
		_scorTeam2 = 0;
		_mousePressed = false;
	}

	void preRender();

	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool initializeAI(bool continueOnErrors);
	bool deinitializeAI(bool continueOnErrors);
	void processInput();
	void processEvents(F32 time);
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onMouseMove(const OIS::MouseEvent& key);
	void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
	void processSimulation(boost::any a, CallbackParam b);
	void startSimulation();
	void resetSimulation();
private:
	vectorImpl<F32> _eventTimers;
	I8 _score;
	vec4<F32> _sunvector;
	SceneGraphNode* _groundPlaceholder;
	vec2<F32> _prevMouse;
	bool _mousePressed;

private: //Joc
	I8 _scorTeam1;
	I8 _scorTeam2;
	///AIEntities are the "processors" behing the NPC's
	vectorImpl<AIEntity *> _army1;
	vectorImpl<AIEntity *> _army2;
	///NPC's are the actual game entities
	vectorImpl<NPC *> _army1NPCs;
	vectorImpl<NPC *> _army2NPCs;
	///Team's are factions for AIEntites so they can manage friend/foe situations
	AICoordination *_faction1, *_faction2;
};

#endif