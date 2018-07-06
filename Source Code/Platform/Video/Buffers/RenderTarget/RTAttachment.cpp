#include "stdafx.h"

#include "Headers/RTAttachment.h"

#include "Headers/RenderTarget.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

RTAttachment::RTAttachment(const RTAttachmentDescriptor& descriptor)
    : RTAttachment(descriptor, nullptr)
{
}

RTAttachment::RTAttachment(const RTAttachmentDescriptor& descriptor, const RTAttachment_ptr& externalAtt)
    : _descriptor(descriptor),
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

const Texture_ptr& RTAttachment::texture() const {
    return isExternal() ? _externalAttachment->texture() : _texture;
}

void RTAttachment::texture(const Texture_ptr& tex) {
    _texture = tex;
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
    if (/*_descriptor._texDescriptor._mipLevels > level && */_mipWriteLevel != level) {
        _mipWriteLevel = level;
        return true;
    }

    return false;
}

U16 RTAttachment::mipWriteLevel() const {
    return _mipWriteLevel;
}

bool RTAttachment::writeLayer(U16 layer) {
    if (_descriptor._texDescriptor._layerCount > layer && _writeLayer != layer) {
        _writeLayer = layer;
        return true;
    }

    return false;
}

U16 RTAttachment::writeLayer() const {
    return _writeLayer;
}

bool RTAttachment::changed() const {
    return _changed;
}

void RTAttachment::clearColour(const FColour& clearColour) {
    _descriptor._clearColour.set(clearColour);
}

const FColour& RTAttachment::clearColour() const {
    return _descriptor._clearColour;
}

void RTAttachment::clearChanged() {
    _changed = false;
}

U32 RTAttachment::binding() const {
    return _binding;
}

void RTAttachment::binding(U32 binding) {
    _binding = binding;
}

const RTAttachmentDescriptor& RTAttachment::descriptor() const {
    return _descriptor;
}
}; //namespace Divide