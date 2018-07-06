/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef SHADER_GENERATOR_H_
#define SHADER_GENERATOR_H_

#include "core.h"
struct ShaderStageDescriptor{
	enum ShaderStage{
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
	Shader* generateShader(const vectorImpl<ShaderStageDescriptor> & stages);
private:
	Unordered_map<std::string, Shader*> _shaderCache;
};

#endif