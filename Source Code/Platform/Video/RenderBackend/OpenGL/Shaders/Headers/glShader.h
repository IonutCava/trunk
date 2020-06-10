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

namespace Divide {

/// glShader represents one of a program's rendering stages (vertex, geometry, fragment, etc)
/// It can be used simultaneously in multiple programs/pipelines
class glShader final : public GUIDWrapper, public GraphicsResource, public glObject {
   public:
    using ShaderMap = ska::bytell_hash_map<U64, glShader*>;

    // one per shader type!
    struct LoadData {
        ShaderType _type = ShaderType::COUNT;
        Str128 _name = "";
        eastl::unordered_set<U64> atoms;
        vectorEASTL<stringImpl> sourceCode;
    };

    using ShaderLoadData = eastl::array<LoadData, to_base(ShaderType::COUNT)>;

    template<typename T>
    struct UniformCache {
        using ShaderVarMap = hashMap<T, GFX::PushConstant>;
        inline void clear() { _shaderVars.clear(); }
        ShaderVarMap _shaderVars;
    };
    using UniformsByNameHash = UniformCache<U64>;

   public:
    /// The shader's name is the period-separated list of properties, type is
    /// the render stage this shader is used for
    explicit glShader(GFXDevice& context, const Str256& name);
    ~glShader();

    bool load(const ShaderLoadData& data);

    U32 getProgramHandle() const noexcept { return _programHandle; }

    /// The shader's name is a period-separated list of strings used to define
    /// the main shader file and the properties to load
    const Str256& name() const noexcept { return _name; }

    bool embedsType(ShaderType type) const;

    void AddRef() noexcept { _refCount.fetch_add(1); }
    /// Returns true if ref count reached 0
    bool SubRef() noexcept { return _refCount.fetch_sub(1) == 1; }
    const size_t GetRef() const { return _refCount.load(); }

   public:
    // ======================= static data ========================= //
    /// Remove a shader from the cache
    static void removeShader(glShader* s);
    /// Return a new shader reference
    static glShader* getShader(const Str256& name);

    /// Add or refresh a shader from the cache
    static glShader* loadShader(GFXDevice& context,
                                const Str256& name,
                                const ShaderLoadData& data);

    static glShader* loadShader(GFXDevice& context,
                                glShader* shader,
                                bool isNew,
                                const ShaderLoadData& data);
   private:
    void UploadPushConstant(const GFX::PushConstant& constant, bool force = false);
    void Uniform(I32 binding, GFX::PushConstantType type, const Byte* const values, const GLsizei byteCount, bool flag) const;

    friend class glShaderProgram;
    void dumpBinary();
    bool loadFromBinary();
    void cacheActiveUniforms();
    void reuploadUniforms();
    I32  cachedValueUpdate(const GFX::PushConstant& constant, bool force = false);
    bool uploadToGPU(bool& previouslyUploaded);

    /// Add a define to the shader. The defined must not have been added previously
    void addShaderDefine(const stringImpl& define, bool appendPrefix);
    /// Remove a define from the shader. The defined must have been added previously
    void removeShaderDefine(const stringImpl& define);

    UseProgramStageMask stageMask() const noexcept { return _stageMask;  }
    const eastl::unordered_set<U64>& usedAtoms() const noexcept { return _usedAtoms; }

    PROPERTY_R(bool, valid, false);
    PROPERTY_R_IW(bool, shouldRecompile, false);

   private:
    using ShaderVarMap = hashMap<U64, I32>;

  private:
    bool _loadedFromBinary;

    GLenum _binaryFormat;
    GLuint _programHandle;

    U8 _stageCount;
    UseProgramStageMask _stageMask;

    Str256 _name;
    eastl::unordered_set<U64> _usedAtoms;
    std::array<vectorEASTL<stringImpl>, to_base(ShaderType::COUNT)> _sourceCode;

    ShaderVarMap _shaderVarLocation;
    UniformsByNameHash _uniformsByNameHash;

    /// A list of preprocessor defines (if the bool in the pair is true, #define is automatically added
    vectorEASTL<std::pair<stringImpl, bool>> _definesList;

    std::atomic_size_t _refCount;

   private:
    /// Shader cache
    static ShaderMap _shaderNameMap;
    static SharedMutex _shaderNameLock;
};

};  // namespace Divide

#endif //_GL_SHADER_H_