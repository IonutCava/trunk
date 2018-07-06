#include "Headers/RTAttachmentPool.h"

#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

RTAttachmentPool::RTAttachmentPool(RenderTarget& parent, U8 colourAttCount)
    : FrameListener("RTAttachmentPool"),
    _parent(parent),
    _isFrameListener(false)
{
    _attachmentCount.fill(0);
    _attachment[to_const_uint(RTAttachment::Type::Colour)].resize(colourAttCount, nullptr);
    _attachment[to_const_uint(RTAttachment::Type::Depth)].resize(1, nullptr);
    _attachment[to_const_uint(RTAttachment::Type::Stencil)].resize(1, nullptr);

    _attachmentHistory[to_const_uint(RTAttachment::Type::Colour)].resize(colourAttCount, nullptr);
    _attachmentHistory[to_const_uint(RTAttachment::Type::Depth)].resize(1, nullptr);
    _attachmentHistory[to_const_uint(RTAttachment::Type::Stencil)].resize(1, nullptr);
}

RTAttachmentPool::~RTAttachmentPool()
{
    if (_isFrameListener) {
        UNREGISTER_FRAME_LISTENER(this);
    }
}

bool RTAttachmentPool::frameEnded(const FrameEvent& evt) {
    static const bool useTextureCopyMethod = true;

    bool change = false;
    for (const std::pair<RTAttachment::Type, U8> &entry : _attachmentHistoryIndex) {
        RTAttachment_ptr& crt = getInternal(_attachment, entry.first, entry.second);
        RTAttachment_ptr& prev = getInternal(_attachmentHistory, entry.first, entry.second);
        if (useTextureCopyMethod) {
            prev->asTexture()->copy(crt->asTexture());
        } else {
            change = true;
        }
    }

    if (!useTextureCopyMethod) {
        if (change) {
            _parent.onAttachmentsChanged();
        }
    }

    return true;
}

void RTAttachmentPool::onClear() {

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
    _attachmentCount[to_uint(type)]++;

    if (keepPreviousFrame) {
        RTAttachment_ptr& ptrPrev = getInternal(_attachmentHistory, type, index);
        ptrPrev = std::make_shared<RTAttachment>();
        ptrPrev->fromDescriptor(descriptor);

        _attachmentHistoryIndex.emplace_back(type, index);

        if (!_isFrameListener) {
            REGISTER_FRAME_LISTENER(this, 6);
            _isFrameListener = true;
        }
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

RTAttachment_ptr& RTAttachmentPool::getPrevFrame(RTAttachment::Type type, U8 index) {
    return getInternal(_attachmentHistory, type, index);
}

const RTAttachment_ptr& RTAttachmentPool::getPrevFrame(RTAttachment::Type type, U8 index) const {
    return getInternal(_attachmentHistory, type, index);
}

bool RTAttachmentPool::keepPrevFrame(RTAttachment::Type type, U8 index) const {
    return std::find_if(std::cbegin(_attachmentHistoryIndex),
                        std::cend(_attachmentHistoryIndex),
                        [type, index](const std::pair<RTAttachment::Type, U8>& entry) {
                            return entry.first == type && entry.second == index;
                        }) != std::cend(_attachmentHistoryIndex);
}

U8 RTAttachmentPool::attachmentCount(RTAttachment::Type type) const {
    return _attachmentCount[to_uint(type)];
}

}; //namespace Divide
