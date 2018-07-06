#include "stdafx.h"

#include "Headers/d3dPixelBuffer.h"

namespace Divide {
d3dPixelBuffer::d3dPixelBuffer(GFXDevice& context, PBType type, const char* name)
    : PixelBuffer(context, type, name)
{
}

d3dPixelBuffer::~d3dPixelBuffer()
{
}

bool d3dPixelBuffer::create(U16 width, U16 height, U16 depth,
                            GFXImageFormat internalFormatEnum,
                            GFXImageFormat formatEnum,
                            GFXDataFormat dataTypeEnum) {
    return true;
}

void d3dPixelBuffer::updatePixels(const F32* const pixels, U32 pixelCount) {
}
};
