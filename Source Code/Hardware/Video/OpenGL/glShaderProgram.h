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

#ifndef GLSL_H_
#define GLSL_H_

#include "../ShaderProgram.h"

class glShaderProgram : public ShaderProgram {

public:
	glShaderProgram();
	~glShaderProgram();

	bool unload(){unbind(); return true;}
	void bind();
	void unbind();
	void attachShader(Shader* shader);
	//Attributes
	void Attribute(const std::string& ext, D32 value);
	void Attribute(const std::string& ext, F32 value);
	void Attribute(const std::string& ext, const vec2<F32>& value);
	void Attribute(const std::string& ext, const vec3<F32>& value);
	void Attribute(const std::string& ext, const vec4<F32>& value);
	//Uniforms
	void Uniform(const std::string& ext, I32 value);
	void Uniform(const std::string& ext, F32 value);
	void Uniform(const std::string& ext, const vec2<F32>& value);
	void Uniform(const std::string& ext, const vec2<I32>& value);
	void Uniform(const std::string& ext, const vec3<F32>& value);
	void Uniform(const std::string& ext, const vec4<F32>& value);
	void Uniform(const std::string& ext, const mat3<F32>& value, bool rowMajor = false);
    void Uniform(const std::string& ext, const mat4<F32>& value, bool rowMajor = false);
	void Uniform(const std::string& ext, const std::vector<mat4<F32> >& values, bool rowMajor = false);
	//Uniform Texture
	void UniformTexture(const std::string& ext, U16 slot);

	I32 getAttributeLocation(const std::string& name);
	I32 getUniformLocation(const std::string& name);

private:
	I32   cachedLoc(const std::string& name,bool uniform = true);
	bool flushLocCache();

private:
	unordered_map<std::string, I32 > _shaderVars;

protected:
	bool generateHWResource(const std::string& name);
	void validate();
	void link();
};


#endif
