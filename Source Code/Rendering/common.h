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

#ifndef ENGINE_H_
#define ENGINE_H_

#include "resource.h"
#include "Utility/Headers/Singleton.h"

class PhysX;
class Camera;
class SceneManager;
class GUI;
class Terrain;
class GFXDevice;
class SFXDevice;

SINGLETON_BEGIN( Engine )

private:
	Engine();
	I8 mainWindowId;
	vec2 _dimensions;

	GFXDevice&    _GFX;
	SFXDevice&    _SFX;
    PhysX&        _px;
	SceneManager& _scene;
	Camera*       _camera;
	GUI&          _gui;

public:
	const vec2& getWindowDimensions() const {return _dimensions;}
	void setWindowWidth(U16 w){_dimensions.x = w;}
	void setWindowHeight(U16 h){_dimensions.y = h;}

   F32 moveFB,moveLR,angleUD,angleLR;
   void LoadControls();
   //rendering functions
   void Initialize(); //Set up the rendering platform
   static void DrawSceneStatic();
   void DrawScene();

   static void Idle();
   void Quit();
   I8   getMainWindowId(){return mainWindowId;}
   void setMainWindowId(U8 id){mainWindowId = id;}
 
   SINGLETON_END()

#endif