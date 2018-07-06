#include "Headers/d3dRenderTarget.h"

namespace Divide {

d3dRenderTarget::d3dRenderTarget(GFXDevice& context, bool multisampled)
    : Framebuffer(context, multisampled)
{
}

d3dRenderTarget::~d3dRenderTarget()
{
}

bool d3dRenderTarget::create(U16 width, U16 height) { return true; }

void d3dRenderTarget::destroy() {}

void d3dRenderTarget::begin(const FramebufferTarget& drawPolicy) {}

void d3dRenderTarget::end() {
    Framebuffer::end();
}

void d3dRenderTarget::bind(U8 unit, TextureDescriptor::AttachmentType slot, bool flushStateOnRequest) {}

void d3dRenderTarget::drawToLayer(TextureDescriptor::AttachmentType slot,
                                  U32 layer,
                                  bool includeDepth) {}

void d3dRenderTarget::setMipLevel(U16 mipLevel, U16 mipMaxLevel, U16 writeLevel,
                                  TextureDescriptor::AttachmentType slot) {}

void d3dRenderTarget::resetMipLevel(TextureDescriptor::AttachmentType slot) {}

bool d3dRenderTarget::checkStatus() const { return true; }

void d3dRenderTarget::readData(const vec4<U16>& rect,
                               GFXImageFormat imageFormat,
                               GFXDataFormat dataType, void* outData) {}

void d3dRenderTarget::blitFrom(Framebuffer* inputFB,
                               TextureDescriptor::AttachmentType slot,
                               bool blitColor, bool blitDepth) {}

void d3dRenderTarget::clear(const FramebufferTarget& drawPolicy) const {}

};
