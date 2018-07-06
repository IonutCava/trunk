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

#ifndef _SHADER_H_
#define _SHADER_H_

#include "resource.h"

enum SHADER_TYPE {
	FRAGMENT_SHADER,
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	TESSELATION_SHADER
};

class Shader {
public:
	Shader(const std::string& name, SHADER_TYPE type);
	virtual ~Shader();

	virtual bool load(const std::string& name) = 0;
	U16  getShaderId() {return _shader;}
	void incRefCount() {_refCount++;}
	void decRefCount() {_refCount--;}
	U32  getRefCount() {return _refCount;}
	SHADER_TYPE getType() {return _type;}
	std::string& getName() {return _name;}

protected:
	virtual void validate() = 0;
	char* shaderFileRead(const std::string &fn);
	I8    shaderFileWrite(char *fn, char *s);

protected:
	std::string _name;
	bool _compiled;
	U16 _shader;
	U32 _refCount;
	SHADER_TYPE _type;
};

#endif