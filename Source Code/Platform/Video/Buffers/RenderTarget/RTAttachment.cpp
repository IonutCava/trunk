#include "Headers/RTAttachment.h"

#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

RTAttachment::RTAttachment()
    : _clearColour(DefaultColours::WHITE()),
      _descriptor(TextureDescriptor()),
      _texture(nullptr),
      _changed(false),
      _attDirty(false),
      _enabled(false),
      _toggledState(false),
      _mipMapLevel(0, 1),
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

TextureDescriptor& RTAttachment::descriptor() {
    return _descriptor;
}

const TextureDescriptor& RTAttachment::descriptor() const {
    return _descriptor;
}

void RTAttachment::flush() {
    assert(_texture != nullptr);
    _texture->flushTextureState();
}

bool RTAttachment::used() const {
    return _texture != nullptr;
}

bool RTAttachment::toggledState() const {
    return _toggledState;
}

void RTAttachment::toggledState(const bool state) {
    _toggledState = state;
}

void RTAttachment::mipMapLevel(U16 min, U16 max) {
    _mipMapLevel.set(min, max);
}

const vec2<U16>& RTAttachment::mipMapLevel() const {
    return _mipMapLevel;
}

bool RTAttachment::enabled() const {
    return _enabled;
}

void RTAttachment::enabled(const bool state) {
    _enabled = state;
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

bool RTAttachment::dirty() const {
    return _attDirty;
}

void RTAttachment::flagDirty() {
    _attDirty = true;
}

void RTAttachment::clean() {
    _attDirty = false;
}

}; //namespace Divide