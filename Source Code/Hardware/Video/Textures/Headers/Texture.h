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

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "TextureDescriptor.h"
#include "Core/Resources/Headers/HardwareResource.h"

namespace Divide {

/// An API-independent representation of a texture
class Texture : public HardwareResource {
    friend class ResourceCache;
    friend class ResourceLoader;
    template<typename X>
    friend class ImplResourceLoader;

public:
    /// Bind the texture to the specified texture unit
    virtual void Bind(U16 slot) = 0;
    /// Change the texture's mip levels. This can be called at any time
    virtual void setMipMapRange(U16 base = 0, U16 max = 1000) = 0;
    // API-dependent loading function that uploads ptr data to the GPU using the specified parameters
    virtual void loadData(U32 target, const U8* const ptr, const vec2<U16>& dimensions, const vec2<U16>& mipLevels, 
                          GFXImageFormat format, GFXImageFormat internalFormat, bool usePOW2 = false) = 0;

    /// Specify the sampler descriptor used to sample from this texture in the shaders
    inline void setCurrentSampler(const SamplerDescriptor& descriptor) {
        // This can be called at any time
        _samplerDescriptor = descriptor;
        // The sampler will be updated before the next bind call and used in that bind
        _samplerDirty = true;
    }
    /// Get the sampler descriptor used by this texture
    inline const SamplerDescriptor& getCurrentSampler() const {
        return _samplerDescriptor;
    }
    /// Set/Get the number of layers (used by texture arrays)
    inline void        setNumLayers(U8 numLayers)       { _numLayers  = numLayers; }
    inline U8          getNumLayers()             const { return _numLayers; }
    /// Texture bit depth as returned by DevIL
    inline U8          getBitDepth()              const { return _bitDepth; }
    /// Texture width as returned by DevIL
    inline U16         getWidth()                 const { return _width; }
    /// Texture height depth as returned by DevIL
    inline U16         getHeight()                const { return _height; }
    /// A rendering API level handle used to uniquely identify this texture (e.g. for OpenGL, it's the texture object)
    inline U32         getHandle()                const { return _handle; }
    /// Simple flag to check if the texture was flipped vertically
    inline bool        isVerticallyFlipped()      const { return _flipped; }
    /// If the texture has an alpha channel and at least one pixel is translucent, return true
    inline bool        hasTransparency()          const { return _hasTransparency; }
    /// Get the type of the texture
    inline TextureType getTextureType()           const { return _textureType; }
    /// Force a full refresh of the mip chain on the next texture bind
    inline void refreshMipMaps() { _mipMapsDirty = true; }

protected:
    Texture(TextureType type, const bool flipped = false);
    virtual ~Texture();
    /// Use DevIL to load a file into a Texture Object
    bool LoadFile(U32 target, const stringImpl& name);
    /// Load texture data using the specified file name
    virtual bool generateHWResource(const stringImpl& name);
    /// Force a refresh of the entire mipmap chain
    virtual void updateMipMaps() = 0;

protected:
    U8 _bitDepth;
    U8 _numLayers;
    U16 _width;
    U16 _height;
    bool _flipped;
    bool _mipMapsDirty;
    bool _samplerDirty;
    bool _hasTransparency;
    TextureType _textureType;
    mat4<F32> _transformMatrix;
    std::atomic<U32>	_handle;
    SamplerDescriptor _samplerDescriptor;
};

}; //namespace Divide
#endif
