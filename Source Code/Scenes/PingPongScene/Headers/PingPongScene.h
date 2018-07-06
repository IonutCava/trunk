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
		_ballSGN = nullptr;
		_ball = nullptr;
	}

	~PingPongScene() {}
	void preRender();

	bool load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui);
	bool loadResources(bool continueOnErrors);
	void processInput(const U64 deltaTime);
	void processTasks(const U64 deltaTime);
    void processGUI(const U64 deltaTime);

	bool onKeyUp(const OIS::KeyEvent& key);
	bool onJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis,I32 deadZone);
	bool onJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button);

private:
	void test(cdiggins::any a, CallbackParam b);
	void serveBall();
	void resetGame();

private:
	I8 _score;
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
    bool _freeFly;
    bool _wasInFreeFly;
	F32 _sideDrift;
};

#endif