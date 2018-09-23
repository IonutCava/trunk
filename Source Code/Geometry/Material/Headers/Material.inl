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

#ifndef _MATERIAL_INL_
#define _MATERIAL_INL_

namespace Divide {

inline void Material::setColourData(const ColourData& other) {
    _dirty = true;
    _colourData = other;
    _translucencyCheck = true;
}

inline void Material::setDiffuse(const FColour& value) {
    _dirty = true;
    _colourData._diffuse = value;
    if (value.a < 0.95f) {
        _translucencyCheck = true;
    }
}

inline void Material::setSpecular(const FColour& value) {
    _dirty = true;
    _colourData._specular = value;
}

inline void Material::setEmissive(const vec3<F32>& value) {
    _dirty = true;
    _colourData._emissive = value;
}

inline void Material::setHardwareSkinning(const bool state) {
    _dirty = true;
    _hardwareSkinning = state;
}

inline void Material::setOpacity(F32 value) {
    _dirty = true;
    _colourData._diffuse.a = value;
    _translucencyCheck = true;
}

inline void Material::setShininess(F32 value) {
    _dirty = true;
    _colourData._shininess = value;
}

// Should the shaders be computed on add? Should reflections be always parsed? Etc
inline void Material::setHighPriority(const bool state) {
    _highPriority = state;
}

inline void Material::addShaderModifier(RenderStagePass renderStagePass, const stringImpl& shaderModifier) {
    _shaderModifier[renderStagePass.index()] = shaderModifier;
}

inline void Material::addShaderModifier(const stringImpl& shaderModifier) {
    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        addShaderModifier(RenderStagePass::stagePass(i), shaderModifier);
    }
}

inline void Material::setShaderDefines(RenderPassType passType, const stringImpl& shaderDefines) {
    for (U8 i = 0; i < to_U8(RenderStage::COUNT); ++i) {
        setShaderDefines(RenderStagePass(static_cast<RenderStage>(i), passType), shaderDefines);
    }
}

inline void Material::setShaderDefines(RenderStage renderStage, const stringImpl& shaderDefines) {
    for (U8 i = 0; i < to_U8(RenderPassType::COUNT); ++i) {
        setShaderDefines(RenderStagePass(renderStage, static_cast<RenderPassType>(i)), shaderDefines);
    }
}

inline void Material::setShaderDefines(RenderStagePass renderStagePass, const stringImpl& shaderDefines) {
    vector<stringImpl>& defines = shaderInfo(renderStagePass)._shaderDefines;
    if (std::find(std::cbegin(defines), std::cend(defines), shaderDefines) == std::cend(defines)) {
        defines.push_back(shaderDefines);
    }
}

inline void Material::setShaderDefines(const stringImpl& shaderDefines) {
    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        setShaderDefines(RenderStagePass::stagePass(i), shaderDefines);
    }
}

/// toggle multi-threaded shader loading on or off for this material
inline void Material::setShaderLoadThreaded(const bool state) {
    _shaderThreadedLoad = state;
}

inline void Material::setShaderProgram(const stringImpl& shader,
                                       const bool computeOnAdd) {
    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        setShaderProgram(shader, RenderStagePass::stagePass(i), computeOnAdd);
    }
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader) {
    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        setShaderProgram(shader, RenderStagePass::stagePass(i));
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash,
                                          I32 variant) {
    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        setRenderStateBlock(renderStateBlockHash, RenderStagePass::stagePass(i), variant);
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash,
                                          RenderStage renderStage,
                                          I32 variant) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        RenderStagePass renderStagePass(renderStage, static_cast<RenderPassType>(pass));

        if (variant < 0) {
            renderStagePass._variant = 0;
            for (size_t& state : defaultRenderStates(renderStagePass)) {
                state = renderStateBlockHash;
                ++renderStagePass._variant;
            }
        } else {
            assert(variant < std::numeric_limits<U8>::max());
            renderStagePass._variant = to_U8(variant);
            defaultRenderStates(renderStagePass)[variant] = renderStateBlockHash;
        }
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash,
                                          RenderPassType renderPassType,
                                          I32 variant) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        RenderStagePass renderStagePass(static_cast<RenderStage>(stage), renderPassType);

        if (variant < 0 ) {
            renderStagePass._variant = 0;
            for (size_t& state : defaultRenderStates(renderStagePass)) {
                state = renderStateBlockHash;
                ++renderStagePass._variant;
            }
        } else {
            assert(variant < std::numeric_limits<U8>::max());
            renderStagePass._variant = to_U8(variant);
            defaultRenderState(renderStagePass) = renderStateBlockHash;
        }
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash,
                                          RenderStagePass renderStagePass,
                                          I32 variant) {
    if (variant < 0) {
        renderStagePass._variant = 0;
        for (size_t& state : defaultRenderStates(renderStagePass)) {
            state = renderStateBlockHash;
            ++renderStagePass._variant;
        }
    } else {
        assert(variant < std::numeric_limits<U8>::max());
        renderStagePass._variant = to_U8(variant);
        defaultRenderStates(renderStagePass)[variant] = renderStateBlockHash;
    }
}

inline size_t Material::getRenderStateBlock(RenderStagePass renderStagePass) {
    return defaultRenderState(renderStagePass);
}

inline void Material::setParallaxFactor(F32 factor) {
    _parallaxFactor = std::min(0.01f, factor);
}

inline F32 Material::getParallaxFactor() const {
    return _parallaxFactor;
}

/// Add a texture <-> bind slot pair to be bound with the default textures
/// on each "bindTexture" call
inline void Material::addExternalTexture(const Texture_ptr& texture, U8 slot, bool activeForDepth) {
    // custom textures are not material dependencies!
    _externalTextures.push_back(ExternalTexture { texture, slot, activeForDepth });
}

inline std::weak_ptr<Texture> Material::getTexture(ShaderProgram::TextureUsage textureUsage) const {
    return _textures[to_U32(textureUsage)];
}

inline const Material::TextureOperation& Material::getTextureOperation() const {
    return _operation;
}

inline const Material::ColourData&  Material::getColourData()  const {
    return _colourData;
}

inline const Material::ShadingMode& Material::getShadingMode() const {
    return _shadingMode;
}

inline const Material::BumpMethod&  Material::getBumpMethod()  const {
    return _bumpMethod;
}

inline bool Material::hasTransparency() const {
    return _translucencySource != TranslucencySource::COUNT;
}

inline void Material::dumpToFile(bool state) {
    _dumpToFile = state;
}

inline bool Material::isDirty() const {
    return _dirty;
}

inline bool Material::isDoubleSided() const {
    return _doubleSided;
}

inline bool Material::isReflective() const {
    return _colourData._shininess > 100 || _isReflective;
}

inline bool Material::isRefractive() const {
    return hasTransparency() && _isRefractive;
}

inline U32 Material::defaultReflectionTextureIndex() const {
    return _reflectionIndex > -1 ? to_U32(_reflectionIndex)
                                 : _defaultReflection.second;
}

inline U32 Material::defaultRefractionTextureIndex() const {
    return _refractionIndex > -1 ? to_U32(_refractionIndex)
                                 : _defaultRefraction.second;
}

inline bool Material::isExternalTexture(ShaderProgram::TextureUsage slot) const {
    return _textureExtenalFlag[to_U32(slot)];
}

inline void Material::setBumpMethod(const BumpMethod& newBumpMethod) {
    _bumpMethod = newBumpMethod;
    recomputeShaders();
}

inline void Material::setShadingMode(const ShadingMode& mode) {
    _shadingMode = mode;
    recomputeShaders();
}

inline ShaderProgramInfo& Material::getShaderInfo(RenderStagePass renderStagePass) {
    return shaderInfo(renderStagePass);
}

inline ShaderProgramInfo& Material::shaderInfo(RenderStagePass renderStagePass) {
    return _shaderInfo[renderStagePass.index()];
}

inline const ShaderProgramInfo& Material::shaderInfo(RenderStagePass renderStagePass) const {
    return _shaderInfo[renderStagePass.index()];
}

// Here we set the shader's name
inline void Material::setShaderProgram(const stringImpl& shader,
                                       RenderStagePass renderStagePass,
                                       const bool computeOnAdd) {
    shaderInfo(renderStagePass)._customShader = true;
    setShaderProgramInternal(shader, renderStagePass, computeOnAdd);
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader,
                                       RenderStagePass renderStagePass) {
    shaderInfo(renderStagePass)._customShader = true;
    setShaderProgramInternal(shader, renderStagePass);
}

inline void Material::setShaderProgram(const stringImpl& shader,
                                       RenderStage stage,
                                       const bool computeOnAdd) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        setShaderProgram(shader, RenderStagePass(stage, static_cast<RenderPassType>(pass)), computeOnAdd);
    }
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader,
                                       RenderStage stage) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        setShaderProgram(shader, RenderStagePass(stage, static_cast<RenderPassType>(pass)));
    }
}

inline void Material::setShaderProgram(const stringImpl& shader,
                                       RenderPassType passType,
                                       const bool computeOnAdd) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        setShaderProgram(shader, RenderStagePass(static_cast<RenderStage>(stage), passType), computeOnAdd);
    }
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader,
                                       RenderPassType passType) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        setShaderProgram(shader, RenderStagePass(static_cast<RenderStage>(stage), passType));
    }
}

inline size_t& Material::defaultRenderState(RenderStagePass renderStagePass) {
    return _defaultRenderStates[renderStagePass.index()][renderStagePass._variant];
}

inline std::array<size_t, 3>& Material::defaultRenderStates(RenderStagePass renderStagePass) {
    return _defaultRenderStates[renderStagePass.index()];
}
}; //namespace Divide

#endif //_MATERIAL_INL_
