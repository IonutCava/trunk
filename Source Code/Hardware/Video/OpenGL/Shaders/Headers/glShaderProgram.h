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

#ifndef GLSL_H_
#define GLSL_H_

#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"

enum  UBO_NAME;
class glUniformBufferObject;
class glShaderProgram : public ShaderProgram {
public:
	glShaderProgram(const bool optimise = false);
	~glShaderProgram();

	bool unload(){unbind(); return true;}
	void bind();
	void unbind(bool resetActiveProgram = true);
	U8   tick(const U32 deltaTime);
	void attachShader(Shader* const shader,const bool refresh = false);
	void detachShader(Shader* const shader);
	//Attributes
	void Attribute(const std::string& ext, GLdouble value);
	void Attribute(const std::string& ext, GLfloat value);
	void Attribute(const std::string& ext, const vec2<GLfloat>& value);
	void Attribute(const std::string& ext, const vec3<GLfloat>& value);
	void Attribute(const std::string& ext, const vec4<GLfloat>& value);
	//Uniforms
    void Uniform(const std::string& ext, GLuint value);
	void Uniform(const std::string& ext, GLint value);
	void Uniform(const std::string& ext, GLfloat value);
	void Uniform(const std::string& ext, const vec2<GLfloat>& value);
	void Uniform(const std::string& ext, const vec2<GLint>& value);
	void Uniform(const std::string& ext, const vec2<GLushort>& value);
	void Uniform(const std::string& ext, const vec3<GLfloat>& value);
	void Uniform(const std::string& ext, const vec4<GLfloat>& value);
	void Uniform(const std::string& ext, const mat3<GLfloat>& value, bool rowMajor = false);
    void Uniform(const std::string& ext, const mat4<GLfloat>& value, bool rowMajor = false);
	void Uniform(const std::string& ext, const vectorImpl<GLint >& values);
	void Uniform(const std::string& ext, const vectorImpl<GLfloat >& values);
	void Uniform(const std::string& ext, const vectorImpl<mat4<GLfloat> >& values, bool rowMajor = false);
	//Uniform Texture
	void UniformTexture(const std::string& ext, GLushort slot);

	inline GLint getAttributeLocation(const std::string& name) { return cachedLoc(name,false); }
	inline GLint getUniformLocation(const std::string& name)   { return cachedLoc(name,true);  }
	inline void  flushLocCache()                               { _shaderVars.clear();}

private:
	void threadedLoad(const std::string& name);
	GLint cachedLoc(const std::string& name, const bool uniform = true);
	void validateInternal();

private:
	typedef Unordered_map<std::string, GLint > ShaderVarMap;
	ShaderVarMap _shaderVars;
	vectorImpl<GLint> _UBOLocation;
	vectorImpl<glUniformBufferObject* > _uniformBufferObjects;
	boost::atomic_bool _validationQueued;
	U32 _shaderProgramIdTemp;
protected:
	bool generateHWResource(const std::string& name);
	inline void validate() {_validationQueued = true;}
	void link();

protected:
	friend class glUniformBufferObject;
	inline GLuint getUBOLocation(const UBO_NAME& ubo) {assert((GLuint)ubo < _UBOLocation.size()); return _UBOLocation[ubo];}
	void initUBO();
};

#endif
