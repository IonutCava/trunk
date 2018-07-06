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

#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "Scenes/Headers/Scene.h"

class Terrain;
class WaterPlane;

class MainScene : public Scene{

public:
	MainScene() : Scene(), 
				  _waterGraphNode(NULL),
				  _water(NULL),
				  _beep(NULL),
				  _mousePressed(false){}

	/*General Scene Requirement*/
	void preRender();
	bool load(const std::string& name);
	bool unload();
	bool loadResources(bool continueOnErrors);


private:
	/*Specific Scene Requirement*/
	void renderEnvironment(bool waterReflection);
	bool updateLights();
	void processInput();
	void processEvents(U32 time);
	void test(boost::any a, CallbackParam b);
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onMouseMove(const OIS::MouseEvent& key);
	void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
	
	vec2<F32> _sunAngle;
	vec4<F32> _sunvector,_sunColor;
	F32  _sun_cosy;
	vec2<F32> _prevMouse;
	bool _mousePressed;
	AudioDescriptor* _beep;
	vectorImpl<Terrain*> _visibleTerrains;
	WaterPlane* _water;
	SceneGraphNode* _waterGraphNode;
};

#endif;