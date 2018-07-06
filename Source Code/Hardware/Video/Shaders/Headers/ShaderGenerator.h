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

#ifndef SHADER_GENERATOR_H_
#define SHADER_GENERATOR_H_

#include "core.h"
struct ShaderStageDescriptor{
	enum SHADER_STAGE{
		SHADER_NUM_LIGHTS = 0,
		SHADER_PHONG = 1, ///Blin <-> Phong interchange
		SHADER_BLIN = 2,
		SHADER_BUMP = 3,
		SHADER_PARALLAX = 4,
		SHADER_SHADOW = 5, ///Shadow <-> Smooth Shadow interchange
		SHADER_SMOOTH_SHADOW = 6,
		SHADER_SPECULAR_MAP = 7,
		SHADER_OPACITY_MAP = 8,
		SHADER_FOG = 9
	} _stage;

	U8  _var;
	bool _bypassDetailLevel;


};

class Shader;
class ShaderGenerator {
public:
	ShaderGenerator();
	~ShaderGenerator();
	Shader* generateShader(const std::vector<ShaderStageDescriptor> & stages);
private:
	unordered_map<std::string, Shader*> _shaderCache;
};

#endif