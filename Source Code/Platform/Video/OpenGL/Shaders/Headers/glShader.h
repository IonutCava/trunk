/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef GL_H_
#define GL_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Headers/GraphicsResource.h"
#include "Core/MemoryManagement/Headers/TrackedObject.h"

namespace Divide {

/// glShader represents one of a program's rendering stages (vertex, geometry, fragment, etc)
/// It can be used simultaneously in multiple programs/pipelines
class glShader : protected GraphicsResource, public TrackedObject {
    USE_CUSTOM_ALLOCATOR
   public:
    typedef hashMapImpl<ULL, glShader*> ShaderMap;

    static const char* CACHE_LOCATION_TEXT;
    static const char* CACHE_LOCATION_BIN;

   public:
    /// The shader's name is the period-separated list of properties, type is
    /// the render stage this shader is used for
    glShader(GFXDevice& context,
             const stringImpl& name,
             const ShaderType& type,
             const bool optimise = false);
    ~glShader();

    bool load(const stringImpl& source);
    bool compile();
    bool validate();

    /// Shader's API specific handle
    inline U32 getShaderID() const { return _shader; }
    /// The pipeline stage this shader is used for
    inline const ShaderType getType() const { return _type; }
    /// The shader's name is a period-separated list of strings used to define
    /// the main shader file and the properties to load
    inline const stringImpl& getName() const { return _name; }

   public:
    // ======================= static data ========================= //
    /// Remove a shader from the cache
    static void removeShader(glShader* s);
    /// Return a new shader reference
    static glShader* getShader(const stringImpl& name);
    /// Add or refresh a shader from the cache
    static glShader* loadShader(const stringImpl& name,
                                const stringImpl& location,
                                const ShaderType& type,
                                const bool parseCode);
   private:
    stringImpl preprocessIncludes(const stringImpl& source,
                                  const stringImpl& filename,
                                  GLint level /*= 0 */);

    inline void skipIncludes(bool state) {
        _skipIncludes = state;
    }

   private:
    stringImpl _name;
    ShaderType _type;
    bool _skipIncludes;
    /// The API dependent object handle. Not thread-safe!
    U32 _shader;
    std::atomic_bool _compiled;

    //extra entry for "common" location
    static stringImpl shaderAtomLocationPrefix[to_const_uint(ShaderType::COUNT) + 1];
    
   private:
    /// Shader cache
    static ShaderMap _shaderNameMap;
    static SharedLock _shaderNameLock;
};

};  // namespace Divide

#endif