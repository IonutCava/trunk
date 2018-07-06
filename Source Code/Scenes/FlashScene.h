/*�Copyright 2009-2011 DIVIDE-Studio�*/
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

#include "Scene.h"

class FlashScene : public Scene
{
public:
	void render();
	void preRender();
	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool loadEvents(bool continueOnErrors){return true;}
	void processInput();
	void processEvents(F32 time);

private:
	F32 i ;
	vec2 _sunAngle;
	vec4 _sunVector;
	F32 angleLR,angleUD,moveFB,moveLR,update_time;
	vec4 _vSunColor;
	std::vector<F32> _eventTimers;

};

#endif