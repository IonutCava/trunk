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

#ifndef _CUBE_SCENE_H
#define _CUBE_SCENE_H

#include "Scenes/Headers/Scene.h"
class Sphere3D;
class CubeScene : public Scene
{
public:
	CubeScene() : Scene(), _renderQuad(NULL),
						   _renderQuad2(NULL),_renderQuad3(NULL),_renderQuad4(NULL),_showTextures(false){}
	void render();
	void preRender();
	bool load(const std::string& name);
	bool unload();
	bool loadResources(bool continueOnErrors);
	void processInput();
	void processEvents(F32 time);

	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);

private:
	bool _showTextures;
	F32 angleLR,angleUD,moveFB,moveLR;
	Quad3D*	_renderQuad,*_renderQuad2,*_renderQuad3,*_renderQuad4;
};

#endif