#include "Headers/glRTAttachment.h"

namespace Divide {
glRTAttachment::glRTAttachment()
    : RTAttachment(),
      _texture(nullptr),
      _toggledState(false),
      _mipMapLevel(0, 1)
{
    setInfo(GL_NONE, 0u);
}

glRTAttachment::~glRTAttachment()
{
}

void glRTAttachment::flush()
{
    assert(_texture != nullptr);
    _texture->flushTextureState();
}

bool glRTAttachment::used() const {
    return _texture != nullptr;
}

void glRTAttachment::clearRefreshFlag() {
    _needsRefresh = false;
}

Texture_ptr glRTAttachment::asTexture() const {
    return _texture;
}

void glRTAttachment::setTexture(const Texture_ptr& tex) {
    _texture = std::static_pointer_cast<glTexture>(tex);
}

void glRTAttachment::setInfo(GLenum slot, U32 handle) {
    _info.first = slot;
    _info.second = handle;
}

std::pair<GLenum, U32> glRTAttachment::getInfo() {
    return _info;
}

bool glRTAttachment::toggledState() const {
    return _toggledState;
}

void glRTAttachment::toggledState(const bool state) {
    _toggledState = state;
}

void glRTAttachment::mipMapLevel(U16 min, U16 max) {
    _mipMapLevel.set(min, max);
}

const vec2<U16>& glRTAttachment::mipMapLevel() const {
    return _mipMapLevel;
}

}; //namespace Divide