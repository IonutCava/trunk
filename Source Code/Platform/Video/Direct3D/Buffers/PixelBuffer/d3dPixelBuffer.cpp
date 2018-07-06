#include "Headers/d3dPixelBuffer.h"

namespace Divide {
d3dPixelBuffer::d3dPixelBuffer(GFXDevice& context, PBType type)
    : PixelBuffer(context, type)
{
}

d3dPixelBuffer::~d3dPixelBuffer()
{
    destroy();
}

bool d3dPixelBuffer::create(U16 width, U16 height, U16 depth,
                            GFXImageFormat internalFormatEnum,
                            GFXImageFormat formatEnum,
                            GFXDataFormat dataTypeEnum) {
    return true;
}

void d3dPixelBuffer::destroy() {
}

bufferPtr d3dPixelBuffer::begin() const {
    return 0;
}

void d3dPixelBuffer::end() const {
}

void d3dPixelBuffer::bind(U8 unit) const {
}

void d3dPixelBuffer::updatePixels(const F32* const pixels, U32 pixelCount) {
}

bool d3dPixelBuffer::checkStatus() {
    return true;
}

};
