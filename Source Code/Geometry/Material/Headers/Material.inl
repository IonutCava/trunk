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
    _properties._colourData = other;
    updateTranslucency();
}
inline void Material::setHardwareSkinning(const bool state) {
    _needsNewShader = true;
    _properties._hardwareSkinning = state;
}

inline void Material::setTextureUseForDepth(TextureUsage slot, bool state) {
    _textureUseForDepth[to_base(slot)] = state;
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader, RenderStage stage, RenderPassType pass, U8 variant) {
    assert(variant < g_maxVariantsPerPass);

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            const RenderStage crtStage = static_cast<RenderStage>(s);
            const RenderPassType crtPass = static_cast<RenderPassType>(p);
            if ((stage == RenderStage::COUNT || stage == crtStage) && (pass == RenderPassType::COUNT || pass == crtPass)) {
                ShaderProgramInfo& shaderInfo = _shaderInfo[s][p][variant];
                shaderInfo._customShader = true;
                shaderInfo._shaderCompStage = ShaderBuildStage::COUNT;
                setShaderProgramInternal(shader, shaderInfo, crtStage, crtPass);
            }
        }
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash, RenderStage stage, RenderPassType pass, U8 variant) {
    assert(variant < g_maxVariantsPerPass);

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            const RenderStage crtStage = static_cast<RenderStage>(s);
            const RenderPassType crtPass = static_cast<RenderPassType>(p);
            if ((stage == RenderStage::COUNT || stage == crtStage) && (pass == RenderPassType::COUNT || pass == crtPass)) {
                _defaultRenderStates[s][p][variant] = renderStateBlockHash;
            }
        }
    }
}

inline void Material::disableTranslucency() {
    _properties._translucencyDisabled = true;
}

inline void Material::setParallaxFactor(F32 factor) {
    _properties._parallaxFactor = std::min(0.01f, factor);
}

inline F32 Material::getParallaxFactor() const {
    return _properties._parallaxFactor;
}

inline Texture_wptr Material::getTexture(TextureUsage textureUsage) const {
    SharedLock<SharedMutex> r_lock(_textureLock);
    return _textures[to_U32(textureUsage)];
}

inline const TextureOperation& Material::getTextureOperation() const {
    return _properties._operation;
}

inline Material::ColourData& Material::getColourData() {
    return _properties._colourData;
}

inline const Material::ColourData&  Material::getColourData()  const {
    return _properties._colourData;
}

inline const ShadingMode& Material::getShadingMode() const {
    return _properties._shadingMode;
}

inline const BumpMethod&  Material::getBumpMethod()  const {
    return _properties._bumpMethod;
}

inline bool Material::hasTransparency() const {
    return _properties._translucencySource != TranslucencySource::COUNT;
}

inline bool Material::hasTranslucency() const {
    assert(hasTransparency());

    return _properties._translucent;
}

inline bool Material::isPBRMaterial() const {
    return getShadingMode() == ShadingMode::OREN_NAYAR || getShadingMode() == ShadingMode::COOK_TORRANCE;
}

inline bool Material::isDoubleSided() const {
    return _properties._doubleSided;
}

inline bool Material::receivesShadows() const {
    return _properties._receivesShadows;
}

inline bool Material::isReflective() const {
    if (_properties._isReflective) {
        return true;
    }
    if (getShadingMode() == ShadingMode::BLINN_PHONG ||
        getShadingMode() == ShadingMode::PHONG ||
        getShadingMode() == ShadingMode::FLAT ||
        getShadingMode() == ShadingMode::TOON)
    {
        return _properties._colourData.shininess() > PHONG_REFLECTIVITY_THRESHOLD;
    }

    return _properties._colourData.reflectivity() > 0.0f;
}

inline bool Material::isRefractive() const {
    return hasTransparency() && _properties._isRefractive;
}

inline void Material::setBumpMethod(const BumpMethod& newBumpMethod) {
    _properties._bumpMethod = newBumpMethod;
    _needsNewShader = true;
}

inline void Material::setShadingMode(const ShadingMode& mode) {
    _properties._shadingMode = mode;
    _needsNewShader = true;
}

inline ShaderProgramInfo& Material::shaderInfo(const RenderStagePass& renderStagePass) {
    ShaderPerVariant& variantMap = _shaderInfo[to_base(renderStagePass._stage)][to_base(renderStagePass._passType)];
    assert(renderStagePass._variant < g_maxVariantsPerPass);

    return variantMap[renderStagePass._variant];
}

inline const ShaderProgramInfo& Material::shaderInfo(const RenderStagePass& renderStagePass) const {
    const ShaderPerVariant& variantMap = _shaderInfo[to_base(renderStagePass._stage)][to_base(renderStagePass._passType)];
    assert(renderStagePass._variant < g_maxVariantsPerPass);

    return variantMap[renderStagePass._variant];
}

inline void Material::addShaderDefine(ShaderType type, const Str128& define, bool addPrefix) {
    if (type != ShaderType::COUNT) {
        addShaderDefineInternal(type, define, addPrefix);
    } else {
        for (U8 i = 0; i < to_U8(ShaderType::COUNT); ++i) {
            addShaderDefine(static_cast<ShaderType>(i), define, addPrefix);
        }
    }
}

inline void Material::addShaderDefineInternal(ShaderType type, const Str128& define, bool addPrefix) {
    ModuleDefines& defines = _extraShaderDefines[to_base(type)];

    if (eastl::find_if(eastl::cbegin(defines),
                       eastl::cend(defines),
                       [&define, addPrefix](const auto& it) {
                            return it.second == addPrefix &&
                                   it.first.compare(define.c_str()) == 0;
                        }) == eastl::cend(defines))
    {
        defines.emplace_back(define, addPrefix);
    }
}

inline const ModuleDefines& Material::shaderDefines(ShaderType type) const {
    assert(type != ShaderType::COUNT);

    return _extraShaderDefines[to_base(type)];
}

}; //namespace Divide

#endif //_MATERIAL_INL_
