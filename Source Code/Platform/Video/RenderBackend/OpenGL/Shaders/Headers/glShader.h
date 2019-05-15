/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _GL_SHADER_H_
#define _GL_SHADER_H_

#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/Headers/GraphicsResource.h"
#include "Core/MemoryManagement/Headers/TrackedObject.h"

namespace Divide {

/// glShader represents one of a program's rendering stages (vertex, geometry, fragment, etc)
/// It can be used simultaneously in multiple programs/pipelines
class glShader : public TrackedObject, public GraphicsResource,  public glObject {
   public:
    typedef hashMap<U64, glShader*> ShaderMap;

    // one per shader type!
    struct LoadData {
        ShaderType _type = ShaderType::COUNT;
        stringImpl _name = "";
        std::set<U64> atoms;
        vectorEASTL<stringImpl> sourceCode;
    };

    typedef std::array<LoadData, to_base(ShaderType::COUNT)> ShaderLoadData;

   public:
    /// The shader's name is the period-separated list of properties, type is
    /// the render stage this shader is used for
    glShader(GFXDevice& context,
             const stringImpl& name,
             const bool deferredUpload);

    ~glShader();

    bool load(const ShaderLoadData& data);

    inline U32 getProgramHandle() const { return _programHandle; }

    /// The shader's name is a period-separated list of strings used to define
    /// the main shader file and the properties to load
    inline const stringImpl& name() const { return _name; }

    bool embedsType(ShaderType type) const;

   public:
    // ======================= static data ========================= //
    /// Remove a shader from the cache
    static void removeShader(glShader* s);
    /// Return a new shader reference
    static glShader* getShader(const stringImpl& name);

    /// Add or refresh a shader from the cache
    static glShader* loadShader(GFXDevice& context,
                                const stringImpl& name,
                                const ShaderLoadData& data,
                                bool deferredUpload);

   private:
     bool loadFromBinary();

   private:
    friend class glShaderProgram;
    void dumpBinary();

    inline const std::set<U64>& usedAtoms() const{
        return _usedAtoms;
    }

    inline bool isValid() const {
        return _valid;
    }

    inline bool shouldRecompile() const {
        return _shouldRecompile;
    }

    /// Cache uniform/attribute locations for shader programs
    I32 binding(const char* name);
    void reuploadUniforms();
    void UploadPushConstant(const GFX::PushConstant& constant);
    I32 cachedValueUpdate(const GFX::PushConstant& constant);
    void Uniform(I32 binding, GFX::PushConstantType type, const vectorEASTL<char>& values, bool flag) const;

    bool uploadToGPU();

    /// Add a define to the shader. The defined must not have been added previously
    void addShaderDefine(const stringImpl& define, bool appendPrefix);
    /// Remove a define from the shader. The defined must have been added previously
    void removeShaderDefine(const stringImpl& define);

    inline UseProgramStageMask stageMask() const { return _stageMask;  }

   private:
    template<typename T>
    struct UniformCache {
        typedef hashMap<T, GFX::PushConstant> ShaderVarMap;
        inline void clear() { _shaderVars.clear(); }
        ShaderVarMap _shaderVars;
    };

    typedef hashMap<U64, I32> ShaderVarMap;
    typedef UniformCache<U64> UniformsByNameHash;

  private:
    bool _valid;
    bool _loadedFromBinary;
    bool _deferredUpload;
    bool _shouldRecompile;

    GLenum _binaryFormat;
    GLuint _programHandle;

    U8 _stageCount;
    UseProgramStageMask _stageMask;

    stringImpl _name;
    std::set<U64> _usedAtoms;
    std::array<vectorEASTL<stringImpl>, to_base(ShaderType::COUNT)> _sourceCode;

    ShaderVarMap _shaderVarLocation;
    UniformsByNameHash _uniformsByNameHash;

    /// A list of preprocessor defines (if the bool in the pair is true, #define is automatically added
    vector<std::pair<stringImpl, bool>> _definesList;

   private:
    /// Shader cache
    static ShaderMap _shaderNameMap;
    static SharedMutex _shaderNameLock;

    template<typename T_out, size_t T_out_count, typename T_in>
    const T_out* castData(const vectorEASTL<char>& values) const {
        static_assert(sizeof(T_out) * T_out_count == sizeof(T_in), "Invalid cast data");
        return reinterpret_cast<const T_out*>(values.data());
    }
};

};  // namespace Divide

#endif //_GL_SHADER_H_