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
	void processTasks(const U32 time);

	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onJoystickMovePOV(const OIS::JoyStickEvent& event,I8 pov);
	void onJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis,I32 deadZone);
	void onJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button);
private:
	void test(boost::any a, CallbackParam b);
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
	F32 _sideDrift;
};

#endif