#include "stdafx.h"

#include "Headers/RTAttachment.h"

#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

RTAttachment::RTAttachment()
    : _clearColour(DefaultColours::WHITE()),
      _descriptor(TextureDescriptor()),
      _texture(nullptr),
      _changed(false),
      _mipWriteLevel(0),
      _writeLayer(0),
      _binding(0)
{
}

RTAttachment::~RTAttachment()
{
}

const Texture_ptr& RTAttachment::asTexture() const {
    return _texture;
}

void RTAttachment::setTexture(const Texture_ptr& tex) {
    _texture = tex;
}

const TextureDescriptor& RTAttachment::descriptor() const {
    return _descriptor;
}

bool RTAttachment::used() const {
    return _texture != nullptr;
}

void RTAttachment::mipWriteLevel(U16 level) {
    _mipWriteLevel = level;
}

U16 RTAttachment::mipWriteLevel() const {
    return _mipWriteLevel;
}

void RTAttachment::writeLayer(U16 layer) {
    _writeLayer = layer;
}

U16 RTAttachment::writeLayer() const {
    return _writeLayer;
}

bool RTAttachment::changed() const {
    return _changed;
}

void RTAttachment::fromDescriptor(const TextureDescriptor& descriptor) {
    _descriptor = descriptor;

    _changed = true;
}

void RTAttachment::clearColour(const vec4<F32>& clearColour) {
    _clearColour.set(clearColour);
}

const vec4<F32>& RTAttachment::clearColour() const {
    return _clearColour;
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

}; //namespace Divide