#include "Headers/d3dFrameBufferObject.h"

bool d3dFrameBufferObject::checkStatus() const {
	return true;
}

void d3dFrameBufferObject::BlitFrom(FrameBufferObject* inputFBO) const {
}

void d3dFrameBufferObject::UpdateMipMaps(TextureDescriptor::AttachmentType slot) const {
}