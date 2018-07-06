#include "Headers/d3dRenderTarget.h"

d3dRenderTarget::d3dRenderTarget(bool multisampled) : FrameBuffer(multisampled)
{
}

d3dRenderTarget::~d3dRenderTarget()
{
}

bool d3dRenderTarget::Create(U16 width, U16 height)
{
    return true;
}

void d3dRenderTarget::Destroy()
{
}

void d3dRenderTarget::Begin(const FrameBufferTarget& drawPolicy)
{
}

void d3dRenderTarget::End()
{
}

void d3dRenderTarget::Bind(U8 unit, TextureDescriptor::AttachmentType slot)
{
}

void d3dRenderTarget::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth)
{
}

void d3dRenderTarget::SetMipLevel(U8 mipLevel, TextureDescriptor::AttachmentType slot) 
{
}

void d3dRenderTarget::ResetMipLevel(TextureDescriptor::AttachmentType slot) 
{
}

bool d3dRenderTarget::checkStatus() const
{
	return true;
}

void d3dRenderTarget::ReadData(const vec4<U16>& rect, GFXImageFormat imageFormat, GFXDataFormat dataType, void* outData)
{
}

void d3dRenderTarget::BlitFrom(FrameBuffer* inputFB, TextureDescriptor::AttachmentType slot, bool blitColor, bool blitDepth)
{
}
