#include "stdafx.h"

#include "Headers/RenderTarget.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

RTDrawDescriptor& RenderTarget::defaultPolicy() {
    static RTDrawDescriptor s_policyDefault;

    return s_policyDefault;
}

RTDrawDescriptor& RenderTarget::defaultPolicyKeepDepth() {
    static RTDrawDescriptor s_policyKeepDepth;
    static bool s_policyInitialised = false;

    if (!s_policyInitialised) {
        s_policyKeepDepth.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        s_policyInitialised = true;
    }

    return s_policyKeepDepth;
}

RTDrawDescriptor& RenderTarget::defaultPolicyKeepColour() {
    static RTDrawDescriptor s_policyKeepColour;
    static bool s_policyInitialised = false;

    if (!s_policyInitialised) {
        s_policyKeepColour.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        s_policyInitialised = true;
    }

    return s_policyKeepColour;
}

RTDrawDescriptor& RenderTarget::defaultPolicyDepthOnly() {
    static RTDrawDescriptor s_policyDepthOnly;
    static bool s_policyInitialised = false;

    if (!s_policyInitialised) {
        s_policyDepthOnly.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        s_policyDepthOnly.drawMask().disableAll();
        s_policyDepthOnly.drawMask().setEnabled(RTAttachmentType::Depth, 0, true);
        s_policyInitialised = true;
    }

    return s_policyDepthOnly;
}

RTDrawDescriptor& RenderTarget::defaultPolicyNoClear() {
    static RTDrawDescriptor s_policyNoClear;
    static bool s_policyInitialised = false;

    if (!s_policyInitialised) {
        s_policyNoClear.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        s_policyNoClear.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        s_policyInitialised = true;
    }

    return s_policyNoClear;
}

RenderTarget::RenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor)
    : GraphicsResource(context, GraphicsResource::Type::RENDER_TARGET, getGUID()),
      _descriptor(descriptor)
{
    if (Config::Profile::USE_2x2_TEXTURES) {
        _descriptor._resolution.set(2u);
    }

    U8 colourAttachmentCount = 0;
    for (U8 i = 0; i < descriptor._attachmentCount; ++i) {
        if (descriptor._attachments[i]._type == RTAttachmentType::Colour) {
            ++colourAttachmentCount;
        }
    }
    for (U8 i = 0; i < descriptor._externalAttachmentCount; ++i) {
        if (descriptor._externalAttachments[i]._type == RTAttachmentType::Colour) {
            ++colourAttachmentCount;
        }
    }

    _attachmentPool = MemoryManager_NEW RTAttachmentPool(*this, colourAttachmentCount);

    for (U8 i = 0; i < descriptor._attachmentCount; ++i) {
        _attachmentPool->update(descriptor._attachments[i]);
    }
    for (U8 i = 0; i < descriptor._externalAttachmentCount; ++i) {
        _attachmentPool->update(descriptor._externalAttachments[i]);
    }
}

RenderTarget::~RenderTarget()
{
    MemoryManager::DELETE(_attachmentPool);
}

const RTAttachment_ptr& RenderTarget::getAttachmentPtr(RTAttachmentType type, U8 index) const {
    return _attachmentPool->get(type, index);
}


const RTAttachment& RenderTarget::getAttachment(RTAttachmentType type, U8 index) const {
    return *_attachmentPool->get(type, index);
}

/// Used by cubemap FB's
void RenderTarget::drawToFace(const DrawLayerParams& params) {
    drawToLayer(params);
}

void RenderTarget::readData(GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) {
    readData(vec4<U16>(0u, 0u, _descriptor._resolution.w, _descriptor._resolution.h), imageFormat, dataType, outData);
}

U16 RenderTarget::getWidth()  const {
    return _descriptor._resolution.w;
}

U16 RenderTarget::getHeight() const {
    return _descriptor._resolution.h;
}

const stringImpl& RenderTarget::name() const {
    return _descriptor._name;
}

F32& RenderTarget::depthClearValue() {
    return _descriptor._depthValue;
}
};
