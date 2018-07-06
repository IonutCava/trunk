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

#include "core.h"

#include "TextureDescriptor.h"
#include "Core/Resources/Headers/HardwareResource.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"

class SamplerDescriptor;
class Texture : public HardwareResource {
/*Abstract interface*/
public:
    virtual void Bind(U16 slot) = 0;
    virtual void loadData(U32 target, const U8* const ptr, const vec2<U16>& dimensions, const vec2<U16>& mipLevels, 
                          GFXImageFormat format, GFXImageFormat internalFormat, bool usePOW2 = false) = 0;
    virtual void setMipMapRange(U16 base = 0, U16 max = 1000) = 0;
    virtual void updateMipMaps() = 0;
    virtual ~Texture() {}

public:
    bool LoadFile(U32 target, const std::string& name);

    void resize(U16 width, U16 height);

    inline       void               setCurrentSampler(const SamplerDescriptor& descriptor) {_samplerDescriptor = descriptor;}
    inline const SamplerDescriptor& getCurrentSampler()                              const {return _samplerDescriptor;}

    inline void        refreshMipMaps()                 { _mipMapsDirty = true; }
    inline void        setNumLayers(U8 numLayers)       { _numLayers  = numLayers; }
    inline U8          getNumLayers()             const { return _numLayers; }
    inline U8          getBitDepth()              const { return _bitDepth; }
    inline U16         getWidth()                 const { return _width; }
    inline U16         getHeight()                const { return _height; }
    inline U32         getHandle()                const { return _handle; }
    inline bool        isFlipped()                const { return _flipped; }
    inline bool        hasTransparency()          const { return _hasTransparency; }
    inline TextureType getTextureType()           const { return _textureType; }

    virtual bool generateHWResource(const std::string& name);

protected:
    Texture(TextureType type, const bool flipped = false);

protected:
    boost::atomic<U32>	_handle;
    U16 _width,_height;
    U8  _bitDepth;
    U8  _numLayers;
    bool _flipped;
    bool _mipMapsDirty;
    bool _hasTransparency;
    TextureType _textureType;
    mat4<F32>  _transformMatrix;
    SamplerDescriptor _samplerDescriptor;
};

#endif
