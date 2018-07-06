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

#ifndef _HANDLER_H_
#define _HANDLER_H_

#include "Core/Resources/Headers/HardwareResource.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Shaders/Headers/Shader.h"

namespace Divide {

class Camera;
class Material;
class Shader;
class ShaderBuffer;
struct GenericDrawCommand;

class ShaderProgram : public HardwareResource {
   public:
    /// A list of built-in sampler slots. Use these if possible
    enum class TextureUsage : U32 {
        UNIT0 = 0,
        UNIT1 = 1,
        NORMALMAP = 2,
        OPACITY = 3,
        SPECULAR = 4,
        PROJECTION = 5,
        GLOSS = SPECULAR,
        ROUGHNESS = GLOSS,
        COUNT
    };

    virtual ~ShaderProgram();

    virtual bool bind();
    virtual void unbind(bool resetActiveProgram = true);
    virtual bool update(const U64 deltaTime);
    virtual bool unload() { return true; }

    virtual void registerShaderBuffer(ShaderBuffer& buffer) = 0;

    /// Uniforms (update constant buffer for D3D. Use index as location in
    /// buffer)
    virtual void Uniform(const stringImpl& ext, U8 value) = 0;
    virtual void Uniform(const stringImpl& ext, U32 value) = 0;
    virtual void Uniform(const stringImpl& ext, I32 value) = 0;
    virtual void Uniform(const stringImpl& ext, F32 value) = 0;
    virtual void Uniform(const stringImpl& ext, const vec2<F32>& value) = 0;

    virtual void Uniform(const stringImpl& ext, const vec2<I32>& value) = 0;

    virtual void Uniform(const stringImpl& ext, const vec2<U16>& value) = 0;

    virtual void Uniform(const stringImpl& ext, const vec3<F32>& value) = 0;

    virtual void Uniform(const stringImpl& ext, const vec4<F32>& value) = 0;

    virtual void Uniform(const stringImpl& ext,
                         const mat3<F32>& value,
                         bool rowMajor = false) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const mat4<F32>& value,
                         bool rowMajor = false) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const vectorImpl<I32>& values) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const vectorImpl<F32>& values) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const vectorImpl<vec2<F32> >& values) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const vectorImpl<vec3<F32> >& values) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const vectorImpl<vec4<F32> >& values) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const vectorImpl<mat3<F32> >& values,
                         bool rowMajor = false) = 0;
    virtual void Uniform(const stringImpl& ext,
                         const vectorImpl<mat4<F32> >& values,
                         bool rowMajor = false) = 0;

    inline void Uniform(const stringImpl& ext, TextureUsage slot) {
        Uniform(ext, static_cast<U8>(slot));
    }
    
    inline void Uniform(const stringImpl& ext, bool value) {
        Uniform(ext, static_cast<I32>(value ? 1 : 0));
    }

    /// Subroutine
    virtual void SetSubroutines(ShaderType type,
                                const vectorImpl<U32>& indices) const = 0;
    virtual void SetSubroutine(ShaderType type, U32 index) const = 0;
    virtual U32 GetSubroutineIndex(ShaderType type,
                                   const stringImpl& name) const = 0;
    virtual U32 GetSubroutineUniformLocation(ShaderType type,
                                             const stringImpl& name) const = 0;
    virtual U32 GetSubroutineUniformCount(ShaderType type) const = 0;
    /// Uniform+UniformTexture implementation
    virtual void Uniform(I32 location, U32 value) = 0;
    virtual void Uniform(I32 location, I32 value) = 0;
    virtual void Uniform(I32 location, F32 value) = 0;
    virtual void Uniform(I32 location, const vec2<F32>& value) = 0;
    virtual void Uniform(I32 location, const vec2<I32>& value) = 0;
    virtual void Uniform(I32 location, const vec2<U16>& value) = 0;
    virtual void Uniform(I32 location, const vec3<F32>& value) = 0;
    virtual void Uniform(I32 location, const vec4<F32>& value) = 0;
    virtual void Uniform(I32 location,
                         const mat3<F32>& value,
                         bool rowMajor = false) = 0;
    virtual void Uniform(I32 location,
                         const mat4<F32>& value,
                         bool rowMajor = false) = 0;
    virtual void Uniform(I32 location, const vectorImpl<I32>& values) = 0;
    virtual void Uniform(I32 location, const vectorImpl<F32>& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<vec2<F32> >& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<vec3<F32> >& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<vec4<F32> >& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<mat3<F32> >& values,
                         bool rowMajor = false) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<mat4<F32> >& values,
                         bool rowMajor = false) = 0;
    virtual void Uniform(I32 location, U8 slot) = 0;

    inline void Uniform(I32 location, TextureUsage slot) {
        Uniform(location, static_cast<U8>(slot));
    }

    inline void Uniform(I32 location, bool value) {
        Uniform(location, static_cast<I32>(value ? 1 : 0));
    }

    inline void SetOutputCount(U8 count) {
        _outputCount = std::min(std::max(count, (U8)1), (U8)4);
    }

    virtual void attachShader(Shader* const shader,
                              const bool refresh = false) = 0;
    virtual void detachShader(Shader* const shader) = 0;
    /// ShaderProgram object id (i.e.: for OGL _shaderProgramID =
    /// glCreateProgram())
    inline U32 getID() const { return _shaderProgramID; }
    /// Currently active
    inline bool isBound() const { return _bound; }
    /// Is the shader ready for drawing?
    virtual bool isValid() const = 0;
    ///  Calling recompile will re-create the marked shaders from source files
    ///  and update them in the ShaderManager if needed
    void recompile(const bool vertex,
                   const bool fragment,
                   const bool geometry = false,
                   const bool tessellation = false,
                   const bool compute = false);
    /// Add a define to the shader. The defined must not have been added
    /// previously
    void addShaderDefine(const stringImpl& define);
    /// Remove a define from the shader. The defined must have been added
    /// previously
    void removeShaderDefine(const stringImpl& define);

    /** ------ BEGIN EXPERIMENTAL CODE ----- **/
    inline vectorAlg::vecSize getFunctionCount(ShaderType shader, U8 LoD) {
        return _functionIndex[to_uint(shader)][LoD].size();
    }

    inline void setFunctionCount(ShaderType shader,
                                 U8 LoD,
                                 vectorAlg::vecSize count) {
        _functionIndex[to_uint(shader)][LoD].resize(count, 0);
    }

    inline void setFunctionCount(ShaderType shader, vectorAlg::vecSize count) {
        for (U8 i = 0; i < Config::SCENE_NODE_LOD; ++i) {
            setFunctionCount(shader, i, count);
        }
    }

    inline void setFunctionIndex(ShaderType shader,
                                 U8 LoD,
                                 U32 index,
                                 U32 functionEntry) {
        U32 shaderTypeValue = to_uint(shader);

        if (_functionIndex[shaderTypeValue][LoD].empty()) {
            return;
        }

        DIVIDE_ASSERT(index < _functionIndex[shaderTypeValue][LoD].size(),
                      "ShaderProgram error: Invalid function index specified "
                      "for update!");
        if (_availableFunctionIndex[shaderTypeValue].empty()) {
            return;
        }

        DIVIDE_ASSERT(
            functionEntry < _availableFunctionIndex[shaderTypeValue].size(),
            "ShaderProgram error: Specified function entry does not exist!");
        _functionIndex[shaderTypeValue][LoD][index] =
            _availableFunctionIndex[shaderTypeValue][functionEntry];
    }

    inline U32 addFunctionIndex(ShaderType shader, U32 index) {
        U32 shaderTypeValue = to_uint(shader);

        _availableFunctionIndex[shaderTypeValue].push_back(index);
        return U32(_availableFunctionIndex[shaderTypeValue].size() - 1);
    }
    /** ------ END EXPERIMENTAL CODE ----- **/

   protected:
    friend class ShaderManager;
    /// Calling refreshSceneData will force an update on scene specific shader
    /// uniforms
    inline void refreshSceneData() { _sceneDataDirty = true; }
    /// Calling refreshShaderData will force an update on default shader
    /// uniforms
    inline void refreshShaderData() { _dirty = true; }
    I32 operator==(const ShaderProgram& _v) {
        return this->getGUID() == _v.getGUID();
    }
    I32 operator!=(const ShaderProgram& _v) { return !(*this == _v); }

   protected:
    ShaderProgram(const bool optimise = false);

    template <typename T>
    friend class ImplResourceLoader;
    virtual bool generateHWResource(const stringImpl& name);
    void updateMatrices();
    const vectorImpl<U32>& getAvailableFunctions(ShaderType type) const;

   protected:
    bool _optimise;
    bool _dirty;
    std::atomic_bool _bound;
    std::atomic_bool _linked;
    U32 _shaderProgramID;  //<not thread-safe. Make sure assignment is protected
    // with a mutex or something
    U64 _elapsedTime;
    F32 _elapsedTimeMS;
    U8 _outputCount;
    /// A list of preprocessor defines
    vectorImpl<stringImpl> _definesList;
    /// ID<->shaders pair
    typedef hashMapImpl<U32, Shader*> ShaderIDMap;
    ShaderIDMap _shaderIDMap;
    std::array<bool, to_const_uint(ShaderType::COUNT)> _refreshStage;
   private:
    bool _sceneDataDirty;

    std::array<std::array<vectorImpl<U32>, Config::SCENE_NODE_LOD>,
               to_const_uint(ShaderType::COUNT)> _functionIndex;
    std::array<vectorImpl<U32>, to_const_uint(ShaderType::COUNT)>
        _availableFunctionIndex;
};

};  // namespace Divide
#endif
