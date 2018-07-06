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
    virtual void Bind(U16 slot);
    virtual void Unbind(U16 slot);
    virtual void Destroy() = 0;
    virtual void loadData(U32 target, const U8* const ptr, const vec2<U16>& dimensions, U8 bpp, GFXImageFormat format) = 0;
    virtual void setMipMapRange(U32 base = 0, U32 max = 1000) = 0;
    virtual ~Texture() {}

public:
    bool LoadFile(U32 target, const std::string& name);
    void resize(U16 width, U16 height);

    inline       void               setCurrentSampler(const SamplerDescriptor& descriptor) {_samplerDescriptor = descriptor;}
    inline const SamplerDescriptor& getCurrentSampler()                              const {return _samplerDescriptor;}

    inline U32 getHandle()        const {return _handle;}
    inline U16 getWidth()         const {return _width;}
    inline U16 getHeight()        const {return _height;}
    inline U8  getBitDepth()      const {return _bitDepth;}
    inline bool isFlipped()       const {return _flipped;}
    inline bool hasTransparency() const {return _hasTransparency;}

protected:
    template<typename T>
    friend class ImplResourceLoader;
    virtual bool generateHWResource(const std::string& name) {return HardwareResource::generateHWResource(name);}

protected:
    Texture(const bool flipped = false);
    static bool checkBinding(U16 unit, U32 handle);

protected:
    boost::atomic<U32>	_handle;
    U16 _width,_height;
    U8  _bitDepth;
    bool _flipped;
    bool _hasTransparency;
    mat4<F32>  _transformMatrix;
    SamplerDescriptor _samplerDescriptor;
    static Unordered_map<U8/*slot*/, U32/*textureHandle*/> textureBoundMap;
};

#endif
