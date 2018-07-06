/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SHADER_H_
#define _SHADER_H_

#include "Core/MemoryManagement/Headers/TrackedObject.h"
#include "Platform/Platform/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

class ShaderProgram;
/// Shader represents one of a program's rendering stages (vertex, geometry,
/// fragment, etc)
/// It can be used simultaneously in multiple programs/pipelines
class NOINITVTABLE Shader : public TrackedObject {
   public:
    /// The shader's name is the period-separated list of properties, type is
    /// the render stage this shader is used for
    Shader(const stringImpl& name, const ShaderType& type,
           const bool optimise = false);
    /// The shader is deleted only by the ShaderManager when no shader programs
    /// are referencing it
    virtual ~Shader();

    /// Shader's API specific handle
    inline U32 getShaderID() const { return _shader; }
    /// The pipeline stage this shader is used for
    inline const ShaderType getType() const { return _type; }
    /// The shader's name is a period-separated list of strings used to define
    /// the main shader file and the properties to load
    inline const stringImpl& getName() const { return _name; }

    /// API dependent loading
    virtual bool load(const stringImpl& name) = 0;
    /// API conversion from text source to binary
    virtual bool compile() = 0;
    /// API dependent validation
    virtual void validate() = 0;

   protected:
    stringImpl _name;
    ShaderType _type;
    /// Use a pre-compile optimisation parser
    bool _optimise;
    /// The API dependent object handle. Not thread-safe!
    U32 _shader;
    std::atomic_bool _compiled;
};

};  // namespace Divide
#endif