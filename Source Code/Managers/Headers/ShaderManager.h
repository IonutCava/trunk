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

#ifndef SHADER_MANAGER_H_
#define SHADER_MANAGER_H_

#include "Core/Headers/Singleton.h"
#include "Utility/Headers/Vector.h"
#include "Utility/Headers/UnorderedMap.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <string>
#include <stack>

class Shader;
class ShaderProgram;
enum  ShaderType;
enum  MATRIX_MODE;

DEFINE_SINGLETON(ShaderManager)

typedef Unordered_map<std::string, Shader* >        ShaderMap;
typedef Unordered_map<std::string, ShaderProgram* > ShaderProgramMap;
typedef Unordered_map<std::string, const char* >    AtomMap;
typedef std::stack<ShaderProgram*, vectorImpl<ShaderProgram* > > ShaderQueue;

public:
	///Create rendering API specific initialization of shader libraries
    bool    init();
	///Called once per frame
	///deltaTime = elapsed time in milliseconds
	U8      tick(const U32 deltaTime);
	///Called once per frame after a swap buffer request
	U8      idle();
	///Remove a shader from the cache
	void    removeShader(Shader* s);
	///Find a shader in a cache
	Shader* findShader(const std::string& name, const bool recompile = false );
	///Add or refresh a shader from the cache
	Shader* loadShader(const std::string& name, const std::string& location,const ShaderType& type,const bool recompile = false);
	///Remove a shaderProgram from the program cache
	void    unregisterShaderProgram(const std::string& name);
	///Add a shaderProgram to the program cache
    void    registerShaderProgram(const std::string& name, ShaderProgram* const shaderProgram);
	///Queue a shaderProgram recompile request
	bool    recompileShaderProgram(const std::string& name);
	///Load a shader from file
	char*   shaderFileRead(const std::string &atomName, const std::string& location);
	///Save a shader to file
	I8      shaderFileWrite(char *atomName, const char *s);
	///Bind the null shader
	bool    unbind();

private:
	///Shader cache
	ShaderMap        _shaderNameMap;
	///Shader program cache
    ShaderProgramMap _shaderPrograms;
	///Only 1 shader program per frame should be recompiled to avoid a lot of stuttering
	ShaderQueue      _recompileQueue;
	///Shaders loaded from files are kept as atoms
	AtomMap          _atoms;
	///Pointer to a shader that we will perform operations on
	ShaderProgram*   _nullShader;
	///A simple check to see if the manager is ready to process commands
    bool             _init;

private:
	ShaderManager();
    ~ShaderManager();

END_SINGLETON
#endif