#include "Headers/d3dPixelBuffer.h"

namespace Divide {
d3dPixelBuffer::d3dPixelBuffer(GFXDevice& context, PBType type)
    : PixelBuffer(context, type)
{
}

d3dPixelBuffer::~d3dPixelBuffer()
{
    Destroy();
}

bool d3dPixelBuffer::Create(U16 width, U16 height, U16 depth,
                            GFXImageFormat internalFormatEnum,
                            GFXImageFormat formatEnum,
                            GFXDataFormat dataTypeEnum) {
    return true;
}

void d3dPixelBuffer::Destroy() {
}

void* d3dPixelBuffer::Begin() const {
    return 0;
}

void d3dPixelBuffer::End() const {
}

void d3dPixelBuffer::Bind(U8 unit) const {
}

void d3dPixelBuffer::updatePixels(const F32* const pixels, U32 pixelCount) {
}

bool d3dPixelBuffer::checkStatus() {
    return true;
}

};
