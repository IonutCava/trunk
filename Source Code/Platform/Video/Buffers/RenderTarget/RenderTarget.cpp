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
      _width(descriptor._resolution.w),
      _height(descriptor._resolution.h),
      _depthValue(1.0),
      _name(descriptor._name)
{
    if (!g_policiesInitialised) {
        s_policyKeepDepth.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        s_policyDepthOnly.drawMask().disableAll();
        s_policyDepthOnly.drawMask().setEnabled(RTAttachment::Type::Depth, 0, true);
        g_policiesInitialised = true;
    }

    if (Config::Profile::USE_2x2_TEXTURES) {
        _width = _height = 2;
    }

    U8 colourAttachmentCount = 0;
    for (U8 i = 0; i < descriptor._attachmentCount; ++i) {
        if (descriptor._attachments[i]._type == RTAttachment::Type::Colour) {
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

const RTAttachment& RenderTarget::getAttachment(RTAttachment::Type type, U8 index) const {
    return *_attachmentPool->get(type, index);
}

/// Used by cubemap FB's
void RenderTarget::drawToFace(RTAttachment::Type type, U8 index, U16 nFace, bool includeDepth) {
    drawToLayer(type, index, nFace, includeDepth);
}

void RenderTarget::readData(GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) {
    readData(vec4<U16>(0, 0, _width, _height), imageFormat, dataType, outData);
}

void RenderTarget::setClearDepth(F32 depthValue) {
    _depthValue = depthValue;
}

U16 RenderTarget::getWidth()  const {
    return _width;
}

U16 RenderTarget::getHeight() const {
    return _height;
}

const stringImpl& RenderTarget::getName() const {
    return _name;
}

};
