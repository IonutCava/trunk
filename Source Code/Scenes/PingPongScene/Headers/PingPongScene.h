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

#ifndef _PINGPONG_SCENE_H
#define _PINGPONG_SCENE_H

#include "Scenes/Headers/Scene.h"

class PingPongScene : public Scene
{

public:
	PingPongScene() : Scene() {
		_miscareLaterala = 0;
		_directieAdversar = true;
		_directieSus = false;
		_atinsTeren = false;
		_atinsTerenAdversar = false;
		_pierdut = false;
		_mingeSGN = NULL;
		_minge = NULL;
	}

	~PingPongScene() {}
	void render();
	void preRender();

	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool unload();
	void processInput();
	void processEvents(F32 time);

	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void OnJoystickMovePOV(const OIS::JoyStickEvent& event,I8 pov);
	void OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis);
	void OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button);
private:
	void test(boost::any a, CallbackParam b);
	void servesteMingea();
	void reseteazaJoc();
private:
	std::vector<F32> _eventTimers;
	F32 angleLR,angleUD,moveFB,moveLR;
	I8 _scor;
	std::vector<std::string> _quotes;
	vec4 _sunVector;
	Sphere3D* _minge;
	SceneGraphNode* _mingeSGN;

private: //JOC:
	bool _directieAdversar;
	bool _directieSus;
	bool _atinsTeren;
	bool _atinsTerenAdversar;
	bool _pierdut;
	F32 _miscareLaterala;
};

#endif