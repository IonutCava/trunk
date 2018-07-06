#include "Headers/FrameBuffer.h"

FrameBuffer::FrameBuffer(bool multiSampled) : GUIDWrapper(),
                                              _multisampled(multiSampled),
                                              _frameBufferHandle(0),
                                              _width(0),
                                              _height(0),
                                              _useDepthBuffer(false),
                                              _disableColorWrites(false),
                                              _bound(false),
                                              _clearBuffersState(true),
                                              _layeredRendering(false)
{
    _clearColor.set(DefaultColors::WHITE());
    memset(_attachmentDirty, false, 5 * sizeof(bool));
}

FrameBuffer::~FrameBuffer()
{
}

bool FrameBuffer::AddAttachment(const TextureDescriptor& descriptor, TextureDescriptor::AttachmentType slot){
    _attachmentDirty[slot] = true;
    _attachment[slot] = descriptor;
    return true;
}

void FrameBuffer::Bind(U8 unit, TextureDescriptor::AttachmentType slot) {
    _bound = true;
}

void FrameBuffer::Unbind(U8 unit) const {
    _bound = false;
}