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

RTAttachmentPool::RTAttachmentPool() : FrameListener()
{
 
}

RTAttachmentPool::~RTAttachmentPool()
{
}

bool RTAttachmentPool::frameEnded(const FrameEvent& evt) {
    for (const std::pair<RTAttachment::Type, U8> &entry : _attachmentHistoryIndex) {
        std::swap(getInternal(_attachment, entry.first, entry.second),
                  getInternal(_attachmentHistory, entry.first, entry.second));
    }

    return true;
}

void RTAttachmentPool::init(U8 colourAttCount) {
    _attachment[to_const_uint(RTAttachment::Type::Colour)].resize(colourAttCount, nullptr);
    _attachment[to_const_uint(RTAttachment::Type::Depth)].resize(1, nullptr);
    _attachment[to_const_uint(RTAttachment::Type::Stencil)].resize(1, nullptr);

    _attachmentHistory[to_const_uint(RTAttachment::Type::Colour)].resize(colourAttCount, nullptr);
    _attachmentHistory[to_const_uint(RTAttachment::Type::Depth)].resize(1, nullptr);
    _attachmentHistory[to_const_uint(RTAttachment::Type::Stencil)].resize(1, nullptr);
}

void RTAttachmentPool::add(RTAttachment::Type type,
                           U8 index,
                           const TextureDescriptor& descriptor,
                           bool keepPreviousFrame) {
    assert(index < to_ubyte(_attachment[to_uint(type)].size()));

    RTAttachment_ptr& ptr = getInternal(_attachment, type, index);
    assert(ptr == nullptr);

    ptr = std::make_shared<RTAttachment>();
    ptr->fromDescriptor(descriptor);

    if (keepPreviousFrame) {
        RTAttachment_ptr& ptrPrev = getInternal(_attachmentHistory, type, index);
        ptrPrev = std::make_shared<RTAttachment>();
        ptrPrev->fromDescriptor(descriptor);

        _attachmentHistoryIndex.emplace_back(type, index);
    }
}

RTAttachment_ptr& RTAttachmentPool::getInternal(AttachmentPool& pool, RTAttachment::Type type, U8 index) {
    switch (type) {
        case RTAttachment::Type::Colour:
        {
            assert(index < to_ubyte(_attachment[to_uint(type)].size()));
            return pool[to_const_uint(RTAttachment::Type::Colour)][index];
        }
        case RTAttachment::Type::Depth:
        {
            assert(index == 0);
            return pool[to_const_uint(RTAttachment::Type::Depth)].front();
        }
        case RTAttachment::Type::Stencil:
        {
            assert(index == 0);
            return pool[to_const_uint(RTAttachment::Type::Stencil)].front();
        }
    }

    DIVIDE_UNEXPECTED_CALL("Invalid render target attachment type");
    return pool[0][0];
}

const RTAttachment_ptr& RTAttachmentPool::getInternal(const AttachmentPool& pool, RTAttachment::Type type, U8 index) const {
    switch (type) {
        case RTAttachment::Type::Colour:
        {
            assert(index < to_ubyte(_attachment[to_uint(type)].size()));
            return pool[to_const_uint(RTAttachment::Type::Colour)][index];
        }
        case RTAttachment::Type::Depth:
        {
            assert(index == 0);
            return pool[to_const_uint(RTAttachment::Type::Depth)].front();
        }
        case RTAttachment::Type::Stencil:
        {
            assert(index == 0);
            return pool[to_const_uint(RTAttachment::Type::Stencil)].front();
        }
    }

    DIVIDE_UNEXPECTED_CALL("Invalid render target attachment type");
    return pool[0][0];
}

RTAttachment_ptr& RTAttachmentPool::get(RTAttachment::Type type, U8 index) {
    return getInternal(_attachment, type, index);
}

const RTAttachment_ptr& RTAttachmentPool::get(RTAttachment::Type type, U8 index) const {
    return getInternal(_attachment, type, index);
}

void RTAttachmentPool::destroy() {
    for (vectorImpl<RTAttachment_ptr>& attachmentEntry : _attachment) {
        attachmentEntry.clear();
    }
}

U8 RTAttachmentPool::attachmentCount(RTAttachment::Type type) const {
    return to_ubyte(std::count_if(std::cbegin(_attachment[to_uint(type)]),
                                  std::cend(_attachment[to_uint(type)]),
                                  [](const RTAttachment_ptr& ptr) {
                                       return ptr != nullptr;
                                  }));
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