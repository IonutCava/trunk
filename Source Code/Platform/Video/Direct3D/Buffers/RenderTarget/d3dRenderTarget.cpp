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

void d3dRenderTarget::begin(const RTDrawDescriptor& drawPolicy) {}

void d3dRenderTarget::end() {}

void d3dRenderTarget::bind(U8 unit, RTAttachmentType type, U8 index) {}

void d3dRenderTarget::drawToLayer(RTAttachmentType type,
                                  U8 index,
                                  U16 layer,
                                  bool includeDepth) {}

void d3dRenderTarget::setMipLevel(U16 writeLevel) {}

void d3dRenderTarget::readData(const vec4<U16>& rect,
                               GFXImageFormat imageFormat,
                               GFXDataFormat dataType, bufferPtr outData) {}

void d3dRenderTarget::blitFrom(RenderTarget* inputFB,
                               bool blitColour,
                               bool blitDepth) {}

void d3dRenderTarget::blitFrom(RenderTarget* inputFB,
                               U8 index,
                               bool blitColour, bool blitDepth) {}
};
