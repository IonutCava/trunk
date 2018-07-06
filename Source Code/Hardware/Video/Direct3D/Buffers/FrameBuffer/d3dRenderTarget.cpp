#include "Headers/d3dRenderTarget.h"

d3dRenderTarget::d3dRenderTarget(FBType type) : FrameBuffer(type)
{
}

d3dRenderTarget::~d3dRenderTarget()
{
}

bool d3dRenderTarget::Create(U16 width, U16 height, U8 imageLayers)
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

void d3dRenderTarget::Bind(U8 unit, TextureDescriptor::AttachmentType slot) const
{
}

void d3dRenderTarget::Unbind(U8 unit) const
{
}

void d3dRenderTarget::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth) const
{
}

void d3dRenderTarget::DrawToFace(TextureDescriptor::AttachmentType slot, U8 nFace, bool includeDepth) const
{
}

bool d3dRenderTarget::checkStatus() const
{
	return true;
}

void d3dRenderTarget::BlitFrom(FrameBuffer* inputFB)
{
}

void d3dRenderTarget::UpdateMipMaps(TextureDescriptor::AttachmentType slot) const
{
}