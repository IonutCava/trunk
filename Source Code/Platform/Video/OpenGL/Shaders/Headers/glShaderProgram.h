/*
   Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _PLATFORM_VIDEO_OPENGLS_PROGRAM_H_
#define _PLATFORM_VIDEO_OPENGLS_PROGRAM_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {
class GL_API;
class glShader;
class glLockManager;
namespace Attorney {
    class GLAPIShaderProgram;
};
/// OpenGL implementation of the ShaderProgram entity
class glShaderProgram final : public ShaderProgram {
    USE_CUSTOM_ALLOCATOR
    friend class Attorney::GLAPIShaderProgram;
   public:
       struct glShaderProgramLoadInfo {
           stringImpl _resourcePath;
           stringImpl _header;
           stringImpl _programName;
           stringImpl _programProperties;
           stringImpl _vertexStageProperties;
       };

  private:
    template<typename T>
    struct UniformCache {
        typedef hashMapImpl<T, U32> ShaderVarU32Map;
        typedef hashMapImpl<T, I32> ShaderVarI32Map;
        typedef hashMapImpl<T, F32> ShaderVarF32Map;
        typedef hashMapImpl<T, vec2<F32>> ShaderVarVec2F32Map;
        typedef hashMapImpl<T, vec2<I32>> ShaderVarvec2I32Map;
        typedef hashMapImpl<T, vec3<F32>> ShaderVarVec3F32Map;
        typedef hashMapImpl<T, vec3<I32>> ShaderVarVec3I32Map;
        typedef hashMapImpl<T, vec4<F32>> ShaderVarVec4F32Map;
        typedef hashMapImpl<T, vec4<I32>> ShaderVarVec4I32Map;
        typedef hashMapImpl<T, mat3<F32>> ShaderVarMat3Map;
        typedef hashMapImpl<T, mat4<F32>> ShaderVarMat4Map;
        typedef hashMapImpl<T, vectorImpl<I32>> ShaderVarVectorI32Map;
        typedef hashMapImpl<T, vectorImpl<F32>> ShaderVarVectorF32Map;
        typedef hashMapImpl<T, vectorImpl<vec2<F32>>> ShaderVarVectorVec2F32Map;
        typedef hashMapImpl<T, vectorImpl<vec3<F32>>> ShaderVarVectorVec3F32Map;
        typedef hashMapImpl<T, vectorImpl<vec4<F32>>> ShaderVarVectorVec4F32Map;
        typedef hashMapImpl<T, vectorImpl<mat3<F32>>> ShaderVarVectorMat3Map;
        typedef hashMapImpl<T, vectorImpl<mat4<F32>>> ShaderVarVectorMat4Map;

        void clear() {
            _shaderVarsU32.clear();
            _shaderVarsI32.clear();
            _shaderVarsF32.clear();
            _shaderVarsVec2F32.clear();
            _shaderVarsVec2I32.clear();
            _shaderVarsVec3F32.clear();
            _shaderVarsVec4F32.clear();
            _shaderVarsMat3.clear();
            _shaderVarsMat4.clear();
            _shaderVarsVectorI32.clear();
            _shaderVarsVectorF32.clear();
            _shaderVarsVectorVec2F32.clear();
            _shaderVarsVectorVec3F32.clear();
            _shaderVarsVectorVec4F32.clear();
            _shaderVarsVectorMat3.clear();
            _shaderVarsVectorMat4.clear();
        }

        ShaderVarU32Map _shaderVarsU32;
        ShaderVarI32Map _shaderVarsI32;
        ShaderVarF32Map _shaderVarsF32;
        ShaderVarVec2F32Map _shaderVarsVec2F32;
        ShaderVarvec2I32Map _shaderVarsVec2I32;
        ShaderVarVec3F32Map _shaderVarsVec3F32;
        ShaderVarVec3I32Map _shaderVarsVec3I32;
        ShaderVarVec4F32Map _shaderVarsVec4F32;
        ShaderVarVec4I32Map _shaderVarsVec4I32;
        ShaderVarMat3Map _shaderVarsMat3;
        ShaderVarMat4Map _shaderVarsMat4;
        ShaderVarVectorI32Map _shaderVarsVectorI32;
        ShaderVarVectorF32Map _shaderVarsVectorF32;
        ShaderVarVectorVec2F32Map _shaderVarsVectorVec2F32;
        ShaderVarVectorVec3F32Map _shaderVarsVectorVec3F32;
        ShaderVarVectorVec4F32Map _shaderVarsVectorVec4F32;
        ShaderVarVectorMat3Map _shaderVarsVectorMat3;
        ShaderVarVectorMat4Map _shaderVarsVectorMat4;

    };

    typedef hashMapImpl<U64, I32> ShaderVarMap;
    typedef UniformCache<const char*> UniformsByName;

   public:
    explicit glShaderProgram(GFXDevice& context,
                             size_t descriptorHash,
                             const stringImpl& name,
                             const stringImpl& resourceName,
                             const stringImpl& resourceLocation,
                             bool asyncLoad);
    ~glShaderProgram();

    static void initStaticData();
    static void destroyStaticData();

    /// Make sure this program is ready for deletion
    bool unload() override;
    /// Bind this shader program
    bool bind() override;
    /// Returns true if the shader is currently active
    bool isBound() const override;
    /// Check every possible combination of flags to make sure this program can
    /// be used for rendering
    bool isValid() const override;
    /// Called once per frame. Used to update internal state
    bool update(const U64 deltaTime) override;
    /// This is used to set all of the subroutine indices for the specified
    /// shader stage for this program
    void SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const override;
    /// This works exactly like SetSubroutines, but for a single index.
    void SetSubroutine(ShaderType type, U32 index) const override;
    /// Get the index of the specified subroutine name for the specified stage.
    /// Not cached!
    U32 GetSubroutineIndex(ShaderType type, const char* name) const override;
    /// Get the uniform location of the specified subroutine uniform for the
    /// specified stage. Not cached!
    U32 GetSubroutineUniformLocation(ShaderType type, const char* name) const override;
    U32 GetSubroutineUniformCount(ShaderType type) const override;
    /// Set an uniform value
    void Uniform(const char* location, U32 value) override;
    void Uniform(const char* location, I32 value) override;
    void Uniform(const char* location, F32 value) override;
    void Uniform(const char* location, const vec2<F32>& value) override;
    void Uniform(const char* location, const vec2<I32>& value) override;
    void Uniform(const char* location, const vec3<F32>& value) override;
    void Uniform(const char* location, const vec3<I32>& value) override;
    void Uniform(const char* location, const vec4<F32>& value) override;
    void Uniform(const char* location, const vec4<I32>& value) override;
    void Uniform(const char* location, const mat3<F32>& value, bool transpose = false) override;
    void Uniform(const char* location, const mat4<F32>& value, bool transpose = false) override;
    void Uniform(const char* location, const vectorImpl<I32>& values) override;
    void Uniform(const char* location, const vectorImpl<F32>& values) override;
    void Uniform(const char* location, const vectorImpl<vec2<F32>>& values) override;
    void Uniform(const char* location, const vectorImpl<vec3<F32>>& values) override;
    void Uniform(const char* location, const vectorImplBest<vec4<F32>>& values) override;
    void Uniform(const char* location, const vectorImpl<mat3<F32>>& values, bool transpose = false) override;
    void Uniform(const char* location, const vectorImplBest<mat4<F32>>& values, bool transpose = false) override;

    void DispatchCompute(U32 xGroups, U32 yGroups, U32 zGroups) override;

    void SetMemoryBarrier(MemoryBarrierType type) override;

   protected:
    glShaderProgramLoadInfo buildLoadInfo();
    std::pair<bool, stringImpl> loadSourceCode(ShaderType stage,
                                               const stringImpl& stageName,
                                               const stringImpl& header,
                                               bool forceReParse);
    bool loadFromBinary();
       
    bool recompileInternal() override;
    /// Creation of a new shader program. Pass in a shader token and use glsw to
    /// load the corresponding effects
    bool load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) override;
    /// Linking a shader program also sets up all pre-link properties for the
    /// shader (varying locations, attrib bindings, etc)
    void link();
    /// This should be called in the loading thread, but some issues are still
    /// present, and it's not recommended (yet)
    void threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback, bool skipRegister);
    /// Cache uniform/attribute locations for shader programs
    I32 getUniformLocation(const char* name);

    struct fake_dependency: public std::false_type {};
    template <typename T>
    I32 cachedValueUpdate(const char* location, const T& value) {
        static_assert(
                fake_dependency::value,
            "glShaderProgram::cachedValue error: unsupported data type!");
        return -1;
    }
    /// Basic OpenGL shader program validation (both in debug and in release)
    bool validateInternal();
    /// Retrieve the program's validation log if we need it
    stringImpl getLog() const;

    /// Add a new shader stage to this program
    void attachShader(glShader* const shader);
    /// Remove a shader stage from this program
    void detachShader(glShader* const shader);

    void reloadShaders(bool reparseShaderSource);

    void reuploadUniforms();

   private:

    ShaderVarMap _shaderVarLocation;
    UniformsByName _uniformsByName;
    
    bool _validationQueued;
    GLenum _binaryFormat;
    bool _validated;
    bool _loadedFromBinary;
    GLuint _shaderProgramIDTemp;
    static std::array<U32, to_base(ShaderType::COUNT)> _lineOffset;
    std::array<glShader*, to_base(ShaderType::COUNT)> _shaderStage;

    glLockManager* _lockManager;
};

namespace Attorney {
    class GLAPIShaderProgram {
    private:
        static void setGlobalLineOffset(U32 offset) {
            glShaderProgram::_lineOffset.fill(offset);
        }
        static void addLineOffset(ShaderType stage, U32 offset) {
            glShaderProgram::_lineOffset[to_U32(stage)] += offset;
        }

        friend class Divide::GL_API;
    };
};  // namespace Attorney
};  // namespace Divide

#endif  //_PLATFORM_VIDEO_OPENGLS_PROGRAM_H_

#include "glShaderProgram.inl"
