#include "Headers/RenderTarget.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

// We don't need more than 4 colour attachments for now.
U8 RenderTarget::g_maxColourAttachments = 4;

RenderTarget::RenderTarget(GFXDevice& context, const stringImpl& name)
    : GraphicsResource(context),
      GUIDWrapper(),
      _width(0),
      _height(0),
      _depthValue(1.0),
      _name(name)
{
    _attachments.init(this, g_maxColourAttachments);
}

RenderTarget::~RenderTarget()
{
}

void RenderTarget::addAttachment(const TextureDescriptor& descriptor,
                                 RTAttachment::Type type,
                                 U8 index,
                                 bool keepPreviousFrame) {
    _attachments.add(type, index, descriptor, keepPreviousFrame);
}

const RTAttachment& RenderTarget::getAttachment(RTAttachment::Type type, U8 index, bool flushStateOnRequest) {
    RTAttachment&  att = *_attachments.get(type, index);
    if (flushStateOnRequest) {
        att.flush();
    }
    return att;
}

const RTAttachment& RenderTarget::getAttachment(RTAttachment::Type type, U8 index) const {
    return *_attachments.get(type, index);
}

const RTAttachment_ptr& RenderTarget::getPrevFrameAttachment(RTAttachment::Type type, U8 index) const {
    return _attachments.getPrevFrame(type, index);
}

void RenderTarget::setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel) {
    for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
            setMipLevel(mipMinLevel, mipMaxLevel, writeLevel, type, j);
        }
    }
}

void RenderTarget::setMipLevel(U16 writeLevel) {
    for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
            setMipLevel(writeLevel, type, j);
        }
    }
}

void RenderTarget::resetMipLevel() {
    for (U8 i = 0; i < to_const_ubyte(RTAttachment::Type::COUNT); ++i) {
        RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
        for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
            resetMipLevel(type, j);
        }
    }
}

RTDrawDescriptor& RenderTarget::defaultPolicy() {
    static RTDrawDescriptor default;
    return default;
}

RTDrawDescriptor& RenderTarget::defaultPolicyKeepDepth() {
    static RTDrawDescriptor policyKeepDepth;
    policyKeepDepth.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
    return policyKeepDepth;
}

RTDrawDescriptor& RenderTarget::defaultPolicyDepthOnly() {
    static RTDrawDescriptor depthOnly;
    depthOnly.drawMask().disableAll();
    depthOnly.drawMask().setEnabled(RTAttachment::Type::Depth, 0, true);

    return depthOnly;
}

TextureDescriptor& RenderTarget::getDescriptor(RTAttachment::Type type, U8 index) {
    return _attachments.get(type, index)->descriptor();
}

bool RenderTarget::create(U16 widthAndHeight) {
    return create(widthAndHeight, widthAndHeight);
}

/// Used by cubemap FB's
void RenderTarget::drawToFace(RTAttachment::Type type, U8 index, U32 nFace, bool includeDepth) {
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
            for (U8 j = 0; j < _attachments.attachmentCount(type); ++j) {
                _attachments.get(type, j)->clearColour(clearColour);
            }
        }
    } else {
        _attachments.get(type, index)->clearColour(clearColour);
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

};
