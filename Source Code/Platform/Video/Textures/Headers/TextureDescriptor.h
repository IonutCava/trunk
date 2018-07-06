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

#ifndef _TEXTURE_DESCRIPTOR_H_
#define _TEXTURE_DESCRIPTOR_H_

#include "Core/Resources/Headers/ResourceDescriptor.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Utility/Headers/Colors.h"
#include "Core/Math/Headers/MathHelper.h"

#include <boost/functional/hash.hpp>

namespace Divide {
/// This class is used to define all of the sampler settings needed to use a
/// texture
/// Apply a sampler descriptor to either a texture's ResourceDescriptor or to a
/// TextureDescriptor to use it
/// We do not define copy constructors as we must define descriptors only with
/// POD
class SamplerDescriptor : public PropertyDescriptor {
   public:
    /// The constructor specifies the type so it can be used later for
    /// down-casting if needed
    SamplerDescriptor() : PropertyDescriptor(DescriptorType::DESCRIPTOR_SAMPLER)
    {
        setDefaultValues();
    }
    virtual ~SamplerDescriptor()
    {

    }
    /// All of these are default values that should be safe for any kind of
    /// texture usage
    inline void setDefaultValues() {
        setWrapMode();
        setFilters(TextureFilter::LINEAR_MIPMAP_LINEAR, TextureFilter::LINEAR);
        setAnisotropy(16);
        setLOD();
        toggleSRGBColorSpace(false);
        // The following 2 are mainly used by depthmaps for hardware comparisons
        _cmpFunc = ComparisonFunction::LEQUAL;
        _useRefCompare = false;
        _mipmaps = true;
        _borderColor.set(DefaultColors::BLACK());
    }

    SamplerDescriptor* clone() const {
        return MemoryManager_NEW SamplerDescriptor(*this);
    }

    /*
    *  Sampler states (LOD, wrap modes, anisotropy levels, etc
    */
    inline void setAnisotropy(U8 value = 0) { _anisotropyLevel = value; }

    inline void setLOD(F32 minLOD = -1000.f, F32 maxLOD = 1000.f,
                       F32 biasLOD = 0.f) {
        _minLOD = minLOD;
        _maxLOD = maxLOD;
        _biasLOD = biasLOD;
    }

    inline void setBorderColor(const vec4<F32>& color) {
        _borderColor.set(color);
    }

    inline void setWrapMode(TextureWrap wrapUVW = TextureWrap::REPEAT) {
        setWrapModeU(wrapUVW);
        setWrapModeV(wrapUVW);
        setWrapModeW(wrapUVW);
    }

    inline void setWrapMode(TextureWrap wrapU, TextureWrap wrapV,
                            TextureWrap wrapW = TextureWrap::REPEAT) {
        setWrapModeU(wrapU);
        setWrapModeV(wrapV);
        setWrapModeW(wrapW);
    }

    inline void setWrapMode(I32 wrapU, I32 wrapV, I32 wrapW) {
        setWrapMode(static_cast<TextureWrap>(wrapU),
                    static_cast<TextureWrap>(wrapV),
                    static_cast<TextureWrap>(wrapW));
    }

    inline void setWrapModeU(TextureWrap wrapU) { _wrapU = wrapU; }
    inline void setWrapModeV(TextureWrap wrapV) { _wrapV = wrapV; }
    inline void setWrapModeW(TextureWrap wrapW) { _wrapW = wrapW; }

    inline void setFilters(TextureFilter filters) {
        setMinFilter(filters);
        switch (filters) {
            case TextureFilter::LINEAR_MIPMAP_LINEAR:
            case TextureFilter::LINEAR_MIPMAP_NEAREST:
                setMagFilter(TextureFilter::LINEAR);
                break;
            case TextureFilter::NEAREST_MIPMAP_LINEAR:
            case TextureFilter::NEAREST_MIPMAP_NEAREST:
                setMagFilter(TextureFilter::NEAREST);
                break;
            default:
                setMagFilter(filters);
                break;
        }
    }

    inline void setFilters(TextureFilter minFilter, TextureFilter magFilter) {
        setMinFilter(minFilter);
        setMagFilter(magFilter);
    }

    inline void setMinFilter(TextureFilter minFilter) {
        _minFilter = minFilter;
    }

    inline void setMagFilter(TextureFilter magFilter) {
        assert(magFilter == TextureFilter::LINEAR ||
               magFilter == TextureFilter::NEAREST);
        _magFilter = magFilter;
    }

    inline void toggleMipMaps(const bool state) {
        _mipmaps = state;
    }

    inline void toggleSRGBColorSpace(const bool state) { _srgb = state; }

    inline U32 getHash() const {
        U32 hash = 0;
        Util::Hash_combine(hash, to_uint(_cmpFunc));
        Util::Hash_combine(hash, _useRefCompare);
        Util::Hash_combine(hash, _srgb);
        Util::Hash_combine(hash, _mipmaps);
        Util::Hash_combine(hash, to_uint(_wrapU));
        Util::Hash_combine(hash, to_uint(_wrapV));
        Util::Hash_combine(hash, to_uint(_wrapW));
        Util::Hash_combine(hash, to_uint(_minFilter));
        Util::Hash_combine(hash, to_uint(_magFilter));
        Util::Hash_combine(hash, _minLOD);
        Util::Hash_combine(hash, _maxLOD);
        Util::Hash_combine(hash, _biasLOD);
        Util::Hash_combine(hash, _anisotropyLevel);
        Util::Hash_combine(hash, _borderColor.r);
        Util::Hash_combine(hash, _borderColor.g);
        Util::Hash_combine(hash, _borderColor.b);
        Util::Hash_combine(hash, _borderColor.a);
        return hash;
    }
    /*
    *  "Internal" data
    */

    // HW comparison settings
    ComparisonFunction _cmpFunc;  ///<Used by RefCompare
    bool _useRefCompare;  ///<use red channel as comparison (e.g. for shadows)

    inline TextureWrap wrapU() const { return _wrapU; }
    inline TextureWrap wrapV() const { return _wrapV; }
    inline TextureWrap wrapW() const { return _wrapW; }
    inline TextureFilter minFilter() const { return _minFilter; }
    inline TextureFilter magFilter() const { return _magFilter; }
    inline F32 minLOD() const { return _minLOD; }
    inline F32 maxLOD() const { return _maxLOD; }
    inline F32 biasLOD() const { return _biasLOD; }
    inline bool srgb() const { return _srgb; }
    inline U8 anisotropyLevel() const { return _anisotropyLevel; }
    inline bool generateMipMaps() const { 
        return _mipmaps;
    }
    inline vec4<F32> borderColor() const { return _borderColor; }

   protected:
    // Sampler states
    /// Texture filtering mode
    TextureFilter _minFilter, _magFilter;
    /// Texture wrap mode (Or S-R-T)
    TextureWrap _wrapU, _wrapV, _wrapW;
    /// Use SRGB color space
    bool _srgb;
    /// Use mipmapping
    bool _mipmaps;
    /// The value must be in the range [0...255] and is automatically clamped by
    /// the max HW supported level
    U8 _anisotropyLevel;
    /// OpenGL eg: used by TEXTURE_MIN_LOD and TEXTURE_MAX_LOD
    F32 _minLOD, _maxLOD;
    /// OpenGL eg: used by TEXTURE_LOD_BIAS
    F32 _biasLOD;
    /// Used with CLAMP_TO_BORDER as the background color outside of the texture
    /// border
    vec4<F32> _borderColor;
};

/// Use to define a texture with details such as type, image formats, etc
/// We do not define copy constructors as we must define descriptors only with
/// POD
class TextureDescriptor : public PropertyDescriptor {
   public:
    /// This enum is used when creating Frame Buffers to define the channel that
    /// the texture will attach to
    enum class AttachmentType : U32 {
        Color0 = 0,
        Color1 = 1,
        Color2 = 2,
        Color3 = 3,
        Depth = 4,
        COUNT

    };

    TextureDescriptor()
        : TextureDescriptor(TextureType::COUNT,
                            GFXImageFormat::COUNT,
                            GFXDataFormat::COUNT)
    {
    }

    TextureDescriptor(TextureType type,
                      GFXImageFormat internalFormat)
        : TextureDescriptor(type,
                            internalFormat,
                            GFXDataFormat::COUNT)
    {
    }

    TextureDescriptor(TextureType type,
                      GFXImageFormat internalFormat,
                      GFXDataFormat dataType)
        : PropertyDescriptor(DescriptorType::DESCRIPTOR_TEXTURE),
          _layerCount(1),
          _baseFormat(GFXImageFormat::COUNT),
          _internalFormat(internalFormat),
          _dataType(dataType),
          _type(type)
    {
        _baseFormat = baseFromInternalFormat(internalFormat);
    }

    virtual ~TextureDescriptor()
    {
    }

    TextureDescriptor* clone() const {
        return MemoryManager_NEW TextureDescriptor(*this);
    }

    /// Pixel alignment and miplevels are set to match what the HW sets by
    /// default
    inline void setDefaultValues() {
        setLayerCount(1);
        setSampler(SamplerDescriptor());
        _internalFormat = GFXImageFormat::RGBA8;
        _baseFormat = baseFromInternalFormat(_internalFormat);
        _dataType = GFXDataFormat::UNSIGNED_BYTE;
        _type = TextureType::TEXTURE_2D;
    }

    inline void setLayerCount(U8 layerCount) { 
        _layerCount = layerCount;
    }

    inline bool isCubeTexture() const {
        return (_type == TextureType::TEXTURE_CUBE_MAP ||
                _type == TextureType::TEXTURE_CUBE_ARRAY);
    }

    /// A TextureDescriptor will always have a sampler, even if it is the
    /// default one
    inline void setSampler(const SamplerDescriptor& descriptor) {
        _samplerDescriptor = descriptor;
    }

    inline SamplerDescriptor& getSampler() {
        return _samplerDescriptor;
    }

    inline const SamplerDescriptor& getSampler() const {
        return _samplerDescriptor;
    }

    inline GFXDataFormat dataType() {
        if (_dataType == GFXDataFormat::COUNT) {
            _dataType = dataTypeForInternalFormat(_internalFormat);
        }

        return _dataType;
    }

    inline GFXImageFormat baseFormat() {
        if (_baseFormat == GFXImageFormat::COUNT) {
            _baseFormat = baseFromInternalFormat(_internalFormat);
        }

        return _baseFormat;
    }

    U8 _layerCount;
    /// Texture data information
    GFXImageFormat _internalFormat;
    TextureType _type;
    /// The sampler used to initialize this texture with
    SamplerDescriptor _samplerDescriptor;

    private:
        GFXImageFormat _baseFormat;
        GFXDataFormat _dataType;

};

};  // namespace Divide
#endif
