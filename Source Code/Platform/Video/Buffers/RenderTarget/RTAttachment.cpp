#include "stdafx.h"

#include "Headers/RTAttachment.h"

#include "Headers/RenderTarget.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

RTAttachment::RTAttachment(RTAttachmentPool& parent, const RTAttachmentDescriptor& descriptor)
    : RTAttachment(parent, descriptor, nullptr)
{
}

RTAttachment::RTAttachment(RTAttachmentPool& parent, const RTAttachmentDescriptor& descriptor, const RTAttachment_ptr& externalAtt)
    : _parent(parent),
      _descriptor(descriptor),
      _texture(nullptr),
      _externalAttachment(externalAtt),
      _changed(false),
      _mipWriteLevel(0),
      _writeLayer(0),
      _binding(0)
{
}

RTAttachment::~RTAttachment()
{
}

const Texture_ptr& RTAttachment::texture(bool autoResolve) const {
    return (autoResolve && isExternal()) ? _externalAttachment->texture() : _texture;
}

void RTAttachment::setTexture(const Texture_ptr& tex) {
    _texture = tex;
    if (tex != nullptr) {
        _descriptor._texDescriptor = tex->descriptor();
    }
    _changed = true;
}

bool RTAttachment::used() const {
    return _texture != nullptr ||
           _externalAttachment != nullptr;
}

bool RTAttachment::isExternal() const {
    return _externalAttachment != nullptr;
}

bool RTAttachment::mipWriteLevel(U16 level) {
    //ToDo: Investigate why this isn't working ... -Ionut
    if (/*_descriptor._texDescriptor.mipLevels() > level && */_mipWriteLevel != level) {
        _mipWriteLevel = level;
        return true;
    }

    return false;
}

U16 RTAttachment::mipWriteLevel() const {
    return _mipWriteLevel;
}

bool RTAttachment::writeLayer(U16 layer) {
    if (numLayers() > layer && _writeLayer != layer) {
        _writeLayer = layer;
        return true;
    }

    return false;
}

U16 RTAttachment::writeLayer() const {
    return _writeLayer;
}

U16 RTAttachment::numLayers() const {
    return to_U16(_descriptor._texDescriptor.layerCount());
}
bool RTAttachment::changed() const {
    return _changed;
}

void RTAttachment::clearColour(const FColour4& clearColour) {
    _descriptor._clearColour.set(clearColour);
}

const FColour4& RTAttachment::clearColour() const {
    return _descriptor._clearColour;
}

void RTAttachment::clearChanged() {
    _changed = false;
}

const RTAttachmentDescriptor& RTAttachment::descriptor() const {
    return _descriptor;
}

RTAttachmentPool& RTAttachment::parent() {
    return _parent;
}
const RTAttachmentPool& RTAttachment::parent() const {
    return _parent;
}

const RTAttachment_ptr& RTAttachment::getExternal() const {
    return _externalAttachment;
}

}; //namespace Divide