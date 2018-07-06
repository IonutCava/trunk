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
#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "Core/Resources/Headers/Resource.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Headers/GraphicsResource.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

#include "Utility/Headers/ImageTools.h"

namespace Divide {

/// An API-independent representation of a texture
class NOINITVTABLE Texture : public CachedResource, public GraphicsResource {
    friend class ResourceCache;
    friend class ResourceLoader;
    template <typename T>
    friend class ImplResourceLoader;
    public:
       struct TextureLoadInfo {
           TextureLoadInfo() noexcept :
               _layerIndex(0),
               _cubeMapCount(0)
           {
           }

           U32 _layerIndex;
           U32 _cubeMapCount;
       };
    public:

    explicit Texture(GFXDevice& context,
                     size_t descriptorHash,
                     const stringImpl& name,
                     const stringImpl& resourceName,
                     const stringImpl& resourceLocation,
                     bool isFlipped,
                     bool asyncLoad,
                     const TextureDescriptor& texDescriptor);

    virtual ~Texture();

    /// Bind a single level
    virtual void bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) = 0;
    /// Change the texture's mip levels. This can be called at any time
    virtual void setMipMapRange(U16 base = 0, U16 max = 1000) {
        ACKNOWLEDGE_UNUSED(base);
        ACKNOWLEDGE_UNUSED(max);
    }
    /// Resize the texture to the specified dimensions and upload the new data
    virtual void resize(const bufferPtr ptr,
                        const vec2<U16>& dimensions) = 0;
    // API-dependent loading function that uploads ptr data to the GPU using the
    // specified parameters
    virtual void loadData(const TextureLoadInfo& info,
                          const vector<ImageTools::ImageLayer>& imageLayers) = 0;

    virtual void loadData(const TextureLoadInfo& info,
                          const bufferPtr data,
                          const vec2<U16>& dimensions) = 0;

    // Other must have same size!
    virtual void copy(const Texture_ptr& other) = 0;

    /// Specify the sampler descriptor used to sample from this texture in the shaders
    virtual void setCurrentSampler(const SamplerDescriptor& descriptor);

    /// Get the sampler descriptor used by this texture
    inline const SamplerDescriptor& getCurrentSampler() const {
        return _descriptor._samplerDescriptor;
    }

    inline TextureData getData() const {
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
    inline U16 getMinMipLevel() const { return _descriptor._mipLevels.min; }
    /// Texture ax mip level
    inline U16 getMaxMipLevel() const { return _descriptor._mipLevels.max; }
    /// A rendering API level handle used to uniquely identify this texture
    /// (e.g. for OpenGL, it's the texture object)
    inline U32 getHandle() const { return _textureData.getHandle(); }
    /// If the texture has an alpha channel and at least one pixel is translucent, return true
    inline bool hasTranslucency() const { return _hasTranslucency; }
    /// If the texture has an alpha channel and at least on pixel is fully transparent and
    // no pixels are partially transparent, return true
    inline bool hasTransparency() const { return _hasTransparency; }
    /// Get the type of the texture
    inline TextureType getTextureType() const { return _textureData._textureType; }

    const TextureDescriptor& getDescriptor() const { return _descriptor; }

    static U16 computeMipCount(U16 width, U16 height);

    inline bool automaticMipMapGeneration() const { return _descriptor.automaticMipMapGeneration(); }

   protected:
    /// Use STB/NV_DDS to load a file into a Texture Object
    bool loadFile(const TextureLoadInfo& info, const stringImpl& name, ImageTools::ImageData& fileData);
    /// Load texture data using the specified file name
    virtual bool load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) override;
    virtual void threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback);

    virtual void validateDescriptor();

   protected:
    U32 _numLayers;
    U16 _width;
    U16 _height;
    bool _flipped;
    bool _hasTransparency;
    bool _hasTranslucency;
    bool _asyncLoad;
    TextureData  _textureData;
    TextureDescriptor _descriptor;

  protected:
    static const char* s_missingTextureFileName;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Texture);

};  // namespace Divide
#endif // _TEXTURE_H_
