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

#ifndef _PHYSX_SCENE_H
#define _PHYSX_SCENE_H

#include "Scenes/Headers/Scene.h"
///For this scene, we will be using PhysX, so name members accordingly for more readable code
class PhysXImplementation;
class PhysXScene : public Scene {

public:
	PhysXScene() : Scene(), _physx(NULL), _mousePressed(false){}
	~PhysXScene() {}
	void render();
	void preRender();

	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool unload();
	void processInput();
	void processEvents(F32 time);
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onMouseMove(const OIS::MouseEvent& key);
	void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);
private:
	void createStack();
private:
	std::vector<F32> _eventTimers;
	vec4<F32> _sunVector;
	PhysXImplementation* _physx;
	bool _mousePressed;
	vec2<F32> _prevMouse;
};

#endif