/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef SHADER_MANAGER_H_
#define SHADER_MANAGER_H_

#include "Core/Headers/Singleton.h"
#include "Utility/Headers/Vector.h"
#include "Utility/Headers/UnorderedMap.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include <string>
#include <stack>

class Kernel;
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
    bool    init(Kernel* const activeKernel);
    void    Destroy();
    ///Called once per frame
    U8      update(const U64 deltaTime);
    ///Called once per frame after a swap buffer request
    U8      idle();
    ///Calling refresh will mark all shader programs as dirty
    void    refresh();
    ///Calling refreshSceneData will mark all shader programs as dirty(scene specific data only)
    void    refreshSceneData();
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
    ///Return a default shader if we try to render something with a material that is missing a valid shader
    ShaderProgram* const getDefaultShader() const {return _imShader;}

    void setMatricesDirty();

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
    ShaderProgram* _nullShader;
    ///Used to render geometry without valid materials.
    ///Should emmulate the basic fixed pipeline functions (no lights, just color and texture)
    ShaderProgram* _imShader;
    ///A simple check to see if the manager is ready to process commands
    bool             _init;
    ///A pointer to the active kernel (for simplicity)
    Kernel*          _activeKernel;

private:
    ShaderManager();
    ~ShaderManager();

END_SINGLETON
#endif