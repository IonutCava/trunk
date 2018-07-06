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
    explicit glShaderProgram(GFXDevice& context,
                             const stringImpl& name,
                             const stringImpl& resourceLocation,
                             bool asyncLoad);
    ~glShaderProgram();

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
    inline void Uniform(const char* ext, U32 value) override;
    inline void Uniform(const char* ext, I32 value) override;
    inline void Uniform(const char* ext, F32 value) override;
    inline void Uniform(const char* ext, const vec2<F32>& value) override;
    inline void Uniform(const char* ext, const vec2<I32>& value) override;
    inline void Uniform(const char* ext, const vec3<F32>& value) override;
    inline void Uniform(const char* ext, const vec3<I32>& value) override;
    inline void Uniform(const char* ext, const vec4<F32>& value) override;
    inline void Uniform(const char* ext, const vec4<I32>& value) override;
    inline void Uniform(const char* ext, const mat3<F32>& value, bool transpose = false) override;
    inline void Uniform(const char* ext, const mat4<F32>& value, bool transpose = false) override;
    inline void Uniform(const char* ext, const vectorImpl<I32>& values) override;
    inline void Uniform(const char* ext, const vectorImpl<F32>& values) override;
    inline void Uniform(const char* ext, const vectorImpl<vec2<F32>>& values) override;
    inline void Uniform(const char* ext, const vectorImpl<vec3<F32>>& values) override;
    inline void Uniform(const char* ext, const vectorImplAligned<vec4<F32>>& values) override;
    inline void Uniform(const char* ext, const vectorImpl<mat3<F32>>& values, bool transpose = false) override;
    inline void Uniform(const char* ext, const vectorImplAligned<mat4<F32>>& values, bool transpose = false) override;

    void Uniform(I32 location, U32 value) override;
    void Uniform(I32 location, I32 value) override;
    void Uniform(I32 location, F32 value) override;
    void Uniform(I32 location, const vec2<F32>& value) override;
    void Uniform(I32 location, const vec2<I32>& value) override;
    void Uniform(I32 location, const vec3<F32>& value) override;
    void Uniform(I32 location, const vec3<I32>& value) override;
    void Uniform(I32 location, const vec4<F32>& value) override;
    void Uniform(I32 location, const vec4<I32>& value) override;
    void Uniform(I32 location, const mat3<F32>& value, bool transpose = false) override;
    void Uniform(I32 location, const mat4<F32>& value, bool transpose = false) override;
    void Uniform(I32 location, const vectorImpl<I32>& values) override;
    void Uniform(I32 location, const vectorImpl<F32>& values) override;
    void Uniform(I32 location, const vectorImpl<vec2<F32>>& values) override;
    void Uniform(I32 location, const vectorImpl<vec3<F32>>& values) override;
    void Uniform(I32 location, const vectorImplAligned<vec4<F32>>& values) override;
    void Uniform(I32 location,
                 const vectorImpl<mat3<F32>>& values,
                 bool transpose = false) override;
    void Uniform(I32 location,
                 const vectorImplAligned<mat4<F32>>& values,
                 bool transpose = false) override;

    void DispatchCompute(U32 xGroups, U32 yGroups, U32 zGroups) override;

    void SetMemoryBarrier(MemoryBarrierType type) override;

   protected:
    /// Creation of a new shader program. Pass in a shader token and use glsw to
    /// load the corresponding effects
    bool load() override;
    /// Linking a shader program also sets up all pre-link properties for the
    /// shader (varying locations, attrib bindings, etc)
    void link();
    /// This should be called in the loading thread, but some issues are still
    /// present, and it's not recommended (yet)
    void threadedLoad();
    /// Cache uniform/attribute locations for shader programs
    I32 getUniformLocation(const char* name) override;

    struct fake_dependency: public std::false_type {};
    template <typename T>
    bool cachedValueUpdate(I32 location, const T& value) {
        static_assert(
                fake_dependency::value,
            "glShaderProgram::cachedValue error: unsupported data type!");
        return false;
    }
    /// Basic OpenGL shader program validation (both in debug and in release)
    bool validateInternal();
    /// Retrieve the program's validation log if we need it
    stringImpl getLog() const;

    /// Add a new shader stage to this program
    void attachShader(glShader* const shader, const bool refresh = false);
    /// Remove a shader stage from this program
    void detachShader(glShader* const shader);

   private:
    typedef hashMapImpl<ULL, I32> ShaderVarMap;
    typedef hashMapImpl<I32, U32> ShaderVarU32Map;
    typedef hashMapImpl<I32, I32> ShaderVarI32Map;
    typedef hashMapImpl<I32, F32> ShaderVarF32Map;
    typedef hashMapImpl<I32, vec2<F32>> ShaderVarVec2F32Map;
    typedef hashMapImpl<I32, vec2<I32>> ShaderVarvec2I32Map;
    typedef hashMapImpl<I32, vec3<F32>> ShaderVarVec3F32Map;
    typedef hashMapImpl<I32, vec3<I32>> ShaderVarVec3I32Map;
    typedef hashMapImpl<I32, vec4<F32>> ShaderVarVec4F32Map;
    typedef hashMapImpl<I32, vec4<I32>> ShaderVarVec4I32Map;
    typedef hashMapImpl<I32, mat3<F32>> ShaderVarMat3Map;
    typedef hashMapImpl<I32, mat4<F32>> ShaderVarMat4Map;

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

    ShaderVarMap _shaderVarLocation;
    bool _validationQueued;
    GLenum _binaryFormat;
    bool _validated;
    bool _loadedFromBinary;
    GLuint _shaderProgramIDTemp;
    static std::array<U32, to_const_uint(ShaderType::COUNT)> _lineOffset;
    std::array<glShader*, to_const_uint(ShaderType::COUNT)> _shaderStage;
    /// ID<->shaders pair
    typedef hashMapImpl<U32, glShader*> ShaderIDMap;
    ShaderIDMap _shaderIDMap;

    glLockManager* _lockManager;
};

namespace Attorney {
    class GLAPIShaderProgram {
    private:
        static void setGlobalLineOffset(U32 offset) {
            glShaderProgram::_lineOffset.fill(offset);
        }
        static void addLineOffset(ShaderType stage, U32 offset) {
            glShaderProgram::_lineOffset[to_uint(stage)] += offset;
        }

        friend class Divide::GL_API;
    };
};  // namespace Attorney
};  // namespace Divide

#endif  //_PLATFORM_VIDEO_OPENGLS_PROGRAM_H_

#include "glShaderProgram.inl"
