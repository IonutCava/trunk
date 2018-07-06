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

#ifndef SHADER_MANAGER_H_
#define SHADER_MANAGER_H_
#include "resource.h"

class Shader;
class ShaderProgram;
DEFINE_SINGLETON(ShaderManager)
public:
	void removeShader(Shader* s);
	Shader* loadShader(const std::string& name, const std::string& location);
	bool    unbind();

private:
	unordered_map<std::string, Shader* > _shaders;
	ShaderProgram* _nullShader;

private:
	ShaderManager();

END_SINGLETON
#endif