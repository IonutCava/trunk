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

void d3dFrameBufferObject::Begin(U8 nFace) const
{
}

void d3dFrameBufferObject::End(U8 nFace) const 
{
}

void d3dFrameBufferObject::Bind(U8 unit, TextureDescriptor::AttachmentType slot) const
{
}

void d3dFrameBufferObject::Unbind(U8 unit) const
{
}

void d3dFrameBufferObject::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer) const
{
}

bool d3dFrameBufferObject::checkStatus() const 
{
	return true;
}

void d3dFrameBufferObject::BlitFrom(FrameBufferObject* inputFBO) const 
{
}

void d3dFrameBufferObject::UpdateMipMaps(TextureDescriptor::AttachmentType slot) const 
{
}