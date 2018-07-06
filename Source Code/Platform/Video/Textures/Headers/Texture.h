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

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "TextureDescriptor.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Core/Resources/Headers/HardwareResource.h"

namespace Divide {

/// An API-independent representation of a texture
class NOINITVTABLE Texture : public HardwareResource {
    friend class ResourceCache;
    friend class ResourceLoader;
    template <typename T>
    friend class ImplResourceLoader;
    public:
       struct TextureLoadInfo {
           TextureLoadInfo() :
               _type(TextureType::COUNT),
               _layerIndex(0),
               _cubeMapCount(0)
           {
           }

           TextureType _type;
           I32 _layerIndex;
           I32 _cubeMapCount;
       };
    public:
    /// Bind the texture to the specified texture unit
    virtual void Bind(U8 slot, bool flushStateOnRequest = true) = 0;
    /// Bind a single level
    virtual void BindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write, bool flushStateOnRequest = true) = 0;
    /// Change the texture's mip levels. This can be called at any time
    virtual void setMipMapRange(U16 base = 0, U16 max = 1000) {
        _mipMinLevel = base;
        _mipMaxLevel = max;
    }
    /// Resize the texture to the specified dimensions and upload the new data
    virtual void resize(const U8* const ptr,
                        const vec2<U16>& dimensions,
                        const vec2<U16>& mipLevels) = 0;
    // API-dependent loading function that uploads ptr data to the GPU using the
    // specified parameters
    virtual void loadData(const TextureLoadInfo& info,
                          const U8* const ptr,
                          const vec2<U16>& dimensions,
                          const vec2<U16>& mipLevels, 
                          GFXImageFormat format,
                          GFXImageFormat internalFormat) = 0;

    /// Specify the sampler descriptor used to sample from this texture in the
    /// shaders
    inline void setCurrentSampler(const SamplerDescriptor& descriptor) {
        // This can be called at any time
        _samplerDescriptor = descriptor;
        _textureData._samplerHash = descriptor.getHash();
        // The sampler will be updated before the next bind call and used in
        // that bind
        _samplerDirty = true;
    }
    /// Get the sampler descriptor used by this texture
    inline const SamplerDescriptor& getCurrentSampler() const {
        return _samplerDescriptor;
    }

    inline TextureData& getData() {
        return _textureData;
    }

    inline const TextureData& getData() const {
        return _textureData;
    }
    /// Set/Get the number of layers (used by texture arrays)
    inline void setNumLayers(U8 numLayers) { _numLayers = numLayers; }
    inline U8 getNumLayers() const { return _numLayers; }
    /// Texture width as returned by DevIL
    inline U16 getWidth() const { return _width; }
    /// Texture height depth as returned by DevIL
    inline U16 getHeight() const { return _height; }
    /// Texture min mip level
    inline U16 getMinMipLevel() const { return _mipMinLevel; }
    /// Texture ax mip level
    inline U16 getMaxMipLevel() const { return _mipMaxLevel; }
    /// A rendering API level handle used to uniquely identify this texture
    /// (e.g. for OpenGL, it's the texture object)
    inline U32 getHandle() const { return _textureData.getHandleHigh(); }
    /// Return the texture format used by this entity
    inline GFXImageFormat getFormat() const { return _textureData._textureFormat; }
    /// If the texture has an alpha channel and at least one pixel is
    /// translucent, return true
    inline bool hasTransparency() const { return _hasTransparency; }
    /// Get the type of the texture
    inline TextureType getTextureType() const { return _textureData._textureType; }
    /// Force a full refresh of the mip chain on the next texture bind
    inline void refreshMipMaps() { _mipMapsDirty = !_lockMipMaps; }
    /// Disable/enable automatic calls to mipmap generation calls (e.g. glGenerateMipmaps)
    /// Useful if we compute our own mipmaps
    inline void lockAutomaticMipMapGeneration(bool state) { _lockMipMaps = state; }
    /// Force a full update of the texture (all pending changes and mipmap refresh);
    /// Returns false if the texture is not ready or in an invalid state
    virtual bool flushTextureState() = 0;
   protected:
    SET_DELETE_FRIEND

    /// Use DevIL to load a file into a Texture Object
    bool LoadFile(const TextureLoadInfo& info, const stringImpl& name);
    /// Load texture data using the specified file name
    virtual bool generateHWResource(const stringImpl& name);
    /// Force a refresh of the entire mipmap chain
    virtual void updateMipMaps() = 0;

    explicit Texture(TextureType type);
    virtual ~Texture();

   protected:
    U8 _numLayers;
    U16 _width;
    U16 _height;
    U16 _mipMaxLevel;
    U16 _mipMinLevel;
    bool _lockMipMaps;
    bool _mipMapsDirty;
    bool _samplerDirty;
    bool _hasTransparency;
    bool _power2Size;
    mat4<F32> _transformMatrix;
    TextureData  _textureData;
    SamplerDescriptor _samplerDescriptor;
};

};  // namespace Divide
#endif
