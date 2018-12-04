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
#ifndef _TEXTURE_DESCRIPTOR_H_
#define _TEXTURE_DESCRIPTOR_H_

#include "Core/Resources/Headers/ResourceDescriptor.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Utility/Headers/Colours.h"

namespace Divide {
/// This struct is used to define all of the sampler settings needed to use a texture
/// We do not define copy constructors as we must define descriptors only with POD
struct SamplerDescriptor : public Hashable {
   
    SamplerDescriptor() = default;
    ~SamplerDescriptor() = default;

    /// Texture filtering mode
    TextureFilter _minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
    TextureFilter _magFilter = TextureFilter::LINEAR;
    /// Texture wrap mode (Or S-R-T)
    TextureWrap _wrapU = TextureWrap::REPEAT;
    TextureWrap _wrapV = TextureWrap::REPEAT;
    TextureWrap _wrapW = TextureWrap::REPEAT;

    /// Use SRGB colour space
    bool _srgb = false;
    bool _useRefCompare = false;  ///<use red channel as comparison (e.g. for shadows)
    ComparisonFunction _cmpFunc = ComparisonFunction::LEQUAL;  ///<Used by RefCompare
    /// The value must be in the range [0...255] and is automatically clamped by the max HW supported level
    U8 _anisotropyLevel = 255;
    /// OpenGL eg: used by TEXTURE_MIN_LOD and TEXTURE_MAX_LOD
    F32 _minLOD = -1000.f;
    F32 _maxLOD = 1000.f;
    /// OpenGL eg: used by TEXTURE_LOD_BIAS
    F32 _biasLOD = 0.f;
    /// Used with CLAMP_TO_BORDER as the background colour outside of the texture border
    FColour _borderColour = DefaultColours::BLACK;

    inline size_t getHash() const override {
        _hash = 17;
        Util::Hash_combine(_hash, to_U32(_cmpFunc));
        Util::Hash_combine(_hash, _useRefCompare);
        Util::Hash_combine(_hash, _srgb);
        Util::Hash_combine(_hash, to_U32(_wrapU));
        Util::Hash_combine(_hash, to_U32(_wrapV));
        Util::Hash_combine(_hash, to_U32(_wrapW));
        Util::Hash_combine(_hash, to_U32(_minFilter));
        Util::Hash_combine(_hash, to_U32(_magFilter));
        Util::Hash_combine(_hash, _minLOD);
        Util::Hash_combine(_hash, _maxLOD);
        Util::Hash_combine(_hash, _biasLOD);
        Util::Hash_combine(_hash, _anisotropyLevel);
        Util::Hash_combine(_hash, _borderColour.r);
        Util::Hash_combine(_hash, _borderColour.g);
        Util::Hash_combine(_hash, _borderColour.b);
        Util::Hash_combine(_hash, _borderColour.a);
        return _hash;
    }

    inline bool generateMipMaps() const {
        return _minFilter != TextureFilter::LINEAR &&
               _minFilter != TextureFilter::NEAREST &&
               _minFilter != TextureFilter::COUNT;
    }
};

/// Use to define a texture with details such as type, image formats, etc
/// We do not define copy constructors as we must define descriptors only with
/// POD
class TextureDescriptor final : public PropertyDescriptor {
   public:
    TextureDescriptor() noexcept
        : TextureDescriptor(TextureType::COUNT,
                            GFXImageFormat::COUNT,
                            GFXDataFormat::COUNT)
    {
    }

    TextureDescriptor(TextureType type) noexcept
         : TextureDescriptor(type,
                             GFXImageFormat::COUNT)
    {
    }

    TextureDescriptor(TextureType type,
                      GFXImageFormat internalFormat) noexcept
        : TextureDescriptor(type,
                            internalFormat,
                            GFXDataFormat::COUNT)
    {
    }

    TextureDescriptor(TextureType type,
                      GFXImageFormat internalFmt,
                      GFXDataFormat dataType) noexcept
        : PropertyDescriptor(DescriptorType::DESCRIPTOR_TEXTURE),
          _layerCount(1),
          _baseFormat(GFXImageFormat::COUNT),
          _dataType(dataType),
          _type(type),
          _compressed(false),
          _autoMipMaps(true),
          _mipLevels(0u, 1u),
          _msaaSamples(-1)
    {
        internalFormat(internalFmt);
    }

    virtual ~TextureDescriptor()
    {
    }

    TextureDescriptor* clone() const {
        return MemoryManager_NEW TextureDescriptor(*this);
    }

    inline void setLayerCount(U32 layerCount) { 
        _layerCount = layerCount;
    }

    inline bool isCubeTexture() const {
        return (_type == TextureType::TEXTURE_CUBE_MAP ||
                _type == TextureType::TEXTURE_CUBE_ARRAY);
    }

    inline bool isArrayTexture() const {
        return _type == TextureType::TEXTURE_2D_ARRAY;
    }

    inline bool isMultisampledTexture() const {
        return _type == TextureType::TEXTURE_2D_MS ||
               _type == TextureType::TEXTURE_2D_ARRAY_MS;
    }
    /// A TextureDescriptor will always have a sampler, even if it is the
    /// default one
    inline void setSampler(const SamplerDescriptor& descriptor) {
        _samplerDescriptor = descriptor;
    }

    inline GFXImageFormat internalFormat() const {
        return _internalFormat;
    }

    inline void internalFormat(GFXImageFormat internalFormat) {
        _internalFormat = internalFormat;
        _baseFormat = baseFromInternalFormat(internalFormat);
        _dataType = dataTypeForInternalFormat(internalFormat);
    }

    inline const SamplerDescriptor& getSampler() const {
        return _samplerDescriptor;
    }

    inline GFXDataFormat dataType() const {
        assert(_dataType != GFXDataFormat::COUNT);
        return _dataType;
    }

    inline GFXImageFormat baseFormat() const {
        assert(_baseFormat != GFXImageFormat::COUNT);

        return _baseFormat;
    }

    inline void type(TextureType type) { _type = type; }

    inline TextureType type() const { return _type; }

    inline I16 msaaSamples() const { return _msaaSamples; }

    inline void msaaSamples(I16 sampleCount) { _msaaSamples = sampleCount; }

    inline bool automaticMipMapGeneration() const { return _autoMipMaps; }

    inline void automaticMipMapGeneration(const bool state) { _autoMipMaps = state; }

    inline U8 numChannels() const {
        switch (baseFormat()) {
                case GFXImageFormat::RED:
                case GFXImageFormat::BLUE:
                case GFXImageFormat::GREEN:
                case GFXImageFormat::DEPTH_COMPONENT:
                    return 1;
                case GFXImageFormat::RG:
                    return 2;
                case GFXImageFormat::BGR:
                case GFXImageFormat::RGB:
                    return 3;
                case GFXImageFormat::BGRA:
                case GFXImageFormat::RGBA:
                    return 4;
        }

        return 0;
    }

    U32 _layerCount = 1;
    TextureType _type = TextureType::TEXTURE_2D;
    bool _compressed = false;
    /// Automatically compute mip mpas (overwrites any manual mipmap computation)
    bool _autoMipMaps = true;
    /// The sampler used to initialize this texture with
    SamplerDescriptor _samplerDescriptor = {};
    /// Mip levels
    vec2<U16> _mipLevels = {0u, 1u};
    /// How many MSAA samples to use: -1 (default) = max available, 0 = disabled
    I16 _msaaSamples = -1;

    const vector<stringImpl>& sourceFileList() const { return _sourceFileList; }

  private:
     GFXImageFormat _baseFormat = GFXImageFormat::RGB;
     GFXDataFormat  _dataType = GFXDataFormat::UNSIGNED_BYTE;
     GFXImageFormat _internalFormat = GFXImageFormat::RGBA8;

   private:
     friend class Texture;
     vector<stringImpl> _sourceFileList;
};

};  // namespace Divide
#endif
