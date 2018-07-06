#include "Headers/Framebuffer.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

Framebuffer::Framebuffer(GFXDevice& context, bool multiSampled)
    : GraphicsResource(context),
      GUIDWrapper(),
      _shouldRebuild(true),
      _useDepthBuffer(false),
      _disableColorWrites(false),
      _multisampled(multiSampled),
      _width(0),
      _height(0),
      _depthValue(1.0),
      _framebufferHandle(0)
{
    _clearColor.set(DefaultColors::WHITE());
    _attachmentChanged.fill(false);
    _attachmentTexture.fill(nullptr);
}

Framebuffer::~Framebuffer()
{
}

bool Framebuffer::addAttachment(const TextureDescriptor& descriptor,
                                TextureDescriptor::AttachmentType slot) {
    _attachmentChanged[to_uint(slot)] = true;
    _attachment[to_uint(slot)] = descriptor;
    _shouldRebuild = true;

    return true;
}

Texture* Framebuffer::getAttachment(TextureDescriptor::AttachmentType slot,
                                    bool flushStateOnRequest) {
    Texture* tex = _attachmentTexture[to_uint(slot)];
    if (tex && ((flushStateOnRequest && tex->flushTextureState())  || !flushStateOnRequest)) {
        return tex;
    }

    return nullptr;
}


void Framebuffer::setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel) {
    for (U32 i = 0; i < to_uint(TextureDescriptor::AttachmentType::COUNT); ++i) {
        Texture* tex = _attachmentTexture[i];
        if (tex != nullptr) {
            setMipLevel(mipMinLevel, mipMaxLevel, writeLevel, static_cast<TextureDescriptor::AttachmentType>(i));
        }
    }
}

void Framebuffer::resetMipLevel() {
    for (U32 i = 0; i < to_uint(TextureDescriptor::AttachmentType::COUNT); ++i) {
        Texture* tex = _attachmentTexture[i];
        if (tex != nullptr) {
            resetMipLevel(static_cast<TextureDescriptor::AttachmentType>(i));
        }
    }
}

};
