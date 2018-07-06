#include "Headers/Framebuffer.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

Framebuffer::Framebuffer(bool multiSampled)
    : GUIDWrapper(),
      _multisampled(multiSampled),
      _framebufferHandle(0),
      _width(0),
      _height(0),
      _useDepthBuffer(false),
      _disableColorWrites(false),
      _clearBuffersState(true),
      _layeredRendering(false)
{
    _clearColor.set(DefaultColors::WHITE());
    for (U8 i = 0; i < to_uint(TextureDescriptor::AttachmentType::COUNT); ++i) {
        _attachmentDirty[i] = false;
        _attachmentTexture[i] = nullptr;
    }
}

Framebuffer::~Framebuffer()
{
}

bool Framebuffer::AddAttachment(const TextureDescriptor& descriptor,
                                TextureDescriptor::AttachmentType slot) {
    _attachmentDirty[to_uint(slot)] = true;
    _attachment[to_uint(slot)] = descriptor;

    return true;
}
};