#include "stdafx.h"

#include "Headers/RTAttachmentPool.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Utility/Headers/Localization.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

namespace {
    const char* getAttachmentName(RTAttachment::Type type) {
        switch (type) {
        case RTAttachment::Type::Colour:  return "Colour";
        case RTAttachment::Type::Depth:   return "Depth";
        case RTAttachment::Type::Stencil: return "Stencil";
        };

        return "ERROR";
    };
};

RTAttachmentPool::RTAttachmentPool(RenderTarget& parent, U8 colourAttCount)
    : _parent(parent)
{
    _attachmentCount.fill(0);
    _attachment[to_base(RTAttachment::Type::Colour)].resize(colourAttCount, nullptr);
    _attachment[to_base(RTAttachment::Type::Depth)].resize(1, nullptr);
    _attachment[to_base(RTAttachment::Type::Stencil)].resize(1, nullptr);
}

RTAttachmentPool::~RTAttachmentPool()
{
}

void RTAttachmentPool::copy(const RTAttachmentPool& other) {
    for (U8 i = 0; i < to_base(RTAttachment::Type::COUNT); ++i) {
        for (U8 j = 0; j < other._attachment[i].size(); ++j) {
            const RTAttachment_ptr& att = other._attachment[i][j];
            if (att != nullptr) {
                RTAttachmentDescriptor descriptor = {};
                descriptor._clearColour = att->clearColour();
                descriptor._index = j;
                descriptor._type = static_cast<RTAttachment::Type>(i);
                descriptor._texDescriptor = att->texture()->getDescriptor();

                update(descriptor);
            }
        }
    }
}

RTAttachment_ptr& RTAttachmentPool::update(const RTAttachmentDescriptor& descriptor) {
    U8 index = descriptor._index;
    RTAttachment::Type type = descriptor._type;
    assert(index < to_U8(_attachment[to_U32(type)].size()));

    RTAttachment_ptr& ptr = getInternal(_attachment, type, index);
    if (ptr != nullptr) {
        // Replacing existing attachment
        Console::d_printfn(Locale::get(_ID("WARNING_REPLACING_RT_ATTACHMENT")), 
                           _parent.getGUID(),
                           getAttachmentName(type),
                           index);
        // Just to be clear about our intentions
        ptr.reset();
    }
    ptr = std::make_shared<RTAttachment>();

    ResourceDescriptor textureAttachment(Util::StringFormat("FBO_%s_Att_%s_%d_%d",
                                                            _parent.getName().c_str(),
                                                            getAttachmentName(type),
                                                            index,
                                                            _parent.getGUID()));
    textureAttachment.setThreadedLoading(false);
    textureAttachment.setPropertyDescriptor(descriptor._texDescriptor);

    GFXDevice& context = _parent.context();
    ResourceCache& parentCache = context.parent().resourceCache();
    Texture_ptr tex = CreateResource<Texture>(parentCache, textureAttachment);
    assert(tex);

    Texture::TextureLoadInfo info;
    tex->loadData(info, NULL, vec2<U16>(_parent.getWidth(), _parent.getHeight()));

    ptr->texture(tex);
    ptr->clearColour(descriptor._clearColour);

    _attachmentCount[to_U32(type)]++;

    return ptr;
}

bool RTAttachmentPool::clear(RTAttachment::Type type, U8 index) {
    RTAttachment_ptr& ptr = getInternal(_attachment, type, index);
    if (ptr != nullptr) {
        ptr.reset();
        return true;
    }

    return false;
}

RTAttachment_ptr& RTAttachmentPool::getInternal(AttachmentPool& pool, RTAttachment::Type type, U8 index) {
    switch (type) {
        case RTAttachment::Type::Colour:
        {
            assert(index < to_U8(_attachment[to_U32(type)].size()));
            return pool[to_base(RTAttachment::Type::Colour)][index];
        }
        case RTAttachment::Type::Depth:
        {
            assert(index == 0);
            return pool[to_base(RTAttachment::Type::Depth)].front();
        }
        case RTAttachment::Type::Stencil:
        {
            assert(index == 0);
            return pool[to_base(RTAttachment::Type::Stencil)].front();
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
            return pool[to_base(RTAttachment::Type::Colour)][index];
        }
        case RTAttachment::Type::Depth:
        {
            assert(index == 0);
            return pool[to_base(RTAttachment::Type::Depth)].front();
        }
        case RTAttachment::Type::Stencil:
        {
            assert(index == 0);
            return pool[to_base(RTAttachment::Type::Stencil)].front();
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

    std::back_insert_iterator<vectorImpl<RTAttachment_ptr>> back_it(attachments);
    auto const usedPredicate = [](const RTAttachment_ptr& ptr) { return ptr && ptr->used(); };
    std::copy_if(std::begin(_attachment[to_U32(type)]), std::end(_attachment[to_U32(type)]), back_it, usedPredicate);
}

U8 RTAttachmentPool::attachmentCount(RTAttachment::Type type) const {
    return _attachmentCount[to_U32(type)];
}

}; //namespace Divide
