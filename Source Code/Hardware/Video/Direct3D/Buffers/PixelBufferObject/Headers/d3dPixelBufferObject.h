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

#ifndef _D3D_PIXEL_BUFFER_OBJECT_H
#define _D3D_PIXEL_BUFFER_OBJECT_H

#include "Hardware/Video/Buffers/PixelBufferObject/Headers/PixelBufferObject.h"

class d3dPixelBufferObject : public PixelBufferObject {
public:

	d3dPixelBufferObject(PBOType type) : PixelBufferObject(type) {}
	~d3dPixelBufferObject() {Destroy();}

	bool Create(U16 width, U16 height,U16 depth = 0,
				GFXImageFormat internalFormatEnum = RGBA8,
				GFXImageFormat formatEnum = RGBA,
				GFXDataFormat dataTypeEnum = FLOAT_32) {return true;}

	void Destroy() {};

	void* Begin(U8 nFace=0) const {return 0;};
	void End(U8 nFace=0) const {}

	void Bind(U8 unit=0) const {}
	void Unbind(U8 unit=0) const {}

	void updatePixels(const F32 * const pixels) {}

private:
	bool checkStatus() {return true;}
};

#endif
