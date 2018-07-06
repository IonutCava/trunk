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

#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "Scene.h"
#include "Hardware/Video/FrameBufferObject.h"
class WaterPlane;
class MainScene : public Scene{

public:
	/*General Scene Requirement*/
	void render();
	void preRender();
	bool load(const std::string& name);
	bool unload();
	bool loadResources(bool continueOnErrors);
	bool loadEvents(bool continueOnErrors){return true;}


private:
	/*Specific Scene Requirement*/
	void renderEnvironment(bool waterReflection, bool depthMap);
	bool updateLights();
	void processInput();
	void processEvents(F32 time);
	void test(boost::any a, CallbackParam b);
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onMouseMove(const OIS::MouseEvent& key);
	void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
	
	vec2 _sunAngle;
	vec4 _sunVector,_sunColor;
	F32  _sun_cosy;
	F32 angleLR,angleUD,moveFB,moveLR;
	vec2 _prevMouse;
	bool _mousePressed;
	AudioDescriptor* _backgroundMusic, *_beep;
	std::vector<Terrain*> _visibleTerrains;
	WaterPlane* _water;
	SceneGraphNode* _waterGraphNode;

};

#endif;