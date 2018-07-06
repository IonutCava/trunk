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

#ifndef _SHADER_HANDLER_H_
#define _SHADER_HANDLER_H_

#include "Core/Resources/Headers/HardwareResource.h"

class Shader;
enum ShaderType;
class ShaderProgram : public HardwareResource {

public:

	virtual void bind();
	virtual void unbind();
	
	inline U32 getId() { return _shaderProgramId; }

	virtual void attachShader(Shader* shader) = 0;
	vectorImpl<Shader* > getShaders(ShaderType type);

	///Attributes
	virtual void Attribute(const std::string& ext, D32 value) = 0;
	virtual void Attribute(const std::string& ext, F32 value) = 0 ;
	virtual void Attribute(const std::string& ext, const vec2<F32>& value) = 0;
	virtual void Attribute(const std::string& ext, const vec3<F32>& value) = 0;
	virtual void Attribute(const std::string& ext, const vec4<F32>& value) = 0;
	///Uniforms
	virtual void Uniform(const std::string& ext, I32 value) = 0;
	virtual void Uniform(const std::string& ext, F32 value) = 0 ;
	virtual void Uniform(const std::string& ext, const vec2<F32>& value) = 0;
	virtual void Uniform(const std::string& ext, const vec2<I32>& value) = 0;
	virtual void Uniform(const std::string& ext, const vec2<U16>& value) = 0;
	virtual void Uniform(const std::string& ext, const vec3<F32>& value) = 0;
	virtual void Uniform(const std::string& ext, const vec4<F32>& value) = 0;
	virtual void Uniform(const std::string& ext, const mat3<F32>& value, bool rowMajor = false) = 0;
	virtual void Uniform(const std::string& ext, const mat4<F32>& value, bool rowMajor = false) = 0;
	virtual void Uniform(const std::string& ext, const vectorImpl<I32 >& values) = 0;
	virtual void Uniform(const std::string& ext, const vectorImpl<F32 >& values) = 0;
	virtual void Uniform(const std::string& ext, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) = 0;
	
	///Uniform Texture
	virtual void UniformTexture(const std::string& ext, U16 slot) = 0;

	virtual I32 getAttributeLocation(const std::string& name) = 0;
	virtual I32 getUniformLocation(const std::string& name) = 0;

	virtual ~ShaderProgram();

	inline void commit() {if(!_compiled) {link(); validate();}}

	inline bool isBound() {return _bound;}

	inline void setDefinesList(const std::string& definesList) {_definesList = definesList;}

protected:
	ShaderProgram() : HardwareResource(), _compiled(false), _shaderProgramId(-1), _maxCombinedTextureUnits(0) {}

	virtual void validate() = 0;
	virtual void link() = 0;
	template<typename T>
	friend class ImplResourceLoader;
	virtual bool generateHWResource(const std::string& name);
	static  bool checkBinding(U32 newShaderProgramId);

protected:
	bool _compiled;
	bool _bound;
	I32  _maxCombinedTextureUnits;
	U32 _shaderProgramId;
	///A list of preprocessor defines,comma separated, if needed
	std::string _definesList;
	vectorImpl<Shader* > _shaders;
	static U32 _prevShaderProgramId;
	static U32 _newShaderProgramId;
};


#endif
