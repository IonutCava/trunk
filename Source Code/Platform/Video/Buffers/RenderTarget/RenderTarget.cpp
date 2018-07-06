#include "Headers/RenderTarget.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

RenderTarget::RenderTarget(GFXDevice& context, bool multiSampled)
    : GraphicsResource(context),
      GUIDWrapper(),
      _shouldRebuild(true),
      _useDepthBuffer(false),
      _multisampled(multiSampled),
      _width(0),
      _height(0),
      _depthValue(1.0),
      _framebufferHandle(0)
{
    _clearColors.fill(DefaultColors::WHITE());
    _attachmentChanged.fill(false);
    _attachmentTexture.fill(nullptr);
}

RenderTarget::~RenderTarget()
{
}

void RenderTarget::addAttachment(const TextureDescriptor& descriptor,
                                 TextureDescriptor::AttachmentType type) {
    _attachmentChanged[to_uint(type)] = true;
    _attachment[to_uint(type)] = descriptor;
    _shouldRebuild = true;
}

void RenderTarget::addAttachment(const Texture_ptr& texture,
                                 TextureDescriptor::AttachmentType type) {
    addAttachment(texture->getDescriptor(), type);
    _attachmentTexture[to_uint(type)] = texture;
}

const Texture_ptr& RenderTarget::getAttachment(TextureDescriptor::AttachmentType slot,
                                               bool flushStateOnRequest) {
    static Texture_ptr defaultAttachment;

    Texture_ptr& tex = _attachmentTexture[to_uint(slot)];
    if (tex && ((flushStateOnRequest && tex->flushTextureState())  || !flushStateOnRequest)) {
        return tex;
    }

    return defaultAttachment;
}


void RenderTarget::setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel) {
    for (U32 i = 0; i < to_const_uint(TextureDescriptor::AttachmentType::COUNT); ++i) {
        const Texture_ptr& tex = _attachmentTexture[i];
        if (tex != nullptr) {
            setMipLevel(mipMinLevel, mipMaxLevel, writeLevel, static_cast<TextureDescriptor::AttachmentType>(i));
        }
    }
}

void RenderTarget::resetMipLevel() {
    for (U32 i = 0; i < to_const_uint(TextureDescriptor::AttachmentType::COUNT); ++i) {
        const Texture_ptr& tex = _attachmentTexture[i];
        if (tex != nullptr) {
            resetMipLevel(static_cast<TextureDescriptor::AttachmentType>(i));
        }
    }
}

};
