#include "Headers/Framebuffer.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

Framebuffer::Framebuffer(bool multiSampled)
    : GUIDWrapper(),
      _shouldRebuild(true),
      _clearBuffersState(true),
      _useDepthBuffer(false),
      _disableColorWrites(false),
      _multisampled(multiSampled),
      _width(0),
      _height(0),
      _framebufferHandle(0)
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
    _shouldRebuild = true;

    return true;
}

Texture* Framebuffer::GetAttachment(TextureDescriptor::AttachmentType slot,
                                    bool flushStateOnRequest) {
    Texture* tex = _attachmentTexture[to_uint(slot)];
    if (tex && ((flushStateOnRequest && tex->flushTextureState())  || !flushStateOnRequest)) {
        return tex;
    }

    return nullptr;
}

};
