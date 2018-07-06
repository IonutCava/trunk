#include "Headers/d3dFrameBufferObject.h"

d3dFrameBufferObject::d3dFrameBufferObject(FBOType type) : FrameBufferObject(type)
{
}

d3dFrameBufferObject::~d3dFrameBufferObject()
{
}

bool d3dFrameBufferObject::Create(U16 width, U16 height, U8 imageLayers)
{
    return true;
}

void d3dFrameBufferObject::Destroy()
{
}

void d3dFrameBufferObject::Begin(const FrameBufferObjectTarget& drawPolicy)
{
}

void d3dFrameBufferObject::End() 
{
}

void d3dFrameBufferObject::Bind(U8 unit, TextureDescriptor::AttachmentType slot) const
{
}

void d3dFrameBufferObject::Unbind(U8 unit) const
{
}

void d3dFrameBufferObject::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth) const
{
}

void d3dFrameBufferObject::DrawToFace(TextureDescriptor::AttachmentType slot, U8 nFace, bool includeDepth) const
{
}

bool d3dFrameBufferObject::checkStatus() const 
{
	return true;
}

void d3dFrameBufferObject::BlitFrom(FrameBufferObject* inputFBO) 
{
}

void d3dFrameBufferObject::UpdateMipMaps(TextureDescriptor::AttachmentType slot) const 
{
}