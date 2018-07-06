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

#ifndef _PINGPONG_SCENE_H
#define _PINGPONG_SCENE_H

#include "Scenes/Headers/Scene.h"

class Sphere3D;

class PingPongScene : public Scene {

public:
	PingPongScene() : Scene() {
		_sideDrift = 0;
		_directionTowardsAdversary = true;
		_upwardsDirection = false;
		_touchedOwnTableHalf = false;
		_touchedAdversaryTableHalf = false;
		_lost = false;
		_ballSGN = NULL;
		_ball = NULL;
	}

	~PingPongScene() {}
	void preRender();

	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	void processInput();
	void processEvents(F32 time);

	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void OnJoystickMovePOV(const OIS::JoyStickEvent& event,I8 pov);
	void OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis);
	void OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button);
private:
	void test(boost::any a, CallbackParam b);
	void serveBall();
	void resetGame();
private:
	vectorImpl<F32> _eventTimers;
	I8 _scor;
	vectorImpl<std::string> _quotes;
	vec3<F32> _sunvector;
	Sphere3D* _ball;
	SceneGraphNode* _ballSGN;

private: //Game stuff:
	bool _directionTowardsAdversary;
	bool _upwardsDirection;
	bool _touchedOwnTableHalf;
	bool _touchedAdversaryTableHalf;
	bool _lost;
	F32 _sideDrift;
};

#endif