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

#ifndef GLSL_H_
#define GLSL_H_

#include "Hardware/Video/ShaderHandler.h"

class glShader : public Shader {

public:
	glShader() : Shader(), _loaded(false), _bound(false) {};
	glShader(const char *vsFile, const char *fsFile);
	~glShader();
	
    void init(const std::string& vsFile, const std::string &fsFile);
	bool load(const std::string& name);
	bool unload(){unbind(); return true;}
	void bind();
	void unbind();
	
	U16 getId();
	//Attributes
	void Attribute(const std::string& ext, D32 value);
	void Attribute(const std::string& ext, F32 value);
	void Attribute(const std::string& ext, const vec2& value);
	void Attribute(const std::string& ext, const vec3& value);
	void Attribute(const std::string& ext, const vec4& value);
	//Uniforms
	void Uniform(const std::string& ext, I32 value);
	void Uniform(const std::string& ext, F32 value);
	void Uniform(const std::string& ext, const vec2& value);
	void Uniform(const std::string& ext, const vec3& value);
	void Uniform(const std::string& ext, const vec4& value);
	void Uniform(const std::string& ext, const mat3& value);
    void Uniform(const std::string& ext, const mat4& value);
	void Uniform(const std::string& ext, const std::vector<mat4>& values);
	//Uniform Texture
	void UniformTexture(const std::string& ext, U16 slot);

	//Legacy
	void Uniform(I32 location, const vec4& value);

private:

	bool _loaded;
	bool _bound;

private:

	char* shaderFileRead(const std::string &fn);
	I8   shaderFileWrite(char *fn, char *s);
	void  validateShader(U16 shader, const std::string &file = 0);
	void  validateProgram(U16 program);

	bool loadVertOnly(const std::string& name);
	bool loadFragOnly(const std::string& name);
};


#endif
