/*
   Copyright (c) 2015 DIVIDE-Studio
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

#include "Platform/DataTypes/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include <stack>

namespace Divide {

class Kernel;
class Shader;
class ShaderProgram;
enum  ShaderType : I32;

DEFINE_SINGLETON(ShaderManager)

typedef hashMapImpl<stringImpl, Shader* >        ShaderMap;
typedef hashMapImpl<stringImpl, ShaderProgram* > ShaderProgramMap;
typedef hashMapImpl<stringImpl, stringImpl >     AtomMap;
typedef std::stack<ShaderProgram*, vectorImpl<ShaderProgram* > > ShaderQueue;

private:
    ShaderManager();
    ~ShaderManager();

public:
    /// Create rendering API specific initialization of shader libraries
    bool init();
    /// Remove default shaders and and destroy API specific shader loading code (programs can still be unloaded, but not loaded)
    void destroy();
    /// Called once per frame
    bool update(const U64 deltaTime);
    /// Called once per frame after a swap buffer request
    void idle();
    /// Calling refreshShaderData will mark all shader programs as dirty (general data)
    void refreshShaderData();
    /// Calling refreshSceneData will mark all shader programs as dirty (scene specific data only)
    void refreshSceneData();
    /// Remove a shader from the cache
    void removeShader(Shader* s);
    /// Return a new shader reference
    Shader* getShader(const stringImpl& name, const bool recompile = false );
    /// Add or refresh a shader from the cache
    Shader* loadShader(const stringImpl& name, const stringImpl& location,const ShaderType& type,const bool recompile = false);
    /// Remove a shaderProgram from the program cache
    void unregisterShaderProgram(const stringImpl& name);
    /// Add a shaderProgram to the program cache
    void registerShaderProgram(const stringImpl& name, ShaderProgram* const shaderProgram);
    /// Queue a shaderProgram recompile request
    bool recompileShaderProgram(const stringImpl& name);
    /// Load a shader from file
    const stringImpl& shaderFileRead(const stringImpl &atomName, const stringImpl& location);
    /// Save a shader to file
    I8 shaderFileWrite(char *atomName, const char *s);
    /// Bind the null shader
    bool unbind();
    /// Return a default shader if we try to render something with a material that is missing a valid shader
    ShaderProgram* const getDefaultShader() const {return _imShader;}

private:
    /// A simple check to see if the manager is ready to process commands
    bool _init;
    /// Shaders loaded from files are kept as atoms
    AtomMap _atoms;
    /// Shader cache
    ShaderMap _shaderNameMap;
    /// Pointer to a shader that we will perform operations on
    ShaderProgram* _nullShader;
    /// Only 1 shader program per frame should be recompiled to avoid a lot of stuttering
    ShaderQueue _recompileQueue;
    /// Shader program cache
    ShaderProgramMap _shaderPrograms;
    /// Used to render geometry without valid materials.
    /// Should emulate the basic fixed pipeline functions (no lights, just color and texture)
    ShaderProgram* _imShader;

END_SINGLETON

};
#endif