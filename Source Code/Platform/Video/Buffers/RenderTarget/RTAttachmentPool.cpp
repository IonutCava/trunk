#include "Headers/RTAttachmentPool.h"

#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

RTAttachmentPool::RTAttachmentPool(RenderTarget& parent, U8 colourAttCount)
    : _parent(parent)
{
    _attachmentCount.fill(0);
    _attachment[to_const_U32(RTAttachment::Type::Colour)].resize(colourAttCount, nullptr);
    _attachment[to_const_U32(RTAttachment::Type::Depth)].resize(1, nullptr);
    _attachment[to_const_U32(RTAttachment::Type::Stencil)].resize(1, nullptr);
}

RTAttachmentPool::~RTAttachmentPool()
{
}

void RTAttachmentPool::copy(const RTAttachmentPool& other) {
    for (U8 i = 0; i < to_const_U8(RTAttachment::Type::COUNT); ++i) {
        for (U8 j = 0; j < other._attachment[i].size(); ++j) {
            const RTAttachment_ptr& att = other._attachment[i][j];
            if (att != nullptr) {
                RTAttachment::Type type = static_cast<RTAttachment::Type>(i);
                add(type, j, att->descriptor());
            }
        }
    }
}

void RTAttachmentPool::onClear() {

}

void RTAttachmentPool::add(RTAttachment::Type type,
                           U8 index,
                           const TextureDescriptor& descriptor) {

    assert(index < to_U8(_attachment[to_U32(type)].size()));

    RTAttachment_ptr& ptr = getInternal(_attachment, type, index);
    assert(ptr == nullptr);

    ptr = std::make_shared<RTAttachment>();
    ptr->fromDescriptor(descriptor);
    _attachmentCount[to_U32(type)]++;
}

RTAttachment_ptr& RTAttachmentPool::getInternal(AttachmentPool& pool, RTAttachment::Type type, U8 index) {
    switch (type) {
        case RTAttachment::Type::Colour:
        {
            assert(index < to_U8(_attachment[to_U32(type)].size()));
            return pool[to_const_U32(RTAttachment::Type::Colour)][index];
        }
        case RTAttachment::Type::Depth:
        {
            assert(index == 0);
            return pool[to_const_U32(RTAttachment::Type::Depth)].front();
        }
        case RTAttachment::Type::Stencil:
        {
            assert(index == 0);
            return pool[to_const_U32(RTAttachment::Type::Stencil)].front();
        }
    }

    DIVIDE_UNEXPECTED_CALL("Invalid render target attachment type");
    return pool[0][0];
}

const RTAttachment_ptr& RTAttachmentPool::getInternal(const AttachmentPool& pool, RTAttachment::Type type, U8 index) const {
    switch (type) {
        case RTAttachment::Type::Colour:
        {
            assert(index < to_U8(_attachment[to_U32(type)].size()));
            return pool[to_const_U32(RTAttachment::Type::Colour)][index];
        }
        case RTAttachment::Type::Depth:
        {
            assert(index == 0);
            return pool[to_const_U32(RTAttachment::Type::Depth)].front();
        }
        case RTAttachment::Type::Stencil:
        {
            assert(index == 0);
            return pool[to_const_U32(RTAttachment::Type::Stencil)].front();
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

void RTAttachmentPool::get(RTAttachment::Type type, vectorImpl<RTAttachment_ptr>& attachments) const {
    if (!attachments.empty()) {
        attachments.resize(0);
    }

    std::back_insert_iterator<std::vector<RTAttachment_ptr>> back_it(attachments);
    auto const usedPredicate = [](const RTAttachment_ptr& ptr) { return ptr && ptr->used(); };
    std::copy_if(std::begin(_attachment[to_U32(type)]), std::end(_attachment[to_U32(type)]), back_it, usedPredicate);
}

U8 RTAttachmentPool::attachmentCount(RTAttachment::Type type) const {
    return _attachmentCount[to_U32(type)];
}

}; //namespace Divide
