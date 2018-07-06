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

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "resource.h"

class SceneManager;
class GUI;
class Terrain;
class Camera;
class GFXDevice;
class SFXDevice;

DEFINE_SINGLETON( Application )

private:
	Application();
	I8 mainWindowId;
	vec2<F32> _dimensions;

	GFXDevice&    _GFX;
	SFXDevice&    _SFX;
	SceneManager& _scene;
	Camera*       _camera;
	GUI&          _gui;
	bool          _previewDepthMaps;
	static bool   _keepAlive;

public:
	const vec2<F32>& getWindowDimensions() const {return _dimensions;}
	inline void setWindowWidth(U16 w){_dimensions.x = w;}
	inline void setWindowHeight(U16 h){_dimensions.y = h;}

	F32 moveFB,moveLR,angleUD,angleLR;

	void Initialize(); ///< Set up the rendering platform

	static void DrawSceneStatic();
	static void Idle();

	inline I8 const&  getMainWindowId() {return mainWindowId;}
	inline void setMainWindowId(U8 id)  {mainWindowId = id;}
	void   togglePreviewDepthMaps() {_previewDepthMaps = !_previewDepthMaps;}

	/// get elapsed time since application start
	inline D32 getCurrentTime() {return _currentTime;}

private:
   bool DrawScene();
   D32 _currentTime;
   END_SINGLETON

#endif