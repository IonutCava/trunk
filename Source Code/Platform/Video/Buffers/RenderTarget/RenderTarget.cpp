#include "stdafx.h"

#include "Headers/RenderTarget.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

namespace {
    bool g_policiesInitialised = false;
};

RTDrawDescriptor RenderTarget::s_policyDefault;
RTDrawDescriptor RenderTarget::s_policyKeepDepth;
RTDrawDescriptor RenderTarget::s_policyDepthOnly;

RTDrawDescriptor& RenderTarget::defaultPolicy() {
    return s_policyDefault;
}

RTDrawDescriptor& RenderTarget::defaultPolicyKeepDepth() {
    return s_policyKeepDepth;
}

RTDrawDescriptor& RenderTarget::defaultPolicyDepthOnly() {
    return s_policyDepthOnly;
}

RenderTarget::RenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor)
    : GUIDWrapper(),
      GraphicsResource(context, getGUID()),
      _descriptor(descriptor)
{
    if (!g_policiesInitialised) {
        s_policyKeepDepth.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);

        s_policyDepthOnly.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        s_policyDepthOnly.drawMask().disableAll();
        s_policyDepthOnly.drawMask().setEnabled(RTAttachmentType::Depth, 0, true);
        g_policiesInitialised = true;
    }

    if (Config::Profile::USE_2x2_TEXTURES) {
        _descriptor._resolution.set(2u);
    }

    U8 colourAttachmentCount = 0;
    for (U8 i = 0; i < descriptor._attachmentCount; ++i) {
        if (descriptor._attachments[i]._type == RTAttachmentType::Colour) {
            ++colourAttachmentCount;
        }
    }

    _attachmentPool = MemoryManager_NEW RTAttachmentPool(*this, colourAttachmentCount);

    for (U8 i = 0; i < descriptor._attachmentCount; ++i) {
        _attachmentPool->update(descriptor._attachments[i]);
    }
}

RenderTarget::~RenderTarget()
{
    MemoryManager::DELETE(_attachmentPool);
}

const RTAttachment& RenderTarget::getAttachment(RTAttachmentType type, U8 index) const {
    return *_attachmentPool->get(type, index);
}

/// Used by cubemap FB's
void RenderTarget::drawToFace(RTAttachmentType type, U8 index, U16 nFace, bool includeDepth) {
    drawToLayer(type, index, nFace, includeDepth);
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

const stringImpl& RenderTarget::getName() const {
    return _descriptor._name;
}

F32& RenderTarget::depthClearValue() {
    return _descriptor._depthValue;
}
};
