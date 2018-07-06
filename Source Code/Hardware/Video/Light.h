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

#ifndef DIVIDE_LIGHT_H_
#define DIVIDE_LIGHT_H_

#include "resource.h"
#include "EngineGraphs/SceneNode.h"

class Sphere3D;
class Light : public SceneNode{

public:
	Light(U8 slot, F32 radius = 1.0f); 
	~Light();
	void update();
	void setLightProperties(const std::string& name, vec4 values);
	vec4& getDiffuseColor() {return _lightProperties["diffuse"];}
	vec4& getPosition() {return  _lightProperties["position"];}
	F32&  getRadius()   {return _radius;}
	Sphere3D* getImpostor() {return _light;}
	void      toggleImpostor(bool state) {_drawImpostor = state;}
	void  getWindowRect(U16 & x, U16 & y, U16 & width, U16 & height);
	void  render();
	bool load(const std::string& name);
	bool unload();
private:
	std::tr1::unordered_map<std::string,vec4> _lightProperties;
	U8 _slot;
	F32 _radius;
	Sphere3D *_light; //Used for debug rendering -Ionut
	bool _drawImpostor;
};

#endif