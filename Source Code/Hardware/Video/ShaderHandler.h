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

#ifndef _SHADER_HANDLER_H_
#define _SHADER_HANDLER_H_

#include "Utility/Headers/BaseClasses.h"

class Shader : public Resource{

public:
    virtual void init(const std::string &vsFile, const std::string &fsFile) = 0;
	virtual bool load(const std::string& name);
    virtual bool loadVertOnly(const std::string& name) = 0;
	virtual bool loadFragOnly(const std::string& name) = 0;

	virtual void bind();
	virtual void unbind() = 0;
	
	virtual U16 getId() = 0;

	virtual void Uniform(const std::string& ext, I32 value) = 0;
	virtual void Uniform(const std::string& ext, F32 value) = 0 ;
	virtual void Uniform(const std::string& ext, const vec2& value) = 0;
	virtual void Uniform(const std::string& ext, const vec3& value) = 0;
	virtual void Uniform(const std::string& ext, const vec4& value) = 0;
	virtual void Uniform(const std::string& ext, const mat3& value) = 0;
	virtual void Uniform(const std::string& ext, const mat4& value) = 0;
	virtual void Uniform(const std::string& ext, const std::vector<mat4>& values) = 0;
	virtual void UniformTexture(const std::string& ext, U16 slot) = 0;

	//Legacy
	virtual void Uniform(I32 location, const vec4& value) = 0;
	virtual ~Shader(){}
	inline const std::string& getFragName() {return _fragName;}
	inline const std::string& getVertName() {return _vertName;}
	virtual void createCopy() {incRefCount();}
	virtual void removeCopy() {decRefCount();}

protected:
	virtual char* shaderFileRead(const std::string &fn) = 0;
	virtual I8   shaderFileWrite(char *fn, char *s) = 0;
	Shader() : Resource() {}
protected:
	U32 _shaderId;
	U32 _shaderVP;
	U32 _shaderFP;
	std::string _fragName, _vertName;

};


#endif
