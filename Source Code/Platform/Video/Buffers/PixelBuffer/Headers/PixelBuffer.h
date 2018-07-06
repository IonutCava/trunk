/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _PIXEL_BUFFER_OBJECT_H
#define _PIXEL_BUFFER_OBJECT_H

#include "Platform/DataTypes/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

class PixelBuffer {
public:

    virtual bool Create(U16 width, U16 height,U16 depth = 0,
                        GFXImageFormat internalFormatEnum = RGBA8,
                        GFXImageFormat formatEnum = RGBA,
                        GFXDataFormat   dataTypeEnum = FLOAT_32) = 0;

    virtual void Destroy() = 0;

    virtual void* Begin(U8 nFace=0) const = 0;
    virtual void  End() const = 0;

    virtual void Bind(U8 unit=0) const = 0;

    virtual void  updatePixels(const F32 * const pixels) = 0;
    inline U32    getTextureHandle() const {return _textureId;}
    inline U16    getWidth()         const {return _width;}
    inline U16    getHeight()        const {return _height;}
    inline U16    getDepth()         const {return _depth;}
    inline PBType getType()          const { return _pbtype; }

    virtual ~PixelBuffer(){};
    PixelBuffer(PBType type): _pbtype(type),
                                 _textureId(0),
                              _width(0),
                              _height(0),
                              _depth(0),
                              _pixelBufferHandle(0),
                              _textureType(0){}
protected:
    PBType      _pbtype;
    U32         _textureId;
    U16            _width, _height, _depth;
    U32            _pixelBufferHandle;
    U32            _textureType;
};

}; //namespace Divide
#endif
