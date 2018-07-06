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

#ifndef _SHADER_H_
#define _SHADER_H_

#include "Core/MemoryManagement/Headers/TrackedObject.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"
#include "Utility/Headers/Vector.h"

enum ShaderType {
	FRAGMENT_SHADER,
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	TESSELATION_SHADER
};

class ShaderProgram;
class Shader : public TrackedObject{
public:
	Shader(const std::string& name,const ShaderType& type, const bool optimise = false);
	virtual ~Shader();

	virtual bool load(const std::string& name) = 0;
	virtual bool compile() = 0; //<from text source to GPU code
	inline       U32          getShaderId() const {return _shader;}
	inline const ShaderType   getType()     const {return _type;}
	inline const std::string& getName()     const {return _name;}
		   void  addParentProgram(ShaderProgram* const shaderProgram);
		   void  removeParentProgram(ShaderProgram* const shaderProgram);
protected:
	virtual void validate() = 0;

protected:
	vectorImpl<ShaderProgram* > _parentShaderPrograms;
	std::string _name;
	U32 _shader;//<not thread-safe. Make sure assignment is protected with a mutex or something
	boost::atomic_bool _compiled;
    bool _optimise;//< Use a pre-compile optimisation parser
	ShaderType _type;
};

#endif