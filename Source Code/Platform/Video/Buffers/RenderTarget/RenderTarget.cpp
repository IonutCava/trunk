#include "Headers/RenderTarget.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

namespace {
    // We don't need more than 4 colour attachments for now.
    const U8 g_maxColourAttachments = 4;
    bool g_policiesInitialised = false;
};

RTDrawDescriptor RenderTarget::_policyDefault;
RTDrawDescriptor RenderTarget::_policyKeepDepth;
RTDrawDescriptor RenderTarget::_policyDepthOnly;

RenderTarget::RenderTarget(GFXDevice& context, const stringImpl& name)
    : GUIDWrapper(),
      GraphicsResource(context, getGUID()),
      _width(0),
      _height(0),
      _depthValue(1.0),
      _name(name)
{
    _attachmentPool = MemoryManager_NEW RTAttachmentPool(*this, g_maxColourAttachments);

    if (!g_policiesInitialised) {
        assert(ParamHandler::instance().getParam<I32>(_ID("rendering.maxRenderTargetOutputs"), 32) > g_maxColourAttachments);

        _policyKeepDepth.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        _policyDepthOnly.drawMask().disableAll();
        _policyDepthOnly.drawMask().setEnabled(RTAttachment::Type::Depth, 0, true);
        g_policiesInitialised = true;
    }
}

RenderTarget::~RenderTarget()
{
    MemoryManager::DELETE(_attachmentPool);
}

void RenderTarget::copy(const RenderTarget& other) {
    _attachmentPool->copy(*other._attachmentPool);
    _width = other._width;
    _height = other._height;
    _depthValue = other._depthValue;
}

void RenderTarget::addAttachment(const TextureDescriptor& descriptor,
                                 RTAttachment::Type type,
                                 U8 index) {
    _attachmentPool->add(type, index, descriptor);
}

const RTAttachment& RenderTarget::getAttachment(RTAttachment::Type type, U8 index, bool flushStateOnRequest) {
    RTAttachment&  att = *_attachmentPool->get(type, index);
    if (flushStateOnRequest) {
        att.flush();
    }
    return att;
}

const RTAttachment& RenderTarget::getAttachment(RTAttachment::Type type, U8 index) const {
    return *_attachmentPool->get(type, index);
}

RTDrawDescriptor& RenderTarget::defaultPolicy() {
    return _policyDefault;
}

RTDrawDescriptor& RenderTarget::defaultPolicyKeepDepth() {
    return _policyKeepDepth;
}

RTDrawDescriptor& RenderTarget::defaultPolicyDepthOnly() {
    return _policyDepthOnly;
}

TextureDescriptor& RenderTarget::getDescriptor(RTAttachment::Type type, U8 index) {
    return _attachmentPool->get(type, index)->descriptor();
}

bool RenderTarget::create(U16 widthAndHeight) {
    return create(widthAndHeight, widthAndHeight);
}

/// Used by cubemap FB's
void RenderTarget::drawToFace(RTAttachment::Type type, U8 index, U16 nFace, bool includeDepth) {
    drawToLayer(type, index, nFace, includeDepth);
}

void RenderTarget::readData(GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) {
    readData(vec4<U16>(0, 0, _width, _height), imageFormat, dataType, outData);
}

// Set the colour the FB will clear to when drawing to it
void RenderTarget::setClearColour(RTAttachment::Type type, U8 index, const vec4<F32>& clearColour) {
    if (type == RTAttachment::Type::COUNT) {
        for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
            type = static_cast<RTAttachment::Type>(i);
            for (U8 j = 0; j < _attachmentPool->attachmentCount(type); ++j) {
                _attachmentPool->get(type, j)->clearColour(clearColour);
            }
        }
    } else {
        _attachmentPool->get(type, index)->clearColour(clearColour);
    }
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
