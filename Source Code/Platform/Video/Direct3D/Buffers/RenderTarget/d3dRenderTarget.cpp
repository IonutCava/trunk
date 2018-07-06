#include "stdafx.h"

#include "Headers/d3dRenderTarget.h"

namespace Divide {

IMPLEMENT_CUSTOM_ALLOCATOR(d3dRenderTarget, 0, 0)
d3dRenderTarget::d3dRenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor)
    : RenderTarget(context, descriptor)
{
}

d3dRenderTarget::~d3dRenderTarget()
{
}

bool d3dRenderTarget::resize(U16 width, U16 height) { return true; }

void d3dRenderTarget::drawToLayer(const DrawLayerParams& params) {}

void d3dRenderTarget::setMipLevel(U16 writeLevel) {}

void d3dRenderTarget::readData(const vec4<U16>& rect,
                               GFXImageFormat imageFormat,
                               GFXDataFormat dataType, bufferPtr outData) {}

void d3dRenderTarget::blitFrom(const RTBlitParams& params) {}

void d3dRenderTarget::setDefaultState(const RTDrawDescriptor& drawPolicy) {}
};
