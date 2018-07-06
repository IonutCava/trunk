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

#include "resource.h"
#ifndef _LIGHT_MANAGER_H_
#define _LIGHT_MANAGER_H_

class Light;
DEFINE_SINGLETON(LightManager)

public:
	typedef unordered_map<U32, Light*> LightMap;

	bool addLight(Light* const light);
	bool removeLight(U32 lightId);
	void setAmbientLight(const vec4& light);

	bool clear();
	U32  generateNewID();
	void update(bool force = false);

	inline LightMap& getLights()      {return _lights;}
	inline Light*    getLight(U32 id) {return _lights[id];}

	///shadow mapping
	void bindDepthMaps();
	void unbindDepthMaps();
	bool shadowMappingEnabled();
	void generateShadowMaps();
	///since every light has the exact same number of depth maps ...
	I8 getDepthMapPerLightCount();
	///shadow mapping
	void previewDepthMaps();
private:
	LightManager();
	~LightManager();
	bool checkId(U32 value);
	void drawDepthMap(U8 light, U8 index);
private:
	LightMap _lights;

END_SINGLETON

#endif