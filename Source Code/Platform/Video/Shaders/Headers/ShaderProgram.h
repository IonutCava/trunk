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

#ifndef _SHADER_HANDLER_H_
#define _SHADER_HANDLER_H_

#include "Core/Resources/Headers/HardwareResource.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Shaders/Headers/Shader.h"

namespace Divide {

class Camera;
class Material;
class Shader;
struct GenericDrawCommand;

class ShaderProgram : public HardwareResource {
public:
    /// A list of built-in sampler slots. Use these if possible
    enum TextureUsage {
        TEXTURE_UNIT0 = 0,
        TEXTURE_UNIT1 = 1,
        TEXTURE_NORMALMAP = 2,
        TEXTURE_OPACITY   = 3,
        TEXTURE_SPECULAR  = 4,
        TEXTURE_PROJECTION = 5,
        TextureUsage_PLACEHOLDER = 6,
        TEXTURE_GLOSS = TEXTURE_SPECULAR,
        TEXTURE_ROUGHNESS = TEXTURE_GLOSS
    };

    virtual ~ShaderProgram();

    virtual bool bind();
    virtual void unbind(bool resetActiveProgram = true);
    virtual bool update(const U64 deltaTime);
    virtual bool unload() { return true; }
    ///Attributes
    inline void Attribute(const stringImpl& ext, D32 value) { Attribute(cachedLoc(ext,false), value); }
    inline void Attribute(const stringImpl& ext, F32 value) { Attribute(cachedLoc(ext, false), value); }
    inline void Attribute(const stringImpl& ext, const vec2<F32>& value) { Attribute(cachedLoc(ext, false), value); }
    inline void Attribute(const stringImpl& ext, const vec3<F32>& value) { Attribute(cachedLoc(ext, false), value); }
    inline void Attribute(const stringImpl& ext, const vec4<F32>& value) { Attribute(cachedLoc(ext, false), value); }
    ///Uniforms (update constant buffer for D3D. Use index as location in buffer)
    inline void Uniform(const stringImpl& ext, U32 value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, I32 value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, F32 value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, bool value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, const vec2<F32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, const vec2<I32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, const vec2<U16>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, const vec3<F32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, const vec4<F32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const stringImpl& ext, const mat3<F32>& value, bool rowMajor = false) { 
        Uniform(cachedLoc(ext), value, rowMajor); 
    }
    inline void Uniform(const stringImpl& ext, const mat4<F32>& value, bool rowMajor = false) { 
        Uniform(cachedLoc(ext), value, rowMajor); 
    }
    inline void Uniform(const stringImpl& ext, const vectorImpl<I32 >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const stringImpl& ext, const vectorImpl<F32 >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const stringImpl& ext, const vectorImpl<vec2<F32> >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const stringImpl& ext, const vectorImpl<vec3<F32> >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const stringImpl& ext, const vectorImpl<vec4<F32> >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const stringImpl& ext, const vectorImpl<mat3<F32> >& values, bool rowMajor = false) { 
        Uniform(cachedLoc(ext), values, rowMajor); 
    }
    inline void Uniform(const stringImpl& ext, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) { 
        Uniform(cachedLoc(ext), values, rowMajor); 
    }
    ///Uniform Texture
    inline void UniformTexture(const stringImpl& ext, U16 slot) { UniformTexture(cachedLoc(ext), slot); }
    ///Subroutine
    virtual void SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const = 0;
    virtual void SetSubroutine(ShaderType type, U32 index) const = 0;
    virtual U32  GetSubroutineIndex(ShaderType type, const stringImpl& name) const = 0;
    virtual U32  GetSubroutineUniformLocation(ShaderType type, const stringImpl& name) const = 0;
    virtual U32  GetSubroutineUniformCount(ShaderType type) const = 0;
    ///Attribute+Uniform+UniformTexture implementation
    virtual void Attribute(I32 location, D32 value) const = 0;
    virtual void Attribute(I32 location, F32 value) const = 0;
    virtual void Attribute(I32 location, const vec2<F32>& value) const = 0;
    virtual void Attribute(I32 location, const vec3<F32>& value) const = 0;
    virtual void Attribute(I32 location, const vec4<F32>& value) const = 0;
    virtual void Uniform(I32 location, U32 value) const = 0;
    virtual void Uniform(I32 location, I32 value) const = 0;
    virtual void Uniform(I32 location, F32 value) const = 0;
    virtual void Uniform(I32 location, const vec2<F32>& value) const = 0;
    virtual void Uniform(I32 location, const vec2<I32>& value) const = 0;
    virtual void Uniform(I32 location, const vec2<U16>& value) const = 0;
    virtual void Uniform(I32 location, const vec3<F32>& value) const = 0;
    virtual void Uniform(I32 location, const vec4<F32>& value) const = 0;
    virtual void Uniform(I32 location, const mat3<F32>& value, bool rowMajor = false) const = 0;
    virtual void Uniform(I32 location, const mat4<F32>& value, bool rowMajor = false) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<I32 >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<F32 >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<vec2<F32> >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<vec3<F32> >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<vec4<F32> >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<mat3<F32> >& values, bool rowMajor = false) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) const = 0;
    virtual void UniformTexture(I32, U16 slot) = 0;

    inline void Uniform(I32 location, bool value) const { Uniform(location, value ? 1 : 0); }

    inline  void SetOutputCount(U8 count) { _outputCount = std::min(std::max(count, (U8)1), (U8)4); }
    virtual void attachShader(Shader* const shader,const bool refresh = false) = 0;
    virtual void detachShader(Shader* const shader) = 0;
    ///ShaderProgram object id (i.e.: for OGL _shaderProgramId = glCreateProgram())
    inline U32  getId()   const { return _shaderProgramId; }
    ///Currently active
    inline bool isBound() const {return _bound;}
    ///Is the shader ready for drawing?
    virtual bool isValid() const = 0;
    ///  Calling recompile will re-create the marked shaders from source files and update them in the ShaderManager if needed
    void recompile(const bool vertex, 
                   const bool fragment, 
                   const bool geometry = false, 
                   const bool tessellation = false, 
                   const bool compute = false);
    /// Add a define to the shader. The defined must not have been added previously
    void addShaderDefine(const stringImpl& define);
    /// Remove a define from the shader. The defined must have been added previously
    void removeShaderDefine(const stringImpl& define);
    /// Add either fragment or vertex uniforms (without the "uniform" word. e.g. addShaderUniform("vec3 eyePos", VERTEX_SHADER);)
    void addShaderUniform(const stringImpl& uniform, const ShaderType& type);
    /// Remove an uniform from the shader. The uniform must have been added previously for the specified shader type
    void removeUniform(const stringImpl& uniform, const ShaderType& type);
    /// Flush stored uniform locations
    virtual void flushLocCache() = 0;

    /** ------ BEGIN EXPERIMENTAL CODE ----- **/
    inline vectorAlg::vecSize getFunctionCount(ShaderType shader, U8 LoD){
        return _functionIndex[shader][LoD].size();
    }

    inline void setFunctionCount(ShaderType shader, U8 LoD, vectorAlg::vecSize count){
        _functionIndex[shader][LoD].resize(count, 0);
    }

    inline void setFunctionCount(ShaderType shader, vectorAlg::vecSize count){
        for (U8 i = 0; i < Config::SCENE_NODE_LOD; ++i) {
            setFunctionCount(shader, i, count);
        }
    }

    inline void setFunctionIndex(ShaderType shader, U8 LoD, U32 index, U32 functionEntry){
        if(_functionIndex[shader][LoD].empty()) return;

        DIVIDE_ASSERT(index < _functionIndex[shader][LoD].size(), 
                      "ShaderProgram error: Invalid function index specified for update!");
        if(_availableFunctionIndex[shader].empty()) return;

        DIVIDE_ASSERT(functionEntry < _availableFunctionIndex[shader].size(), 
                      "ShaderProgram error: Specified function entry does not exist!");
        _functionIndex[shader][LoD][index] = _availableFunctionIndex[shader][functionEntry];
    }

    inline U32 addFunctionIndex(ShaderType type, U32 index){
        _availableFunctionIndex[type].push_back(index);
        return U32(_availableFunctionIndex[type].size() - 1);
    }
    /** ------ END EXPERIMENTAL CODE ----- **/

protected:
    friend class ShaderManager;
    /// Calling refreshSceneData will force an update on scene specific shader uniforms
    inline void refreshSceneData()  { _sceneDataDirty = true; }
    /// Calling refreshShaderData will force an update on default shader uniforms
    inline void refreshShaderData() { _dirty = true;}
    I32 operator==(const ShaderProgram &_v) { return this->getGUID() == _v.getGUID(); }
    I32 operator!=(const ShaderProgram &_v) { return !(*this == _v); }

protected:

    ShaderProgram(const bool optimise = false);

    virtual I32 cachedLoc(const stringImpl& name, const bool uniform = true) = 0;
    template<typename T>
    friend class ImplResourceLoader;
    virtual bool generateHWResource(const stringImpl& name);
    void updateMatrices();
    const vectorImpl<U32>& getAvailableFunctions(ShaderType type) const;

protected:
    bool _refreshStage[ShaderType_PLACEHOLDER];
    bool _optimise;
    bool _dirty;
    std::atomic_bool _bound;
    std::atomic_bool _linked;
    U32 _shaderProgramId; //<not thread-safe. Make sure assignment is protected with a mutex or something
    U64 _elapsedTime;
    F32 _elapsedTimeMS;
    U8  _outputCount;
    ///A list of preprocessor defines
    vectorImpl<stringImpl > _definesList;
    ///A list of custom shader uniforms
    vectorImpl<stringImpl > _customUniforms[ShaderType_PLACEHOLDER];
    ///ID<->shaders pair
    typedef hashMapImpl<U32, Shader* > ShaderIdMap;
    ShaderIdMap _shaderIdMap;

private:
    bool _sceneDataDirty;
    ///Various uniform/attribute locations
    I32 _timeLoc;
    I32 _enableFogLoc;
    I32 _lightAmbientLoc;
    I32 _invScreenDimension;
    I32 _fogColorLoc;
    I32 _fogDensityLoc;

    vectorImpl<U32> _functionIndex[ShaderType_PLACEHOLDER][Config::SCENE_NODE_LOD];
    vectorImpl<U32> _availableFunctionIndex[ShaderType_PLACEHOLDER];
};

}; //namespace Divide
#endif
