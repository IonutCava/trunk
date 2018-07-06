#include "Headers/d3dRenderTarget.h"

namespace Divide {

IMPLEMENT_ALLOCATOR(d3dRenderTarget, 0, 0)
d3dRenderTarget::d3dRenderTarget(GFXDevice& context, bool multisampled)
    : RenderTarget(context, multisampled)
{
}

d3dRenderTarget::~d3dRenderTarget()
{
}

bool d3dRenderTarget::create(U16 width, U16 height) { return true; }

void d3dRenderTarget::destroy() {}

void d3dRenderTarget::begin(const RenderTargetDrawDescriptor& drawPolicy) {}

void d3dRenderTarget::end() {
}

void d3dRenderTarget::bind(U8 unit, TextureDescriptor::AttachmentType slot, bool flushStateOnRequest) {}

void d3dRenderTarget::drawToLayer(TextureDescriptor::AttachmentType slot,
                                  U32 layer,
                                  bool includeDepth) {}

void d3dRenderTarget::setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel,
                                  TextureDescriptor::AttachmentType slot) {}

void d3dRenderTarget::resetMipLevel(TextureDescriptor::AttachmentType slot) {}

bool d3dRenderTarget::checkStatus() const { return true; }

void d3dRenderTarget::readData(const vec4<U16>& rect,
                               GFXImageFormat imageFormat,
                               GFXDataFormat dataType, void* outData) {}

void d3dRenderTarget::blitFrom(RenderTarget* inputFB,
                               TextureDescriptor::AttachmentType slot,
                               bool blitColor, bool blitDepth) {}

void d3dRenderTarget::clear(const RenderTargetDrawDescriptor& drawPolicy) const {}

};
