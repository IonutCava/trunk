#include "Headers/RTAttachment.h"

namespace Divide {

RTAttachment::RTAttachment()
    : _clearColour(DefaultColours::WHITE()),
      _descriptor(TextureDescriptor()),
      _needsRefresh(false),
      _attDirty(false),
      _enabled(false)
{
}

RTAttachment::~RTAttachment()
{
}

TextureDescriptor& RTAttachment::descriptor() {
    return _descriptor;
}

const TextureDescriptor& RTAttachment::descriptor() const {
    return _descriptor;
}

void RTAttachment::flush() {
}

bool RTAttachment::used() const {
    return false;
}

bool RTAttachment::enabled() const {
    return _enabled;
}

void RTAttachment::enabled(const bool state) {
    _enabled = state;
}

bool RTAttachment::changed() const {
    return _needsRefresh;
}

void RTAttachment::fromDescriptor(const TextureDescriptor& descriptor) {
    _descriptor = descriptor;
    _needsRefresh = true;
}

void RTAttachment::clearColour(const vec4<F32>& clearColour) {
    _clearColour.set(clearColour);
}

const vec4<F32>& RTAttachment::clearColour() const {
    return _clearColour;
}


RTAttachmentPool::RTAttachmentPool()
{
 
}

RTAttachmentPool::~RTAttachmentPool()
{
}


const RTAttachment_ptr& RTAttachmentPool::get(RTAttachment::Type type, U8 index) const {
    
    switch(type) {
        case RTAttachment::Type::Colour :
        {
            assert(index < attachmentCount(type));
            return _attachment[to_const_uint(RTAttachment::Type::Colour)][index];
        }
        case RTAttachment::Type::Depth :
        {
            assert(index == 0);
            return _attachment[to_const_uint(RTAttachment::Type::Depth)].front();
        }
        case RTAttachment::Type::Stencil:
        {
            assert(index == 0);
            return _attachment[to_const_uint(RTAttachment::Type::Stencil)].front();
        }
    }

    assert(false);
    return _attachment[0][0];
}

U8 RTAttachmentPool::attachmentCount(RTAttachment::Type type) const {
    return to_ubyte(_attachment[to_uint(type)].size());
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