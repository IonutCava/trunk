/*
   Copyright (c) 2013 DIVIDE-Studio
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