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

#ifndef _FLASH_SCENE_H
#define _FLASH_SCENE_H

#include "Scenes/Headers/Scene.h"

class FlashScene : public Scene {

public:
	FlashScene() : Scene() {}
	void render();
	void preRender();
	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	void processInput();
	void processEvents(F32 time);

private:
	F32 i ;
	vec2<F32> _sunAngle;
	vec3<F32> _sunvector;
	F32 update_time;
	vec4<F32> _vSunColor;
	vectorImpl<F32> _eventTimers;

};

#endif