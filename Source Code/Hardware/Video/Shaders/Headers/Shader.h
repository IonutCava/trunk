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

#include "core.h"
#include "Core/MemoryManagement/Headers/TrackedObject.h"

enum ShaderType {
	FRAGMENT_SHADER,
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	TESSELATION_SHADER
};

class Shader : public TrackedObject{
public:
	Shader(const std::string& name, ShaderType type);
	virtual ~Shader();

	virtual bool load(const std::string& name) = 0;

	inline U16          getShaderId() {return _shader;}
	inline ShaderType  getType()     {return _type;}
	inline std::string& getName()     {return _name;}

protected:
	virtual void validate() = 0;

protected:
	std::string _name;
	bool _compiled;
	U16 _shader;
	ShaderType _type;
};

#endif