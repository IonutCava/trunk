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

#ifndef _NETWORK_SCENE_H
#define _NETWORK_SCENE_H

#include "Scenes/Headers/Scene.h"

class NetworkScene : public Scene {

public:
	NetworkScene() : Scene() {} 
	void render();
	void preRender();

	bool preLoad();
	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool unload();

	void processEvents(F32 time);
	void processInput();

private: 
	void test();
	void connect();
	void disconnect();
	void checkPatches();

private:
	vec2<F32> _sunAngle;
	vec4<F32> _sunVector;
	std::vector<F32> _eventTimers;
};

#endif