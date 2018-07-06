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

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "Core/Resources/Headers/Resource.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Headers/GraphicsResource.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

#include "Utility/Headers/ImageTools.h"

namespace Divide {

/// An API-independent representation of a texture
class NOINITVTABLE Texture : public GraphicsResource, public Resource {
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
           U32 _layerIndex;
           U32 _cubeMapCount;
       };
    public:

    explicit Texture(GFXDevice& context,
                     const stringImpl& name,
                     const stringImpl& resourceName,
                     const stringImpl& resourceLocation,
                     TextureType type,
                     bool asyncLoad);
    virtual ~Texture();

    /// Bind the texture to the specified texture unit
    virtual void bind(U8 slot, bool flushStateOnRequest = true) = 0;
    /// Bind a single level
    virtual void bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write, bool flushStateOnRequest = true) = 0;
    /// Change the texture's mip levels. This can be called at any time
    virtual void setMipMapRange(U16 base = 0, U16 max = 1000) {
        _mipMinLevel = base;
        _mipMaxLevel = max;
    }
    /// Resize the texture to the specified dimensions and upload the new data
    virtual void resize(const bufferPtr ptr,
                        const vec2<U16>& dimensions,
                        const vec2<U16>& mipLevels) = 0;
    // API-dependent loading function that uploads ptr data to the GPU using the
    // specified parameters
    virtual void loadData(const TextureLoadInfo& info,
                          const TextureDescriptor& descriptor,
                          const vectorImpl<ImageTools::ImageLayer>& imageLayers,
                          const vec2<U16>& mipLevels) = 0;
    virtual void loadData(const TextureLoadInfo& info,
                          const TextureDescriptor& descriptor,
                          const bufferPtr data,
                          const vec2<U16>& dimensions,
                          const vec2<U16>& mipLevels) = 0;

    // Other must have same size!
    virtual void copy(const Texture_ptr& other) = 0;

    /// Specify the sampler descriptor used to sample from this texture in the
    /// shaders
    inline void setCurrentSampler(const SamplerDescriptor& descriptor) {
        // This can be called at any time
        _descriptor._samplerDescriptor = descriptor;
        _textureData._samplerHash = descriptor.getHash();
        // The sampler will be updated before the next bind call and used in
        // that bind
        _samplerDirty = true;
    }
    /// Get the sampler descriptor used by this texture
    inline const SamplerDescriptor& getCurrentSampler() const {
        return _descriptor._samplerDescriptor;
    }

    inline TextureData& getData() {
        return _textureData;
    }

    inline const TextureData& getData() const {
        return _textureData;
    }
    /// Set/Get the number of layers (used by texture arrays)
    inline void setNumLayers(U32 numLayers) { _numLayers = numLayers; }
    inline U32 getNumLayers() const { return _numLayers; }
    /// Texture width as returned by STB/DDS loader
    inline U16 getWidth() const { return _width; }
    /// Texture height depth as returned by STB/DDS loader
    inline U16 getHeight() const { return _height; }
    /// Texture min mip level
    inline U16 getMinMipLevel() const { return _mipMinLevel; }
    /// Texture ax mip level
    inline U16 getMaxMipLevel() const { return _mipMaxLevel; }
    /// A rendering API level handle used to uniquely identify this texture
    /// (e.g. for OpenGL, it's the texture object)
    inline U32 getHandle() const { return _textureData.getHandleHigh(); }
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

    const TextureDescriptor& getDescriptor() const {
        return _descriptor;
    }

    static U16 computeMipCount(U16 width, U16 height);

   protected:
    /// Use STB/NV_DDS to load a file into a Texture Object
    bool loadFile(const TextureLoadInfo& info, const stringImpl& name);
    /// Load texture data using the specified file name
    virtual bool load(DELEGATE_CBK<void, Resource_ptr> onLoadCallback) override;
    virtual void threadedLoad(DELEGATE_CBK<void, Resource_ptr> onLoadCallback);
    /// Force a refresh of the entire mipmap chain
    virtual void updateMipMaps() = 0;

   protected:
    U32 _numLayers;
    U16 _width;
    U16 _height;
    U16 _mipMaxLevel;
    U16 _mipMinLevel;
    bool _lockMipMaps;
    bool _mipMapsDirty;
    bool _samplerDirty;
    bool _hasTransparency;
    bool _asyncLoad;
    TextureData  _textureData;
    TextureDescriptor _descriptor;

  protected:
    static const char* s_missingTextureFileName;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Texture);

};  // namespace Divide
#endif // _TEXTURE_H_
